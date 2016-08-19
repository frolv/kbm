/*
 * keymap.h
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

#ifndef _KEYMAP_H
#define _KEYMAP_H

/* kbm keycodes */

/* alphabetic keys */
#define KEY_Q 0x01
#define KEY_W 0x02
#define KEY_E 0x03
#define KEY_R 0x04
#define KEY_T 0x05
#define KEY_Y 0x06
#define KEY_U 0x07
#define KEY_I 0x08
#define KEY_O 0x09
#define KEY_P 0x0A
#define KEY_A 0x0B
#define KEY_S 0x0C
#define KEY_D 0x0D
#define KEY_F 0x0E
#define KEY_G 0x0F
#define KEY_H 0x10
#define KEY_J 0x11
#define KEY_K 0x12
#define KEY_L 0x13
#define KEY_Z 0x14
#define KEY_X 0x15
#define KEY_C 0x16
#define KEY_V 0x17
#define KEY_B 0x18
#define KEY_N 0x19
#define KEY_M 0x1A

/* numeric keys */
#define KEY_1 0x1B
#define KEY_2 0x1C
#define KEY_3 0x1D
#define KEY_4 0x1E
#define KEY_5 0x1F
#define KEY_6 0x20
#define KEY_7 0x21
#define KEY_8 0x22
#define KEY_9 0x23
#define KEY_0 0x24

/* other keys */
#define KEY_BTICK	0x25
#define KEY_MINUS	0x26
#define KEY_EQUAL	0x27
#define KEY_LSQBR	0x28
#define KEY_RSQBR	0x29
#define KEY_BSLASH	0x2A
#define KEY_SEMIC	0x2B
#define KEY_QUOTE	0x2C
#define KEY_COMMA	0x2D
#define KEY_PERIOD	0x2E
#define KEY_FSLASH	0x2F
#define KEY_SPACE	0x30

/* modifiers and special keys */
#define KEY_ESCAPE	0x31
#define KEY_BSPACE	0x32
#define KEY_TAB		0x33
#define KEY_CAPS	0x34
#define KEY_ENTER	0x35
#define KEY_SHIFT	0x36	/* don't bother distinguishing	*/
#define KEY_CTRL	0x37	/* between the left and right	*/
#define KEY_SUPER	0x38	/* versions of these keys;	*/
#define KEY_META	0x39	/* treat them identically	*/


/* keystr: return a string representation of key corresponding to keycode */
char *keystr(unsigned int keycode);


/*
 * functions to convert kbm keycodes and mod
 * masks to their OS-specific versions
 */
#include <stdint.h>

#ifdef __linux__
#include <X11/keysym.h>

unsigned int kbm_to_keysym(uint32_t keycode);
unsigned int kbm_to_xcb_masks(uint32_t modmask);
#endif

#if defined(__CYGWIN__) || defined (__MINGW32__)
#include <Windows.h>

unsigned int kbm_to_win32(uint32_t keycode);
unsigned int kbm_to_win_masks(uint32_t modmask);
#endif

#ifdef __APPLE__
#include <Carbon/Carbon.h>

unsigned int kbm_to_carbon(uint32_t keycode);
#endif

#endif
