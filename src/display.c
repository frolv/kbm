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

/* head of the hotkey linked list */
static struct hotkey *keymaps;

static CGEventRef callback(CGEventTapProxy proxy, CGEventType type,
		CGEventRef event, void *refcon);
#endif


static void map_keys(struct hotkey *head);
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
}

/* close_display: disconnect from X server and clean up */
void close_display()
{
	xcb_key_symbols_free(keysyms);
	xcb_disconnect(conn);
}

/* start_loop: map all hotkeys and start listening for keypresses */
void start_loop(struct hotkey *head)
{
	xcb_generic_event_t *e;
	xcb_key_press_event_t *evt;
	xcb_keysym_t ks;
	struct hotkey *hk;

	/* assign listeners for every mapped key */
	map_keys(head);

	while ((e = xcb_wait_for_event(conn))) {
		switch (e->response_type & ~0x80) {
		case XCB_KEY_PRESS:
			evt = (xcb_key_press_event_t *)e;
			ks = xcb_key_press_lookup_keysym(keysyms, evt, 0);

			/* unset the caps lock and num lock bits */
			evt->state &= ~(XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2);

			if (!(hk = find_by_os_code(head, ks, evt->state))) {
				/* this sometimes happens when keys are */
				/* pressed quickly in succession */
				/* event should be sent back out */
				continue;
			}
			process_hotkey(hk);
			break;
		default:
			break;
		}
		free(e);
	}
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
		/* bind key with num lock active */
		xcb_grab_key(conn, 1, root, head->os_modmask | XCB_MOD_MASK_2,
				kc[0], XCB_GRAB_MODE_ASYNC,
				XCB_GRAB_MODE_ASYNC);
		/* bind key with caps lock active */
		xcb_grab_key(conn, 1, root, head->os_modmask
				| XCB_MOD_MASK_LOCK, kc[0],
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		/* bind key with both active */
		xcb_grab_key(conn, 1, root, head->os_modmask
				| XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2, kc[0],
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		free(kc);
	}
	xcb_flush(conn);
}
#endif


#if defined(__CYGWIN__) || defined (__MINGW32__)
void init_display()
{
}

void close_display()
{
}

/* start_loop: map hotkeys and start listening for keypresses */
void start_loop(struct hotkey *head)
{
	MSG msg;
	struct hotkey *hk;
	unsigned int kc, mask;

	map_keys(head);

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_HOTKEY) {
			/* mod mask is stored in the lower half of lParam */
			mask = msg.lParam & 0xFFFF;
			/* keycode is stored in the upper half of lParam */
			kc = (msg.lParam >> 16) & 0xFFFF;
			if (!(hk = find_by_os_code(head, kc, mask)))
				/* should never happen */
				continue;
			process_hotkey(hk);
		}
	}
}

/* map_keys: register all provided hotkeys */
static void map_keys(struct hotkey *head)
{
	for (; head; head = head->next) {
		if (!RegisterHotKey(NULL, 1, head->os_modmask, head->os_code))
			fprintf(stderr, "error: the key '%s' is already "
					"mapped by another program\n",
					keystr(head->kbm_code,
						head->kbm_modmask));
	}
}
#endif


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

void start_loop(struct hotkey *head)
{
	map_keys(head);
	CFRunLoopRun();
}

/* callback: function called when event is registered */
static CGEventRef callback(CGEventTapProxy proxy, CGEventType type,
		CGEventRef event, void *refcon)
{
	CGKeyCode keycode;
	struct hotkey *hk;

	/* just in case */
	if (type != kCGEventKeyDown)
		return event;

	keycode = (CGKeyCode)CGEventGetIntegerValueField(event,
			kCGKeyboardEventKeycode);
	if ((hk = find_by_os_code(keymaps, keycode, 0))) {
		process_hotkey(hk);
		/* prevent the event from propagating further */
		return NULL;
	}
	return event;
}

static void map_keys(struct hotkey *head)
{
	keymaps = head;
}
#endif

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
