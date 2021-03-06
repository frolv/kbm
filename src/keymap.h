/*
 * keymap.h
 * Copyright (C) 2016-2017 Alexei Frolov
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

#ifndef KBM_KEYMAP_H
#define KBM_KEYMAP_H

/*
 * Keycodes for all keys on a standard ANSI keyboard
 * and both functions of Numpad keys.
 */

/* alphabetic keys */
#define KEY_Q           0x01
#define KEY_W           0x02
#define KEY_E           0x03
#define KEY_R           0x04
#define KEY_T           0x05
#define KEY_Y           0x06
#define KEY_U           0x07
#define KEY_I           0x08
#define KEY_O           0x09
#define KEY_P           0x0A
#define KEY_A           0x0B
#define KEY_S           0x0C
#define KEY_D           0x0D
#define KEY_F           0x0E
#define KEY_G           0x0F
#define KEY_H           0x10
#define KEY_J           0x11
#define KEY_K           0x12
#define KEY_L           0x13
#define KEY_Z           0x14
#define KEY_X           0x15
#define KEY_C           0x16
#define KEY_V           0x17
#define KEY_B           0x18
#define KEY_N           0x19
#define KEY_M           0x1A

/* numeric keys */
#define KEY_0           0x1B
#define KEY_1           0x1C
#define KEY_2           0x1D
#define KEY_3           0x1E
#define KEY_4           0x1F
#define KEY_5           0x20
#define KEY_6           0x21
#define KEY_7           0x22
#define KEY_8           0x23
#define KEY_9           0x24

/* other keys */
#define KEY_BTICK       0x25
#define KEY_MINUS       0x26
#define KEY_EQUAL       0x27
#define KEY_LSQBR       0x28
#define KEY_RSQBR       0x29
#define KEY_BSLASH      0x2A
#define KEY_SEMIC       0x2B
#define KEY_QUOTE       0x2C
#define KEY_COMMA       0x2D
#define KEY_PERIOD      0x2E
#define KEY_FSLASH      0x2F
#define KEY_SPACE       0x30

/* modifiers and special keys */
#define KEY_ESCAPE      0x31
#define KEY_BSPACE      0x32
#define KEY_TAB	        0x33
#define KEY_CAPS        0x34
#define KEY_ENTER       0x35
#define KEY_SHIFT       0x36    /* don't bother distinguishing	*/
#define KEY_CTRL        0x37    /* between the left and right	*/
#define KEY_SUPER       0x38    /* versions of these keys;	*/
#define KEY_META        0x39    /* treat them identically	*/

/* f keys */
#define KEY_F1          0x3A
#define KEY_F2          0x3B
#define KEY_F3          0x3C
#define KEY_F4          0x3D
#define KEY_F5          0x3E
#define KEY_F6          0x3F
#define KEY_F7          0x40
#define KEY_F8          0x41
#define KEY_F9          0x42
#define KEY_F10         0x43
#define KEY_F11         0x44
#define KEY_F12         0x45

/* TKL keys */
#define KEY_PRTSCR      0x46
#define KEY_SCRLCK      0x47
#define KEY_PAUSE       0x48
#define KEY_INSERT      0x49
#define KEY_DELETE      0x4A
#define KEY_HOME        0x4B
#define KEY_END         0x4C
#define KEY_PGUP        0x4D
#define KEY_PGDOWN      0x4E
#define KEY_LARROW      0x4F
#define KEY_RARROW      0x50
#define KEY_UARROW      0x51
#define KEY_DARROW      0x52

/* numpad keys */
#define KEY_NUMLOCK     0x53
#define KEY_NUMDIV      0x54
#define KEY_NUMMULT     0x55
#define KEY_NUMMINUS    0x56
#define KEY_NUMPLUS     0x57
#define KEY_NUMENTER    0x58
/* num lock off */
#define KEY_NUMDEL      0x59
#define KEY_NUMINS      0x5A
#define KEY_NUMEND      0x5B
#define KEY_NUMDOWN     0x5C
#define KEY_NUMPGDN     0x5D
#define KEY_NUMLEFT     0x5E
#define KEY_NUMCLEAR    0x5F
#define KEY_NUMRIGHT    0x60
#define KEY_NUMHOME     0x61
#define KEY_NUMUP       0x62
#define KEY_NUMPGUP     0x63
/* num lock on */
#define KEY_NUMDEC      0x64
#define KEY_NUM0        0x65
#define KEY_NUM1        0x66
#define KEY_NUM2        0x67
#define KEY_NUM3        0x68
#define KEY_NUM4        0x69
#define KEY_NUM5        0x6A
#define KEY_NUM6        0x6B
#define KEY_NUM7        0x6C
#define KEY_NUM8        0x6D
#define KEY_NUM9        0x6E


/* bitmasks for the various modifier keys */
#define KBM_SHIFT_MASK  0x01
#define KBM_CTRL_MASK   0x02
#define KBM_SUPER_MASK  0x04    /* command on OS X */
#define KBM_META_MASK   0x08    /* option  on OS X */

/* check if kc is the keycode of a modifier */
#define K_ISMOD(kc) \
	((kc) == KEY_CTRL || (kc) == KEY_SHIFT || \
	(kc) == KEY_SUPER || (kc) == KEY_META)

/* convert kbm codes and modmasks to OS-specific ones */
#ifdef __linux__
#define OSCODE(x) kbm_to_keysym(x)
#define OSMASK(x) kbm_to_xcb_masks(x)
#endif
#if defined(__CYGWIN__) || defined (__MINGW32__)
#define OSCODE(x) kbm_to_win32(x)
#define OSMASK(x) kbm_to_win_masks(x)
#endif
#ifdef __APPLE__
#define OSCODE(x) kbm_to_carbon(x)
#define OSMASK(x) kbm_to_osx_masks(x)
#endif

#include <stdint.h>

void keymap_init(void);
void keymap_free(void);

/* keystr: return a string representation of key corresponding to keycode */
char *keystr(uint8_t keycode, uint8_t mask);

/* lookup_keycode: return the kbm keycode of key */
uint32_t lookup_keycode(const char *key);


/*
 * Functions to convert kbm keycodes and modifier masks to
 * their OS-specific versions.
 */

#ifdef __linux__
#include <X11/keysym.h>
#include <xcb/xcb.h>

unsigned int kbm_to_keysym(uint8_t keycode);
unsigned int kbm_to_xcb_masks(uint8_t modmask);
#endif

#if defined(__CYGWIN__) || defined (__MINGW32__)
#include <Windows.h>

unsigned int kbm_to_win32(uint8_t keycode);
unsigned int kbm_to_win_masks(uint8_t modmask);
#endif

#ifdef __APPLE__
#include <Carbon/Carbon.h>

unsigned int kbm_to_carbon(uint8_t keycode);
unsigned int kbm_to_osx_masks(uint8_t modmask);
#endif

#endif /* KBM_KEYMAP_H */
