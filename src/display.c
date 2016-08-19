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
#include <X11/keysym.h>

/* connection to the X server */
static xcb_connection_t *conn;

/* root screen of the X display and root window of screen */
static xcb_screen_t *root_screen;
static xcb_window_t root;

/* X11 keysyms */
static xcb_key_symbols_t *keysyms;

static unsigned int convert_x11_keysym(unsigned int keysym);
#endif


#if defined(__CYGWIN__) || defined (__MINGW32__)
#include <Windows.h>

static unsigned int convert_win_keycode(unsigned int keycode);
#endif


#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
/* keycode definitions */
#include <Carbon/Carbon.h>
#include <search.h>

/* array of all mapped keycodes */
static int *mapped_keys;

/* number of keys mapped */
static size_t nmapped;

static CGEventRef callback(CGEventTapProxy proxy, CGEventType type,
		CGEventRef event, void *refcon);
static unsigned int convert_osx_keycode(unsigned int keycode);
static int intcmp(const void *a, const void *b);
#endif


static void map_keys();


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
void start_loop()
{
	xcb_generic_event_t *e;
	xcb_keysym_t ks;
	unsigned int keycode;

	/* assign listeners for every mapped key */
	map_keys();

	while ((e = xcb_wait_for_event(conn))) {
		switch (e->response_type & ~0x80) {
		case XCB_KEY_PRESS:
			ks = xcb_key_press_lookup_keysym(keysyms,
					(xcb_key_press_event_t *)e, 0);
			keycode = convert_x11_keysym(ks);
			process_hotkey(keycode);
			break;
		default:
			break;
		}
		free(e);
	}
}

/* map_keys: grab all provided hotkeys */
static void map_keys()
{
	/* temp */
	const unsigned int keys[] = { XK_q, XK_w, XK_e };
	size_t i;
	unsigned int mods = 0;

	xcb_keycode_t *kc;
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *err;

	for (i = 0; i < 3; ++i) {
		kc = xcb_key_symbols_get_keycode(keysyms, keys[i]);
		cookie = xcb_grab_key_checked(conn, 1, root, mods, kc[0],
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

		/* key grab will fail if the key is already grabbed */
		if ((err = xcb_request_check(conn, cookie))) {
			fprintf(stderr, "error: the key '%s' is already "
					"mapped by another program\n",
					keystr(convert_x11_keysym(keys[i])));
			free(err);
		}
		/* bind key with num lock active */
		xcb_grab_key(conn, 1, root, mods | XCB_MOD_MASK_2, kc[0],
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		/* bind key with caps lock active */
		xcb_grab_key(conn, 1, root, mods | XCB_MOD_MASK_LOCK, kc[0],
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		/* bind key with both active */
		xcb_grab_key(conn, 1, root,
				mods | XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2, kc[0],
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		free(kc);
	}
	xcb_flush(conn);
}

/* convert_x11_keysym: convert a x11 keysym to a kbm keycode */
static unsigned int convert_x11_keysym(unsigned int keysym)
{
	switch (keysym) {
	case XK_q:
		return KEY_Q;
	case XK_w:
		return KEY_W;
	case XK_e:
		return KEY_E;
	default:
		return 0;
	}
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
void start_loop()
{
	MSG msg;
	unsigned int keycode;

	map_keys();

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_HOTKEY) {
			/* event keycode is stored in the upper half of lParam */
			keycode = convert_win_keycode((msg.lParam >> 16) & 0xFFFF);
			process_hotkey(keycode);
		}
	}
}

/* map_keys: register all provided hotkeys */
static void map_keys()
{
	/* temp */
	const unsigned int keys[] = { 0x51, 0x57, 0x45 };
	size_t i;
	unsigned int mods = 0;

	for (i = 0; i < 3; ++i) {
		if (!RegisterHotKey(NULL, 1, mods, keys[i]))
			fprintf(stderr, "error: the key '%s' is already "
					"mapped by another program\n",
					keystr(convert_win_keycode(keys[i])));
	}
}

/* convert_win_keycode: convert win32 keycode to a kbm keycode */
static unsigned int convert_win_keycode(unsigned int keycode)
{
	switch (keycode) {
	case 0x51:
		return KEY_Q;
	case 0x57:
		return KEY_W;
	case 0x45:
		return KEY_E;
	default:
		return 0;
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

	mapped_keys = NULL;
}

void close_display()
{
	if (mapped_keys)
		free(mapped_keys);
}

void start_loop()
{
	map_keys();
	CFRunLoopRun();
}

/* map_keys: grab all provided hotkeys */
static void map_keys()
{
	const unsigned int keys[] = { kVK_ANSI_Q, kVK_ANSI_W, kVK_ANSI_E };
	size_t i;

	mapped_keys = malloc(3 * sizeof(*mapped_keys));

	for (i = 0; i < 3; ++i)
		mapped_keys[i] = keys[i];
	nmapped = i;
}

/* convert_osx_keycode: convert osx keycode to a kbm keycode */
static unsigned int convert_osx_keycode(unsigned int keycode)
{
	switch (keycode) {
	case kVK_ANSI_Q:
		return KEY_Q;
	case kVK_ANSI_W:
		return KEY_W;
	case kVK_ANSI_E:
		return KEY_E;
	default:
		return 0;
	}
}

/* callback: function called when event is registered */
static CGEventRef callback(CGEventTapProxy proxy, CGEventType type,
		CGEventRef event, void *refcon)
{
	CGKeyCode keycode;
	unsigned int kc;

	/* just in case */
	if (type != kCGEventKeyDown)
		return event;

	keycode = (CGKeyCode)CGEventGetIntegerValueField(event,
			kCGKeyboardEventKeycode);
	if (lfind(&keycode, mapped_keys, &nmapped, sizeof(*mapped_keys),
				intcmp)) {
		kc = convert_osx_keycode(keycode);
		process_hotkey(kc);
		/* prevent the event from propagating further */
		return NULL;
	}
	return event;
}

/* intcmp: compare two integers */
static int intcmp(const void *a, const void *b)
{
	return *(int *)a - *(int *)b;
}
#endif
