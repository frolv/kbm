/*
 * hotkey.h
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

#ifndef KBM_HOTKEY_H
#define KBM_HOTKEY_H

#include <stdint.h>
#include "keymap.h"

/* operations that can be performed */
#define OP_CLICK	0xA0
#define OP_RCLICK	0xA1
#define OP_JUMP		0xA2
#define OP_KEY		0xA3
#define OP_TOGGLE	0xA4
#define OP_QUIT		0xA5
#define OP_EXEC		0xA6

/* keypress and key release */
#define KBM_PRESS	0x00
#define KBM_RELEASE	0x01

/* additional flags */
#define KBM_NOREPEAT	0x01

struct hotkey {
	uint8_t		kbm_code;	/* kbm keycode of the hotkey */
	uint8_t		kbm_modmask;	/* kbm modifier masks */
	uint32_t	os_code;	/* os-specific keycode of the hotkey */
	uint32_t	os_modmask;	/* os-specific modifier masks */
	uint8_t		op;		/* operation to perform on keypress */
	uint64_t	opargs;		/* arguments for the operation */
	uint32_t	key_flags;	/* extra hotkey flags */
	struct hotkey	*next;		/* next key binding */
};

#define KBM_ACTIVEWIN   0x01    /* only run hotkeys in specified windows */

struct keymap {
	int flags;              /* global flags */
	char **windows;         /* titles of windows in which keys are active */
	size_t win_len;         /* number of windows in which keys are active */
	size_t win_size;        /* allocated size of windows array */
	struct hotkey *keys;    /* list of mapped keys */
};

/* create_hotkey: define a new hotkey */
struct hotkey *create_hotkey(uint8_t keycode, uint8_t mods,
                             uint8_t op, uint64_t opargs, uint32_t flags);

/* add_hotkey: append hotkey hk to the end of list head */
void add_hotkey(struct hotkey **head, struct hotkey *hk);

/* free_keys: free all hotkeys in list head */
void free_keys(struct hotkey *head);

/* process_hotkey: perform the operation of hotkey hk */
int process_hotkey(struct hotkey *hk, unsigned int type);

void free_windows(struct keymap *k);

#endif /* KBM_HOTKEY_H */
