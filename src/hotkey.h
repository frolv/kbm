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
#define OP_SUSPEND	0xA3
#define OP_QUIT		0xA4

struct hotkey {
	uint8_t kbm_code;	/* the kbm keycode of the hotkey */
	uint8_t kbm_modmask;	/* kbm modifier masks */
	uint32_t os_code;	/* the os-specific keycode of the hotkey */
	uint32_t os_modmask;	/* os-specific modifier masks */
	uint8_t op;		/* the operation to perform when key pressed */
	uint64_t opargs;	/* arguments for the operation */
	struct hotkey *next;	/* next key binding */
};

struct hotkey *create_hotkey(uint8_t keycode, uint8_t mods,
		uint8_t op, uint64_t opargs);

int process_hotkey(struct hotkey *hk);

#endif /* KBM_HOTKEY_H */
