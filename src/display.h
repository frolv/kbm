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

#ifndef _DISPLAY_H
#define _DISPLAY_H

/*
 * init_display: perform OS-specific initialization
 * actions for a graphical display
 */
void init_display();

/* close_display: perform OS-specific cleanup actions for a graphical display */
void close_display();

/* start_loop: map the provided hotkeys and begin an event loop */
void start_loop();

#endif
