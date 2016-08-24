/*
 * display.c
 * Copyright (C) 2016 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include "display.h"
#include "kbm.h"
#include "keymap.h"
#include "hotkey.h"


#ifdef __linux__
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xtest.h>

/* connection to the X server */
static xcb_connection_t *conn;

/* root screen of the X display and root window of screen */
static xcb_screen_t *root_screen;
static xcb_window_t root;

/* X11 keysyms */
static xcb_key_symbols_t *keysyms;
#endif


#if defined(__CYGWIN__) || defined (__MINGW32__)
#include <Windows.h>
#endif


#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>

static CGEventRef callback(CGEventTapProxy proxy, CGEventType type,
		CGEventRef event, void *refcon);
#endif


/* all hotkey mappings excluding toggles */
static struct hotkey *actions;
/* toggle hotkey mappings */
static struct hotkey *toggles;

/* whether hotkeys are currently enabled */
static int keys_active;

static void map_keys(struct hotkey *head);
static void unmap_keys(struct hotkey *head);

static struct hotkey *find_by_os_code(struct hotkey *head,
		uint32_t code, uint32_t mask);


#ifdef __linux__
/* init_display: connect to the X server and grab the root window */
void init_display()
{
	int screen;

	if (!(conn = xcb_connect(NULL, &screen))) {
		fprintf(stderr, "error: failed to connect to X server\n");
		exit(1);
	}
	/* get the root screen and root window of the X display */
	root_screen = xcb_aux_get_screen(conn, screen);
	root = root_screen->root;
	keysyms = xcb_key_symbols_alloc(conn);

	actions = toggles = NULL;
}

/* close_display: disconnect from X server and clean up */
void close_display()
{
	unmap_keys(actions);
	unmap_keys(toggles);
	xcb_key_symbols_free(keysyms);
	xcb_disconnect(conn);
}

/* start_loop: map all hotkeys and start listening for keypresses */
void start_loop()
{
	xcb_generic_event_t *e;
	xcb_key_press_event_t *evt;
	xcb_keysym_t ks;
	struct hotkey *hk;
	unsigned int running = 1;

	while (running && (e = xcb_wait_for_event(conn))) {
		switch (e->response_type & ~0x80) {
		case XCB_KEY_PRESS:
			evt = (xcb_key_press_event_t *)e;
			ks = xcb_key_press_lookup_keysym(keysyms, evt, 0);

			/* unset the caps lock and num lock bits */
			evt->state &= ~(XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2);

			if (!(hk = find_by_os_code(actions, ks, evt->state))
					&& !(hk = find_by_os_code(toggles,
							ks, evt->state))) {
				/*
				 * This sometimes happens when keys are
				 * pressed in quick succession.
				 * The event should be sent back out.
				 */
				free(e);
				continue;
			}
			if (process_hotkey(hk) == -1)
				running = 0;
			break;
		default:
			break;
		}
		free(e);
	}
}

