/*
 * display.h
 * Copyright (C) 2016-2017 Alexei Frolov
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

#ifndef KBM_DISPLAY_H
#define KBM_DISPLAY_H

#include "hotkey.h"

/* buttons on a mouse */
enum {
	KBM_BUTTON_LEFT         = 1,
	KBM_BUTTON_MIDDLE       = 2,
	KBM_BUTTON_RIGHT        = 3
};

/*
 * init_display: perform OS-specific initialization
 * actions for a graphical display
 */
int init_display(void);

/* close_display: perform OS-specific cleanup actions for a graphical display */
void close_display(void);

/* start_listening: map the provided hotkeys and begin an event loop */
void start_listening(void);

/* load_keys: store list of keys starting at head */
void load_keys(struct hotkey *head);

/* unload_keys: remove and free stored hotkey lists */
void unload_keys(void);

/* send_button: send a button event */
void send_button(unsigned int button);

/* send_key: send a key event */
void send_key(unsigned int keycode, unsigned int modmask, unsigned int type);

/* move_cursor: move cursor along vector x,y from current position */
void move_cursor(int x, int y);

/* toggle_keys: disable hotkeys if active; enable otherwise */
void toggle_keys(void);

/* kbm_exec: execute the specified program */
void kbm_exec(void *args);

#endif /* KBM_DISPLAY_H */
