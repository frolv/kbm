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
#include "kbm.h"
#include "display.h"
#include "keymap.h"

#ifdef __linux__
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

/* connection to the X server */
static xcb_connection_t *conn;
static xcb_screen_t *root_screen;
/* root X window */
static xcb_window_t root;
/* X11 keysyms */
static xcb_key_symbols_t *keysyms;

static void map_keys();
static unsigned int convert_x11_keysym(unsigned int keysym);
#endif

void init_display()
{
	int screen;

	if (!(conn = xcb_connect(NULL, &screen))) {
		fprintf(stderr, "error: failed to connect to X server\n");
		exit(1);
	}
	root_screen = xcb_aux_get_screen(conn, screen);
	root = root_screen->root;
	keysyms = xcb_key_symbols_alloc(conn);
}

void close_display()
{
	xcb_disconnect(conn);
	xcb_key_symbols_free(keysyms);
}

void start_loop()
{
	xcb_generic_event_t *e;
	xcb_keysym_t ks;
	int keycode;

	/* create listeners for every mapped key */
	map_keys();

	while ((e = xcb_wait_for_event(conn))) {
		switch (e->response_type & ~0x80) {
		case XCB_KEY_PRESS:
			ks = xcb_key_press_lookup_keysym(keysyms,
					(xcb_key_press_event_t *)e, 0);
			keycode = convert_x11_keysym(ks);
			printf("%s\n", keystr(keycode));
			break;
		default:
			break;
		}
	}
}

#ifdef __linux__
/* map_keys: grab all provided hotkeys */
static void map_keys()
{
	/* temp */
	const unsigned int keys[] = { XK_q, XK_w, XK_e };
	size_t i;

	xcb_keycode_t *kc;

	for (i = 0; i < 3; ++i) {
		kc = xcb_key_symbols_get_keycode(keysyms, keys[i]);
		xcb_grab_key(conn, 1, root, 0, kc[0], XCB_GRAB_MODE_ASYNC,
				XCB_GRAB_MODE_ASYNC);
		/* also bind key with numlock + capslock */
	}
	xcb_flush(conn);
}

/* convert_x11_keysym: convert a x11 keysm to a kbm keycode */
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
