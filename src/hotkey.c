/*
 * hotkey.c
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

#include <stdlib.h>
#include "display.h"
#include "hotkey.h"
#include "kbm.h"
#include "keymap.h"

static void get_os_codes(struct hotkey *hk);

struct hotkey *create_hotkey(uint8_t keycode, uint8_t modmask,
		uint8_t op, uint64_t opargs)
{
	struct hotkey *hk;

#if defined(__CYGWIN__) || defined (__MINGW32__)
	/*
	 * If $KEY and $MODS+$KEY are registered at the
	 * same time, it will fail if both have the same ID.
	 * Each registered key is given a unique ID to prevent this.
	 */
	static int id = 0;
#endif

	hk = malloc(sizeof(*hk));
	hk->kbm_code = keycode;
	hk->kbm_modmask = modmask;
	hk->op = op;
	hk->opargs = opargs;
	hk->next = NULL;
	get_os_codes(hk);

#if defined(__CYGWIN__) || defined (__MINGW32__)
	hk->id = ++id;
#endif

	return hk;
}

void add_hotkey(struct hotkey **head, struct hotkey *hk)
{
	while (*head)
		head = &(*head)->next;
	*head = hk;
}

void free_keys(struct hotkey *head)
{
	if (head->next)
		free_keys(head->next);
	free(head);
}

int process_hotkey(struct hotkey *hk)
{
	printf("KEYPRESS: %s\n", keystr(hk->kbm_code, hk->kbm_modmask));
	switch (hk->op) {
	case OP_CLICK:
		/* click operation: send a mouse click event */
		printf("OPERATION: click\n");
		send_button(KBM_BUTTON_LEFT);
		return 0;
	case OP_RCLICK:
		/* rclick operation: send a mouse right click event */
		printf("OPERATION: rclick\n");
		send_button(KBM_BUTTON_RIGHT);
		return 0;
	case OP_TOGGLE:
		/* toggle operation: enable/disable hotkeys */
		printf("OPERATION: toggle\n");
		toggle_keys();
		return 0;
	case OP_QUIT:
		/* exit operation: quit the program */
		printf("OPERATION: quit\n");
		return -1;
	default:
		return 0;
	}
}

/* get_os_codes: load os-specific keycodes and mod masks into hk */
static void get_os_codes(struct hotkey *hk)
{
#ifdef __linux__
	hk->os_code = kbm_to_keysym(hk->kbm_code);
	hk->os_modmask = kbm_to_xcb_masks(hk->kbm_modmask);
#endif

#if defined(__CYGWIN__) || defined (__MINGW32__)
	hk->os_code = kbm_to_win32(hk->kbm_code);
	hk->os_modmask = kbm_to_win_masks(hk->kbm_modmask);
#endif

#ifdef __APPLE__
	hk->os_code = kbm_to_carbon(hk->kbm_code);
	hk->os_modmask = kbm_to_osx_masks(hk->kbm_modmask);
#endif
}
