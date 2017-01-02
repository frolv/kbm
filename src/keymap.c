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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "kbm.h"
#include "keymap.h"
#include "uthash.h"

struct skey {
	uint32_t	keycode;	/* the key's kbm keycode */
	const char	*keystr;	/* lexeme representing the key */
	const char	*keyname;	/* the proper name of the key */
	UT_hash_handle	hh;
};

static char key_str[64];
struct skey *keymap = NULL;

static void add_key(uint32_t kc, const char *str, const char *name);

/* keymap_init: populate the key hash table */
void keymap_init(void)
{
	/* macros are good */
	add_key(KEY_Q, "q", "Q");
	add_key(KEY_W, "w", "W");
	add_key(KEY_E, "e", "E");
	add_key(KEY_R, "r", "R");
	add_key(KEY_T, "t", "T");
	add_key(KEY_Y, "y", "Y");
	add_key(KEY_U, "u", "U");
	add_key(KEY_I, "i", "I");
	add_key(KEY_O, "o", "O");
	add_key(KEY_P, "p", "P");
	add_key(KEY_A, "a", "A");
	add_key(KEY_S, "s", "S");
	add_key(KEY_D, "d", "D");
	add_key(KEY_F, "f", "F");
	add_key(KEY_G, "g", "G");
	add_key(KEY_H, "h", "H");
	add_key(KEY_J, "j", "J");
	add_key(KEY_K, "k", "K");
	add_key(KEY_L, "l", "L");
	add_key(KEY_Z, "z", "Z");
	add_key(KEY_X, "x", "X");
	add_key(KEY_C, "c", "C");
	add_key(KEY_V, "v", "V");
	add_key(KEY_B, "b", "B");
	add_key(KEY_N, "n", "N");
	add_key(KEY_M, "m", "M");
	add_key(KEY_0, "zero",	"0");
	add_key(KEY_1, "one",	"1");
	add_key(KEY_2, "two",	"2");
	add_key(KEY_3, "three",	"3");
	add_key(KEY_4, "four",	"4");
	add_key(KEY_5, "five",	"5");
	add_key(KEY_6, "six",	"6");
	add_key(KEY_7, "seven",	"7");
	add_key(KEY_8, "eight",	"8");
	add_key(KEY_9, "nine",	"9");
	add_key(KEY_BTICK,      "backtick", "`");
	add_key(KEY_BTICK,      "grave", "`");
	add_key(KEY_MINUS,      "minus", "-");
	add_key(KEY_MINUS,      "dash", "-");
	add_key(KEY_EQUAL,      "equals", "=");
	add_key(KEY_LSQBR,      "leftbracket", "[");
	add_key(KEY_LSQBR,      "leftsq", "[");
	add_key(KEY_LSQBR,      "leftsquare", "[");
	add_key(KEY_RSQBR,      "rightbracket", "]");
	add_key(KEY_RSQBR,      "rightsq", "]");
	add_key(KEY_RSQBR,      "rightsquare", "]");
	add_key(KEY_BSLASH,     "backslash", "\\");
	add_key(KEY_SEMIC,      "semicolon", ";");
	add_key(KEY_QUOTE,      "quote", "'");
	add_key(KEY_QUOTE,      "apostrophe", "'");
	add_key(KEY_COMMA,      "comma", ",");
	add_key(KEY_PERIOD,     "period", ".");
	add_key(KEY_PERIOD,     "dot", ".");
	add_key(KEY_FSLASH,     "slash", "/");
	add_key(KEY_SPACE,      "space", "Space");
	add_key(KEY_ESCAPE,     "esc", "Escape");
	add_key(KEY_ESCAPE,     "escape", "Escape");
	add_key(KEY_BSPACE,     "backspace", "Backspace");
	add_key(KEY_TAB,        "tab", "Tab");
	add_key(KEY_CAPS,       "caps", "CapsLock");
	add_key(KEY_CAPS,       "capslock", "CapsLock");
	add_key(KEY_ENTER,      "enter", "Enter");
	add_key(KEY_ENTER,      "return", "Enter");
	add_key(KEY_SHIFT,      "shift", "Shift");
	add_key(KEY_CTRL,       "control", "Control");
	add_key(KEY_CTRL,       "ctrl", "Control");
	add_key(KEY_SUPER,      "super", "Super");
	add_key(KEY_SUPER,      "command", "Super");
	add_key(KEY_SUPER,      "cmd", "Super");
	add_key(KEY_SUPER,      "win", "Super");
	add_key(KEY_SUPER,      "windows", "Super");
	add_key(KEY_META,       "meta", "Meta");
	add_key(KEY_META,       "alt", "Meta");
	add_key(KEY_META,       "option", "Meta");
	add_key(KEY_F1,         "f1",  "F1");
	add_key(KEY_F2,         "f2",  "F2");
	add_key(KEY_F3,         "f3",  "F3");
	add_key(KEY_F4,         "f4",  "F4");
	add_key(KEY_F5,         "f5",  "F5");
	add_key(KEY_F6,         "f6",  "F6");
	add_key(KEY_F7,         "f7",  "F7");
	add_key(KEY_F8,         "f8",  "F8");
	add_key(KEY_F9,         "f9",  "F9");
	add_key(KEY_F10,        "f10", "F10");
	add_key(KEY_F11,        "f11", "F11");
	add_key(KEY_F12,        "f12", "F12");
	add_key(KEY_PRTSCR,     "printscreen", "PrintScreen");
	add_key(KEY_SCRLCK,     "scrolllock", "ScrollLock");
	add_key(KEY_PAUSE,      "pause", "Pause");
	add_key(KEY_INSERT,     "insert", "Insert");
	add_key(KEY_INSERT,     "ins", "Insert");
	add_key(KEY_DELETE,     "delete", "Delete");
	add_key(KEY_DELETE,     "del", "Delete");
	add_key(KEY_HOME,       "home", "Home");
	add_key(KEY_END,        "end", "End");
	add_key(KEY_PGUP,       "pageup", "PageUp");
	add_key(KEY_PGUP,       "pgup", "PageUp");
	add_key(KEY_PGDOWN,     "pagedown", "PageDown");
	add_key(KEY_PGDOWN,     "pgdn", "PageDown");
	add_key(KEY_LARROW,     "left", "Left");
	add_key(KEY_RARROW,     "right", "Right");
	add_key(KEY_UARROW,     "up", "Up");
	add_key(KEY_DARROW,     "down", "Down");
	add_key(KEY_NUMLOCK,    "numlock", "NumLock");
	add_key(KEY_NUMDIV,     "numdiv", "NumDiv");
	add_key(KEY_NUMDIV,     "numdivide", "NumDiv");
	add_key(KEY_NUMDIV,     "numslash", "NumDiv");
	add_key(KEY_NUMMULT,    "nummult", "NumMult");
	add_key(KEY_NUMMULT,    "nummultiply", "NumMult");
	add_key(KEY_NUMMULT,    "numasterisk", "NumMult");
	add_key(KEY_NUMMULT,    "numtimes", "NumMult");
	add_key(KEY_NUMMINUS,   "numminus", "NumMinus");
	add_key(KEY_NUMPLUS,    "numplus", "NumPlus");
	add_key(KEY_NUMENTER,   "numenter", "NumEnter");
	add_key(KEY_NUMDEL,     "numdel", "NumDel");
	add_key(KEY_NUMDEL,     "numdelete", "NumDel");
	add_key(KEY_NUMINS,     "numins", "NumIns");
	add_key(KEY_NUMINS,     "numinsert", "NumIns");
	add_key(KEY_NUMEND,     "numend", "NumEnd");
	add_key(KEY_NUMDOWN,    "numdown", "NumDown");
	add_key(KEY_NUMPGDN,    "numpgdn", "NumPageDown");
	add_key(KEY_NUMPGDN,    "numpagedown", "NumPageDown");
	add_key(KEY_NUMLEFT,    "numleft", "NumLeft");
	add_key(KEY_NUMCLEAR,   "numclear", "NumClear");
	add_key(KEY_NUMRIGHT,   "numright", "NumRight");
	add_key(KEY_NUMHOME,    "numhome", "NumHome");
	add_key(KEY_NUMUP,      "numup", "NumUp");
	add_key(KEY_NUMPGUP,    "numpgup", "NumPageUp");
	add_key(KEY_NUMPGUP,    "numpageup", "NumPageUp");
	add_key(KEY_NUMDEC,     "numdecimal", "NumDecimal");
	add_key(KEY_NUMDEC,     "numdec", "NumDecimal");
	add_key(KEY_NUM0,       "num0", "Num0");
	add_key(KEY_NUM1,       "num1", "Num1");
	add_key(KEY_NUM2,       "num2", "Num2");
	add_key(KEY_NUM3,       "num3", "Num3");
	add_key(KEY_NUM4,       "num4", "Num4");
	add_key(KEY_NUM5,       "num5", "Num5");
	add_key(KEY_NUM6,       "num6", "Num6");
	add_key(KEY_NUM7,       "num7", "Num7");
	add_key(KEY_NUM8,       "num8", "Num8");
	add_key(KEY_NUM9,       "num9", "Num9");
}

