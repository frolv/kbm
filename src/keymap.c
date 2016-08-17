/*
 * keymap.c
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

#include <string.h>
#include "keymap.h"

static char key_str[8];

/* keystr: return a string representation of key corresponding to keycode */
char *keystr(unsigned int keycode)
{
	switch (keycode) {
	case KEY_Q:
		strcpy(key_str, "q");
		break;
	case KEY_W:
		strcpy(key_str, "w");
		break;
	case KEY_E:
		strcpy(key_str, "e");
		break;
	default:
		key_str[0] = '\0';
		break;
	}
	return key_str;
}
