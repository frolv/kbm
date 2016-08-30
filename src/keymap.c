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

#include <stdio.h>
#include <string.h>
#include "keymap.h"

static char key_str[64];

/*
 * String of all single character keys, in the order they are defined in
 * keymap.h. The location of a key in the string is equivalent to its keycode.
 */
static const char *SINGLE_KEYS =
"_QWERTYUIOPASDFGHJKLZXCVBNM1234567890`-=[]\\;'./";

/* keystr: return a string representation of key corresponding to keycode */
char *keystr(uint8_t keycode, uint8_t mask)
{
	char *s;

	key_str[0] = '\0';

	if (CHECK_MOD(mask, KBM_CTRL_MASK))
		strcat(key_str, "Ctrl-");
	if (CHECK_MOD(mask, KBM_SUPER_MASK))
		strcat(key_str, "Super-");
	if (CHECK_MOD(mask, KBM_META_MASK))
		strcat(key_str, "Alt-");
	if (CHECK_MOD(mask, KBM_SHIFT_MASK))
		strcat(key_str, "Shift-");

	s = strchr(key_str, '\0');
	if (keycode <= KEY_FSLASH) {
		sprintf(s, "%c", SINGLE_KEYS[keycode]);
		return key_str;
	}

	/* I love vim macros */
	switch (keycode) {
	case KEY_SPACE:
		strcat(s, "Space");
		break;
	case KEY_ESCAPE:
		strcat(s, "Escape");
		break;
	case KEY_BSPACE:
		strcat(s, "Backspace");
		break;
	case KEY_TAB:
		strcat(s, "Tab");
		break;
	case KEY_CAPS:
		strcat(s, "CapsLock");
		break;
	case KEY_ENTER:
		strcat(s, "Enter");
		break;
	case KEY_SHIFT:
		strcat(s, "Shift");
		break;
	case KEY_CTRL:
		strcat(s, "Control");
		break;
	case KEY_SUPER:
		strcat(s, "Super");
		break;
	case KEY_META:
		strcat(s, "Alt");
		break;
	case KEY_F1:
		strcat(s, "F1");
		break;
	case KEY_F2:
		strcat(s, "F2");
		break;
	case KEY_F3:
		strcat(s, "F3");
		break;
	case KEY_F4:
		strcat(s, "F4");
		break;
	case KEY_F5:
		strcat(s, "F5");
		break;
	case KEY_F6:
		strcat(s, "F6");
		break;
	case KEY_F7:
		strcat(s, "F7");
		break;
	case KEY_F8:
		strcat(s, "F8");
		break;
	case KEY_F9:
		strcat(s, "F9");
		break;
	case KEY_F10:
		strcat(s, "F10");
		break;
	case KEY_F11:
		strcat(s, "F11");
		break;
	case KEY_F12:
		strcat(s, "F12");
		break;
	case KEY_PRTSCR:
		strcat(s, "PrintScreen");
		break;
	case KEY_SCRLCK:
		strcat(s, "ScrollLock");
		break;
	case KEY_PAUSE:
		strcat(s, "Pause");
		break;
	case KEY_INSERT:
		strcat(s, "Insert");
		break;
	case KEY_DELETE:
		strcat(s, "Delete");
		break;
	case KEY_HOME:
		strcat(s, "Home");
		break;
	case KEY_END:
		strcat(s, "End");
		break;
	case KEY_PGUP:
		strcat(s, "PageUp");
		break;
	case KEY_PGDOWN:
		strcat(s, "PageDown");
		break;
	case KEY_LARROW:
		strcat(s, "Left");
		break;
	case KEY_RARROW:
		strcat(s, "Right");
		break;
	case KEY_UARROW:
		strcat(s, "Up");
		break;
	case KEY_DARROW:
		strcat(s, "Down");
		break;
	case KEY_NUMLOCK:
		strcat(s, "NumLock");
		break;
	case KEY_NUMDIV:
		strcat(s, "NumDiv");
		break;
	case KEY_NUMMULT:
		strcat(s, "NumMult");
		break;
	case KEY_NUMMINUS:
		strcat(s, "NumMinus");
		break;
	case KEY_NUMPLUS:
		strcat(s, "NumPlus");
		break;
	case KEY_NUMENTER:
		strcat(s, "NumEnter");
		break;
	case KEY_NUMDEL:
		strcat(s, "NumDelete");
		break;
	case KEY_NUMINS:
		strcat(s, "NumInsert");
		break;
	case KEY_NUMEND:
		strcat(s, "NumEnd");
		break;
	case KEY_NUMDOWN:
		strcat(s, "NumDown");
		break;
	case KEY_NUMPGDN:
		strcat(s, "NumPageDown");
		break;
	case KEY_NUMLEFT:
		strcat(s, "NumLeft");
		break;
	case KEY_NUMCLEAR:
		strcat(s, "NumClear");
		break;
	case KEY_NUMRIGHT:
		strcat(s, "NumRight");
		break;
	case KEY_NUMHOME:
		strcat(s, "NumHome");
		break;
	case KEY_NUMUP:
		strcat(s, "NumUp");
		break;
	case KEY_NUMPGUP:
		strcat(s, "NumPageUp");
		break;
	case KEY_NUMDEC:
		strcat(s, "NumDecimal");
		break;
	case KEY_NUM0:
		strcat(s, "Num0");
		break;
	case KEY_NUM1:
		strcat(s, "Num1");
		break;
	case KEY_NUM2:
		strcat(s, "Num2");
		break;
	case KEY_NUM3:
		strcat(s, "Num3");
		break;
	case KEY_NUM4:
		strcat(s, "Num4");
		break;
	case KEY_NUM5:
		strcat(s, "Num5");
		break;
	case KEY_NUM6:
		strcat(s, "Num6");
		break;
	case KEY_NUM7:
		strcat(s, "Num7");
		break;
	case KEY_NUM8:
		strcat(s, "Num8");
		break;
	case KEY_NUM9:
		strcat(s, "Num9");
		break;
	default:
		break;
	}

	return key_str;
}


