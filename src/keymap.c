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

/*
 * String of all single character keys, in the order they are defined in
 * keymap.h. The location of a key in the string can be found by multiplying
 * its keycode by 2.
 */
static const char *SINGLE_KEYS =
"z\0Q\0W\0E\0R\0T\0Y\0U\0I\0O\0P\0A\0S\0D\0F\0G\0H\0J\0K\0L\0Z\0X\0C\0V\0B\0N\0"
"M\0" "1\0" "2\0" "3\0" "4\0" "5\0" "6\0" "7\0" "8\0" "9\0" "0\0`\0-\0=\0[\0]\0"
"\\\0;\0'\0,\0.\0/\0";

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

	if (keycode <= KEY_FSLASH) {
		strcat(key_str, SINGLE_KEYS + (keycode << 1));
		return key_str;
	}

	return key_str;
}


#ifdef __linux__
static const uint32_t x11_keysyms[] = {
	0x00, XK_q, XK_w, XK_e, XK_r, XK_t, XK_y, XK_u, XK_i, XK_o, XK_p, XK_a,
	XK_s, XK_d, XK_f, XK_g, XK_h, XK_j, XK_k, XK_l, XK_z, XK_x, XK_c, XK_v,
	XK_b, XK_n, XK_m, XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9,
	XK_0, XK_grave, XK_minus, XK_equal, XK_bracketleft, XK_bracketright,
	XK_backslash, XK_semicolon, XK_apostrophe, XK_comma, XK_period, XK_slash
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
