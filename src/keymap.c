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

static char key_str[64];

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

	/* I love vim macros */
	switch (keycode) {
	case KEY_SPACE:
		strcat(key_str, "Space");
		break;
	case KEY_ESCAPE:
		strcat(key_str, "Escape");
		break;
	case KEY_BSPACE:
		strcat(key_str, "Backspace");
		break;
	case KEY_TAB:
		strcat(key_str, "Tab");
		break;
	case KEY_CAPS:
		strcat(key_str, "CapsLock");
		break;
	case KEY_ENTER:
		strcat(key_str, "Enter");
		break;
	case KEY_SHIFT:
		strcat(key_str, "Shift");
		break;
	case KEY_CTRL:
		strcat(key_str, "Control");
		break;
	case KEY_SUPER:
		strcat(key_str, "Super");
		break;
	case KEY_META:
		strcat(key_str, "Meta");
		break;
	case KEY_F1:
		strcat(key_str, "F1");
		break;
	case KEY_F2:
		strcat(key_str, "F2");
		break;
	case KEY_F3:
		strcat(key_str, "F3");
		break;
	case KEY_F4:
		strcat(key_str, "F4");
		break;
	case KEY_F5:
		strcat(key_str, "F5");
		break;
	case KEY_F6:
		strcat(key_str, "F6");
		break;
	case KEY_F7:
		strcat(key_str, "F7");
		break;
	case KEY_F8:
		strcat(key_str, "F8");
		break;
	case KEY_F9:
		strcat(key_str, "F9");
		break;
	case KEY_F10:
		strcat(key_str, "F10");
		break;
	case KEY_F11:
		strcat(key_str, "F11");
		break;
	case KEY_F12:
		strcat(key_str, "F12");
		break;
	case KEY_PRTSCR:
		strcat(key_str, "PrintScreen");
		break;
	case KEY_SCRLCK:
		strcat(key_str, "ScrollLock");
		break;
	case KEY_PAUSE:
		strcat(key_str, "Pause");
		break;
	case KEY_INSERT:
		strcat(key_str, "Insert");
		break;
	case KEY_DELETE:
		strcat(key_str, "Delete");
		break;
	case KEY_HOME:
		strcat(key_str, "Home");
		break;
	case KEY_END:
		strcat(key_str, "End");
		break;
	case KEY_PGUP:
		strcat(key_str, "PageUp");
		break;
	case KEY_PGDOWN:
		strcat(key_str, "PageDown");
		break;
	case KEY_LARROW:
		strcat(key_str, "Left");
		break;
	case KEY_RARROW:
		strcat(key_str, "Right");
		break;
	case KEY_UARROW:
		strcat(key_str, "Up");
		break;
	case KEY_DARROW:
		strcat(key_str, "Down");
		break;
	case KEY_NUMLOCK:
		strcat(key_str, "NumLock");
		break;
	case KEY_NUMDIV:
		strcat(key_str, "NumDiv");
		break;
	case KEY_NUMMULT:
		strcat(key_str, "NumMult");
		break;
	case KEY_NUMMINUS:
		strcat(key_str, "NumMinus");
		break;
	case KEY_NUMPLUS:
		strcat(key_str, "NumPlus");
		break;
	case KEY_NUMENTER:
		strcat(key_str, "NumEnter");
		break;
	case KEY_NUMDEC:
		strcat(key_str, "NumDot");
		break;
	case KEY_NUM0:
		strcat(key_str, "Num0");
		break;
	case KEY_NUM1:
		strcat(key_str, "Num1");
		break;
	case KEY_NUM2:
		strcat(key_str, "Num2");
		break;
	case KEY_NUM3:
		strcat(key_str, "Num3");
		break;
	case KEY_NUM4:
		strcat(key_str, "Num4");
		break;
	case KEY_NUM5:
		strcat(key_str, "Num5");
		break;
	case KEY_NUM6:
		strcat(key_str, "Num6");
		break;
	case KEY_NUM7:
		strcat(key_str, "Num7");
		break;
	case KEY_NUM8:
		strcat(key_str, "Num8");
		break;
	case KEY_NUM9:
		strcat(key_str, "Num9");
		break;
	default:
		break;
	}

	return key_str;
}


#ifdef __linux__
static const uint32_t x11_keysyms[] = {
	0x00, XK_q, XK_w, XK_e, XK_r, XK_t, XK_y, XK_u, XK_i, XK_o, XK_p, XK_a,
	XK_s, XK_d, XK_f, XK_g, XK_h, XK_j, XK_k, XK_l, XK_z, XK_x, XK_c, XK_v,
	XK_b, XK_n, XK_m, XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9,
	XK_0, XK_grave, XK_minus, XK_equal, XK_bracketleft, XK_bracketright,
	XK_backslash, XK_semicolon, XK_apostrophe, XK_comma, XK_period,
	XK_slash, XK_space, XK_Escape, XK_BackSpace, XK_Tab, XK_Caps_Lock,
	XK_Return, XK_Shift_L, XK_Control_L, XK_Super_L, XK_Alt_L, XK_F1, XK_F2,
	XK_F3, XK_F4, XK_F5, XK_F6, XK_F7, XK_F8, XK_F9, XK_F10, XK_F11, XK_F12,
	XK_Print, XK_Scroll_Lock, XK_Pause, XK_Insert, XK_Delete, XK_Home,
	XK_End, XK_Page_Up, XK_Page_Down, XK_Left, XK_Right, XK_Up, XK_Down,
	XK_Num_Lock, XK_KP_Divide, XK_KP_Multiply, XK_KP_Subtract, XK_KP_Add,
	XK_KP_Enter, XK_KP_Delete, XK_KP_Insert, XK_KP_End, XK_KP_Down,
	XK_KP_Next, XK_KP_Left, XK_KP_Begin, XK_KP_Right, XK_KP_Home, XK_KP_Up,
	XK_KP_Prior
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
	0x00, 0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4F, 0x50, 0x41,
	0x53, 0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x5A, 0x58, 0x43, 0x56,
	0x42, 0x4E, 0x4D, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x30, 0xC0, 0xBD, 0xBB, 0xDB, 0xDD, 0xDC, 0xBA, 0xDE, 0xBC, 0xBE, 0xBF,
	0x20, 0x1B, 0x08, 0x09, 0x14, 0x0D, 0x10, 0x11, 0x5B, 0x12, 0x70, 0x71,
	0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x2C, 0x91,
	0x13, 0x2D, 0x2E, 0x24, 0x23, 0x21, 0x22, 0x25, 0x27, 0x26, 0x28, 0x90,
	0x6F, 0x6A, 0x6D, 0x6B, 0x0D, 0x6E, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65,
	0x66, 0x67, 0x68, 0x69
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