/* keymap_free: free the key hash table */
void keymap_free(void)
{
	struct skey *k, *tmp;

	HASH_ITER(hh, keymap, k, tmp) {
		HASH_DEL(keymap, k);
		free(k);
	}
}

/* keystr: return a string representation of key corresponding to keycode */
char *keystr(uint8_t keycode, uint8_t mask)
{
	struct skey *k, *tmp;

	key_str[0] = '\0';

	if (CHECK_MASK(mask, KBM_CTRL_MASK))
		strcat(key_str, "Control-");
	if (CHECK_MASK(mask, KBM_SUPER_MASK))
		strcat(key_str, "Super-");
	if (CHECK_MASK(mask, KBM_META_MASK))
		strcat(key_str, "Meta-");
	if (CHECK_MASK(mask, KBM_SHIFT_MASK))
		strcat(key_str, "Shift-");

	HASH_ITER(hh, keymap, k, tmp) {
		if (k->keycode == keycode) {
			strcat(key_str, k->keyname);
			break;
		}
	}

	return key_str;
}

/* lookup_keycode: find a keycode from a string representation */
uint32_t lookup_keycode(const char *key)
{
	struct skey *k;
	char buf[BUFFER_SIZE];
	char *s;

	strcpy(buf, key);
	for (s = buf; *s; ++s)
		*s = tolower(*s);
	HASH_FIND_STR(keymap, buf, k);
	return k ? k->keycode : 0;
}