/* send_button: send a button event */
void send_button(enum buttons button)
{
	xcb_test_fake_input(conn, XCB_BUTTON_PRESS, button, XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
	xcb_test_fake_input(conn, XCB_BUTTON_RELEASE, button, XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
	xcb_flush(conn);
}

/* move_cursor: move cursor along vector x,y from current position */
void move_cursor(int x, int y)
{
	xcb_warp_pointer(conn, XCB_NONE, XCB_NONE, 0, 0, 0, 0, x, y);
	xcb_flush(conn);
}

/* map_keys: grab all provided hotkeys */
static void map_keys(struct hotkey *head)
{
	xcb_keycode_t *kc;
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *err;

	for (; head; head = head->next) {
		kc = xcb_key_symbols_get_keycode(keysyms, head->os_code);
		cookie = xcb_grab_key_checked(conn, 1, root, head->os_modmask,
				kc[0], XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

		/* key grab will fail if the key is already grabbed */
		if ((err = xcb_request_check(conn, cookie))) {
			fprintf(stderr, "error: the key '%s' is already "
					"mapped by another program\n",
					keystr(head->kbm_code,
						head->kbm_modmask));
			free(err);
		}

		/*
		 * In X11, caps lock and num lock are defined as modifiers and
		 * events involving these keys held down are treated as
		 * different events to those occurring without them.
		 *
		 * We don't want to distinguish between these events, so we
		 * also grab the key with the caps and num lock masks.
		 */

		/* num lock */
		xcb_grab_key(conn, 1, root, head->os_modmask | XCB_MOD_MASK_2,
				kc[0], XCB_GRAB_MODE_ASYNC,
				XCB_GRAB_MODE_ASYNC);
		/* caps lock */
		xcb_grab_key(conn, 1, root, head->os_modmask
				| XCB_MOD_MASK_LOCK, kc[0],
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		/* both */
		xcb_grab_key(conn, 1, root, head->os_modmask
				| XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2, kc[0],
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		free(kc);
	}
	xcb_flush(conn);
	keys_active = 1;
}

/* unmap_keys: ungrab all assigned hotkeys */
static void unmap_keys(struct hotkey *head)
{
	xcb_keycode_t *kc;

	for (; head; head = head->next) {

		kc = xcb_key_symbols_get_keycode(keysyms, head->os_code);
		xcb_ungrab_key(conn, kc[0], root, head->os_modmask);

		/* account for num lock and caps lock modifiers */
		xcb_ungrab_key(conn, kc[0], root, head->os_modmask
				| XCB_MOD_MASK_2);
		xcb_ungrab_key(conn, kc[0], root, head->os_modmask
				| XCB_MOD_MASK_LOCK);
		xcb_ungrab_key(conn, kc[0], root, head->os_modmask
				| XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK);
	}
	xcb_flush(conn);
	keys_active = 0;
}
#endif /* __linux__ */


#if defined(__CYGWIN__) || defined (__MINGW32__)
void init_display()
{
}

void close_display()
{
}

/* start_loop: map hotkeys and start listening for keypresses */
void start_loop()
{
	MSG msg;
	struct hotkey *hk;
	unsigned int kc, mask;

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_HOTKEY) {
			/* mod mask is stored in the lower half of lParam */
			mask = msg.lParam & 0xFFFF;
			/* keycode is stored in the upper half of lParam */
			kc = (msg.lParam >> 16) & 0xFFFF;
			if (!(hk = find_by_os_code(actions, kc, mask))
					&& !(hk = find_by_os_code(toggles,
							kc, mask)))
				/* should never happen */
				continue;
			if (process_hotkey(hk) == -1)
				break;
		}
	}
}

void send_button(enum buttons button)
{
	INPUT ip;

	ip.type = INPUT_MOUSE;

	memset(&ip.mi, 0, sizeof(ip.mi));

	/* send the button press event */
	switch (button) {
	case KBM_BUTTON_LEFT:
		ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		break;
	case KBM_BUTTON_MIDDLE:
		ip.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
		break;
	case KBM_BUTTON_RIGHT:
		ip.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		break;
	default:
		break;
	}
	SendInput(1, &ip, sizeof(ip));

	/* send the button release event */
	switch (button) {
	case KBM_BUTTON_LEFT:
		ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		break;
	case KBM_BUTTON_MIDDLE:
		ip.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
		break;
	case KBM_BUTTON_RIGHT:
		ip.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		break;
	default:
		break;
	}
	SendInput(1, &ip, sizeof(ip));
}

/* move_cursor: move cursor along vector x,y from current position */
void move_cursor(int x, int y)
{
	POINT pt;

	GetCursorPos(&pt);
	printf("%d %d\n", pt.x, pt.y);
	SetCursorPos(pt.x + x, pt.y + y);
}

/* map_keys: register all provided hotkeys */
static void map_keys(struct hotkey *head)
{
	for (; head; head = head->next) {
		if (!RegisterHotKey(NULL, head->id, head->os_modmask,
					head->os_code))
			fprintf(stderr, "error: the key '%s' is already "
					"mapped by another program\n",
					keystr(head->kbm_code,
						head->kbm_modmask));
	}
	keys_active = 1;
}

static void unmap_keys(struct hotkey *head)
{
	for (; head; head = head->next) {
		UnregisterHotKey(NULL, head->id);
	}
	keys_active = 0;
}
#endif /* __CYGWIN__ || __MINGW32__ */


#ifdef __APPLE__
/* init_display: enable the keypress event tap */
void init_display()
{
	CFMachPortRef tap;
	CGEventMask mask;
	CFRunLoopSourceRef src;

	mask = 1 << kCGEventKeyDown;
	tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
			0, mask, callback, NULL);
	if (!tap) {
		/* enable access for assistive devices */
		fprintf(stderr, "error: failed to create event tap\n");
		exit(1);
	}

	src = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), src, kCFRunLoopCommonModes);
	CGEventTapEnable(tap, true);
}

