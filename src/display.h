/*
 * display.h
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

#ifndef KBM_DISPLAY_H
#define KBM_DISPLAY_H

#include "hotkey.h"

enum buttons {
	KBM_BUTTON_LEFT		= 1,
	KBM_BUTTON_MIDDLE	= 2,
	KBM_BUTTON_RIGHT	= 3
};

/*
 * init_display: perform OS-specific initialization
 * actions for a graphical display
 */
void init_display();

/* close_display: perform OS-specific cleanup actions for a graphical display */
void close_display();

/* start_loop: map the provided hotkeys and begin an event loop */
void start_loop();

void load_keys(struct hotkey *head);

void unload_keys();

/* send_button: send a button event */
void send_button(enum buttons button);

/* move_cursor: move cursor along vector x,y from current position */
void move_cursor(int x, int y);

/* toggle_keys: disable hotkeys if active, enable otherwise */
void toggle_keys();

#endif /* KBM_DISPLAY_H */
