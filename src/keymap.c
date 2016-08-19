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


#ifdef __linux__
static const uint32_t x11_keysyms[] = {
	0x00, XK_q, XK_w, XK_e
};

unsigned int kbm_to_keysym(uint32_t keycode)
{
	return x11_keysyms[keycode];
}

unsigned int kbm_to_xcb_masks(uint32_t modmask)
{
	return modmask;
}
#endif

#if defined(__CYGWIN__) || defined (__MINGW32__)
static const uint32_t win_keycodes[] = {
	0x00, 0x51, 0x57, 0x45
};

unsigned int kbm_to_win32(uint32_t keycode)
{
	return win_keycodes[keycode];
}

unsigned int kbm_to_win_masks(uint32_t modmask)
{
	return modmask;
}
#endif

#ifdef __APPLE__
static const uint32_t win_keycodes[] = {
	0x00, kVK_ANSI_Q, kVK_ANSI_W, kVK_ANSI_E
};

unsigned int kbm_to_carbon(uint32_t keycode)
{
}
#endif
