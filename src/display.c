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

#include "kbm.h"
#include "display.h"

#include <stdlib.h>

#ifdef __linux__
#include <X11/Xlib.h>

static Display *dpy;
#endif

void init_display()
{
	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "error: failed to open display\n");
		exit(1);
	}
}

void close_display()
{
	XCloseDisplay(dpy);
}