void close_display()
{
}

void start_loop()
{
	CFRunLoopRun();
}

void send_button(enum buttons button)
{
	CGEventRef posevent, downevent, upevent;
	CGPoint pos;
	CGEventType dtype, utype;
	CGMouseButton mb;

	/* get the current position of the cursor by making an empty event */
	posevent = CGEventCreate(NULL);
	pos = CGEventGetLocation(posevent);

	switch (button) {
	case KBM_BUTTON_LEFT:
		dtype = kCGEventLeftMouseDown;
		utype = kCGEventLeftMouseUp;
		mb = kCGMouseButtonLeft;
		break;
	case KBM_BUTTON_MIDDLE:
		dtype = kCGEventOtherMouseDown;
		utype = kCGEventOtherMouseUp;
		mb = kCGMouseButtonCenter;
		break;
	case KBM_BUTTON_RIGHT:
		dtype = kCGEventRightMouseDown;
		utype = kCGEventRightMouseUp;
		mb = kCGMouseButtonRight;
		break;
	default:
		break;
	}

	downevent = CGEventCreateMouseEvent(NULL, dtype, pos, mb);
	upevent = CGEventCreateMouseEvent(NULL, utype, pos, mb);

	CGEventPost(kCGHIDEventTap, downevent);
	CGEventPost(kCGHIDEventTap, upevent);

	CFRelease(posevent);
	CFRelease(downevent);
	CFRelease(upevent);
}

static void map_keys(struct hotkey *head)
{
	if (head->op != OP_TOGGLE)
		keys_active = 1;
}

static void unmap_keys(struct hotkey *head)
{
	if (head->op != OP_TOGGLE)
		keys_active = 0;
}

/* callback: function called when event is registered */
static CGEventRef callback(CGEventTapProxy proxy, CGEventType type,
		CGEventRef event, void *refcon)
{
	CGKeyCode keycode;
	CGEventFlags flags;
	struct hotkey *hk;

	/* just in case */
	if (type != kCGEventKeyDown)
		return event;

	keycode = (CGKeyCode)CGEventGetIntegerValueField(event,
			kCGKeyboardEventKeycode);
	flags = CGEventGetFlags(event);
	/* filter out all the bits we're not interested in */
	flags &= (kCGEventFlagMaskShift | kCGEventFlagMaskControl
			| kCGEventFlagMaskCommand | kCGEventFlagMaskAlternate);

	if (keys_active && (hk = find_by_os_code(actions, keycode, flags))) {
		if (process_hotkey(hk) == -1)
			CFRunLoopStop(CFRunLoopGetCurrent());
		/* prevent the event from propagating further */
		return NULL;
	}
	if ((hk = find_by_os_code(toggles, keycode, flags))) {
		process_hotkey(hk);
		return NULL;
	}
	return event;
}
#endif /* __APPLE__ */

void load_keys(struct hotkey *head)
{
	struct hotkey *tmp;

	while (head) {
		tmp = head;
		head = head->next;
		tmp->next = NULL;
		if (tmp->op == OP_TOGGLE)
			add_hotkey(&toggles, tmp);
		else
			add_hotkey(&actions, tmp);
	}

	map_keys(actions);
	map_keys(toggles);
}

void unload_keys()
{
	free_keys(actions);
	free_keys(toggles);
	actions = toggles = NULL;
}

void toggle_keys()
{
	if (keys_active)
		unmap_keys(actions);
	else
		map_keys(actions);
}

/* find_by_os_code: return the hotkey in head with os_code code */
static struct hotkey *find_by_os_code(struct hotkey *head,
		uint32_t code, uint32_t mask)
{
	for (; head; head = head->next) {
		if (head->os_code == code && head->os_modmask == mask)
			return head;
	}
	return NULL;
}
