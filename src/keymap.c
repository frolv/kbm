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

static char key_str[32];

/* keystr: return a string representation of key corresponding to keycode */
char *keystr(uint8_t keycode, uint8_t mask)
{
	key_str[0] = '\0';

	if (CHECK_MOD(mask, KBM_CTRL_MASK))
		strcat(key_str, "Ctrl-");
	if (CHECK_MOD(mask, KBM_SUPER_MASK))
		strcat(key_str, "Super-");
	if (CHECK_MOD(mask, KBM_META_MASK))
		strcat(key_str, "Alt-");
	if (CHECK_MOD(mask, KBM_SHIFT_MASK))
		strcat(key_str, "Shift-");

	switch (keycode) {
	case KEY_Q:
		strcat(key_str, "q");
		break;
	case KEY_W:
		strcat(key_str, "w");
		break;
	case KEY_E:
		strcat(key_str, "e");
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

unsigned int kbm_to_keysym(uint8_t keycode)
{
	return x11_keysyms[keycode];
}

unsigned int kbm_to_xcb_masks(uint8_t modmask)
{
	unsigned int mask = 0;

	if (CHECK_MOD(modmask, KBM_SHIFT_MASK))
		mask |= XCB_MOD_MASK_SHIFT;
	if (CHECK_MOD(modmask, KBM_CTRL_MASK))
		mask |= XCB_MOD_MASK_CONTROL;
	if (CHECK_MOD(modmask, KBM_SUPER_MASK))
		mask |= XCB_MOD_MASK_4;
	if (CHECK_MOD(modmask, KBM_META_MASK))
		mask |= XCB_MOD_MASK_1;

	return mask;
}
#endif

#if defined(__CYGWIN__) || defined (__MINGW32__)
static const uint32_t win_keycodes[] = {
	0x00, 0x51, 0x57, 0x45
};

unsigned int kbm_to_win32(uint8_t keycode)
{
	return win_keycodes[keycode];
}

unsigned int kbm_to_win_masks(uint8_t modmask)
{
	unsigned int mask = 0;

	if (CHECK_MOD(modmask, KBM_SHIFT_MASK))
		mask |= MOD_SHIFT;
	if (CHECK_MOD(modmask, KBM_CTRL_MASK))
		mask |= MOD_CONTROL;
	if (CHECK_MOD(modmask, KBM_SUPER_MASK))
		mask |= MOD_WIN;
	if (CHECK_MOD(modmask, KBM_META_MASK))
		mask |= MOD_ALT;

	return mask;
}
#endif

#ifdef __APPLE__
static const uint32_t osx_keycodes[] = {
	0x00, kVK_ANSI_Q, kVK_ANSI_W, kVK_ANSI_E
};

unsigned int kbm_to_carbon(uint8_t keycode)
{
	return osx_keycodes[keycode];
}

unsigned int kbm_to_osx_masks(uint8_t modmask)
{
	unsigned int mask = 0;

	if (CHECK_MOD(modmask, KBM_SHIFT_MASK))
		mask |= kCGEventFlagMaskShift;
	if (CHECK_MOD(modmask, KBM_CTRL_MASK))
		mask |= kCGEventFlagMaskControl;
	if (CHECK_MOD(modmask, KBM_SUPER_MASK))
		mask |= kCGEventFlagMaskCommand;
	if (CHECK_MOD(modmask, KBM_META_MASK))
		mask |= kCGEventFlagMaskAlternate;

	return mask;
}
#endif