/* add_key: create a skey struct and add it to the hash table */
static void add_key(uint32_t kc, const char *str, const char *name)
{
	struct skey *k;

	k = malloc(sizeof *k);
	k->keycode = kc;
	k->keystr = str;
	k->keyname = name;
	HASH_ADD_KEYPTR(hh, keymap, k->keystr, strlen(k->keystr), k);
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
	XK_b, XK_n, XK_m, XK_0, XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8,
	XK_9, XK_grave, XK_minus, XK_equal, XK_bracketleft, XK_bracketright,
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

	if (CHECK_MASK(modmask, KBM_SHIFT_MASK))
		mask |= XCB_MOD_MASK_SHIFT;
	if (CHECK_MASK(modmask, KBM_CTRL_MASK))
		mask |= XCB_MOD_MASK_CONTROL;
	if (CHECK_MASK(modmask, KBM_SUPER_MASK))
		mask |= XCB_MOD_MASK_4;
	if (CHECK_MASK(modmask, KBM_META_MASK))
		mask |= XCB_MOD_MASK_1;

	return mask;
}
#endif

#if defined(__CYGWIN__) || defined (__MINGW32__)
/*
 * Give Numpad Insert and Delete unassigned keycodes 0x88 and 0x89 and Numpad
 * End through PageUp keycodes 0x97 through 0x9E (excluding NumClear which is
 * already mapped to 0x0C).
 */
static const uint32_t win_keycodes[] = {
	0x00, 0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4F, 0x50, 0x41,
	0x53, 0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x5A, 0x58, 0x43, 0x56,
	0x42, 0x4E, 0x4D, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
	0x39, 0xC0, 0xBD, 0xBB, 0xDB, 0xDD, 0xDC, 0xBA, 0xDE, 0xBC, 0xBE, 0xBF,
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

	if (CHECK_MASK(modmask, KBM_SHIFT_MASK))
		mask |= MOD_SHIFT;
	if (CHECK_MASK(modmask, KBM_CTRL_MASK))
		mask |= MOD_CONTROL;
	if (CHECK_MASK(modmask, KBM_SUPER_MASK))
		mask |= MOD_WIN;
	if (CHECK_MASK(modmask, KBM_META_MASK))
		mask |= MOD_ALT;

	return mask;
}
#endif

#ifdef __APPLE__
/*
 * As there is no Num Lock on OS X, the Numpad keys each only have
 * a single function.
 */
static const uint32_t osx_keycodes[] = {
	0x00, kVK_ANSI_Q, kVK_ANSI_W, kVK_ANSI_E, kVK_ANSI_R, kVK_ANSI_T,
	kVK_ANSI_Y, kVK_ANSI_U, kVK_ANSI_I, kVK_ANSI_O, kVK_ANSI_P, kVK_ANSI_A,
	kVK_ANSI_S, kVK_ANSI_D, kVK_ANSI_F, kVK_ANSI_G, kVK_ANSI_H, kVK_ANSI_J,
	kVK_ANSI_K, kVK_ANSI_L, kVK_ANSI_Z, kVK_ANSI_X, kVK_ANSI_C, kVK_ANSI_V,
	kVK_ANSI_B, kVK_ANSI_N, kVK_ANSI_M, kVK_ANSI_0, kVK_ANSI_1, kVK_ANSI_2,
	kVK_ANSI_3, kVK_ANSI_4, kVK_ANSI_5, kVK_ANSI_6, kVK_ANSI_7, kVK_ANSI_8,
	kVK_ANSI_9, kVK_ANSI_Grave, kVK_ANSI_Minus, kVK_ANSI_Equal,
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

	if (CHECK_MASK(modmask, KBM_SHIFT_MASK))
		mask |= kCGEventFlagMaskShift;
	if (CHECK_MASK(modmask, KBM_CTRL_MASK))
		mask |= kCGEventFlagMaskControl;
	if (CHECK_MASK(modmask, KBM_SUPER_MASK))
		mask |= kCGEventFlagMaskCommand;
	if (CHECK_MASK(modmask, KBM_META_MASK))
		mask |= kCGEventFlagMaskAlternate;

	return mask;
}
#endif