#ifdef __linux__
/*
 * All the OS-specific keycodes are arranged in the same order in which the
 * corresponding kbm key codes are defined, so they can be accessed simply
 * by going to the index of the kbm code. As the kbm codes start at 0x01, the
 * first element in the array is empty.
 */
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
	XK_KP_Prior, XK_KP_Delete, XK_KP_Insert, XK_KP_End, XK_KP_Down,
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
/*
 * Give numpad Insert and Delete unassigned keycodes 0x88 and 0x89 and numpad
 * End through PageUp keycodes 0x97 through 0x9E (excluding NumClear which is
 * already mapped to 0x0C).
 */
static const uint32_t win_keycodes[] = {
	0x00, 0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4F, 0x50, 0x41,
	0x53, 0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x5A, 0x58, 0x43, 0x56,
	0x42, 0x4E, 0x4D, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x30, 0xC0, 0xBD, 0xBB, 0xDB, 0xDD, 0xDC, 0xBA, 0xDE, 0xBC, 0xBE, 0xBF,
	0x20, 0x1B, 0x08, 0x09, 0x14, 0x0D, 0x10, 0x11, 0x5B, 0x12, 0x70, 0x71,
	0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x2C, 0x91,
	0x13, 0x2D, 0x2E, 0x24, 0x23, 0x21, 0x22, 0x25, 0x27, 0x26, 0x28, 0x90,
	0x6F, 0x6A, 0x6D, 0x6B, 0x6C, 0x88, 0x89, 0x97, 0x98, 0x99, 0x9A, 0x0C,
	0x9B, 0x9C, 0x9D, 0x9E, 0x6E, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
	0x67, 0x68, 0x69
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
	0x00, kVK_ANSI_Q, kVK_ANSI_W, kVK_ANSI_E, kVK_ANSI_R, kVK_ANSI_T,
	kVK_ANSI_Y, kVK_ANSI_U, kVK_ANSI_I, kVK_ANSI_O, kVK_ANSI_P, kVK_ANSI_A,
	kVK_ANSI_S, kVK_ANSI_D, kVK_ANSI_F, kVK_ANSI_G, kVK_ANSI_H, kVK_ANSI_J,
	kVK_ANSI_K, kVK_ANSI_L, kVK_ANSI_Z, kVK_ANSI_X, kVK_ANSI_C, kVK_ANSI_V,
	kVK_ANSI_B, kVK_ANSI_N, kVK_ANSI_M, kVK_ANSI_1, kVK_ANSI_2, kVK_ANSI_3,
	kVK_ANSI_4, kVK_ANSI_5, kVK_ANSI_6, kVK_ANSI_7, kVK_ANSI_8, kVK_ANSI_9,
	kVK_ANSI_0, kVK_ANSI_Grave, kVK_ANSI_Minus, kVK_ANSI_Equal,
	kVK_ANSI_LeftBracket, kVK_ANSI_RightBracket, kVK_ANSI_Backslash,
	kVK_ANSI_Semicolon, kVK_ANSI_Quote, kVK_ANSI_Comma, kVK_ANSI_Period,
	kVK_ANSI_Slash, kVK_Space, kVK_Escape, kVK_Delete, kVK_Tab,
	kVK_CapsLock, kVK_Return, kVK_Shift, kVK_Control, kVK_Command,
	kVK_Option, kVK_F1, kVK_F2, kVK_F3, kVK_F4, kVK_F5, kVK_F6, kVK_F7,
	kVK_F8, kVK_F9, kVK_F10, kVK_F11, kVK_F12, kVK_F13, kVK_F14, kVK_F15,
	kVK_Help, kVK_ForwardDelete, kVK_Home, kVK_End, kVK_PageUp,
	kVK_PageDown, kVK_LeftArrow, kVK_RightArrow, kVK_DownArrow, kVK_UpArrow,
	kVK_ANSI_KeypadClear, kVK_ANSI_KeypadDivide, kVK_ANSI_KeypadMultiply,
	kVK_ANSI_KeypadMinus, kVK_ANSI_KeypadPlus, kVK_ANSI_KeypadEnter,
	kVK_ANSI_KeypadDecimal, kVK_ANSI_Keypad0, kVK_ANSI_Keypad1,
	kVK_ANSI_Keypad2, kVK_ANSI_Keypad3, kVK_ANSI_Keypad4, kVK_ANSI_Keypad5,
	kVK_ANSI_Keypad6, kVK_ANSI_Keypad7, kVK_ANSI_Keypad8, kVK_ANSI_Keypad9,
	kVK_ANSI_KeypadDecimal, kVK_ANSI_Keypad0, kVK_ANSI_Keypad1,
	kVK_ANSI_Keypad2, kVK_ANSI_Keypad3, kVK_ANSI_Keypad4, kVK_ANSI_Keypad5,
	kVK_ANSI_Keypad6, kVK_ANSI_Keypad7, kVK_ANSI_Keypad8, kVK_ANSI_Keypad9,
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
