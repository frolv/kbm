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

static void get_os_codes(struct hotkey *hk);

struct hotkey *create_hotkey(uint8_t keycode, uint8_t modmask,
		uint8_t op, uint64_t opargs)
{
	struct hotkey *hk;

	hk = malloc(sizeof(*hk));
	hk->kbm_code = keycode;
	hk->kbm_modmask = modmask;
	hk->op = op;
	hk->opargs = opargs;
	hk->next = NULL;
	get_os_codes(hk);

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

int process_hotkey(struct hotkey *hk, unsigned int type)
{
	int x, y;

	if (type == KBM_RELEASE) {
		/* send a release event for a key mapping on key release */
		if (hk->op == OP_KEY) {
			x = hk->opargs & 0xFFFFFFFF;
			y = (hk->opargs >> 32) & 0xFFFFFFFF;
			send_key(OSCODE(x), OSMASK(y), type);
		}
		return 0;
	}

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
	case OP_JUMP:
		/* jump operation: move the cursor */
		/* x value is stored in lower 32 bits, y value in upper 32 */
		x = hk->opargs & 0xFFFFFFFF;
		y = (hk->opargs >> 32) & 0xFFFFFFFF;
		printf("OPERATION: jump %d %d\n", x, y);
		move_cursor(x, y);
		return 0;
	case OP_KEY:
		/* key operation: simulate a keypress */
		/* keycode is stored in lower 32 bits, modmask in upper 32 */
		x = hk->opargs & 0xFFFFFFFF;
		y = (hk->opargs >> 32) & 0xFFFFFFFF;
		printf("OPERATION: key %s\n", keystr(x, y));
		send_key(OSCODE(x), OSMASK(y), type);
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
	hk->os_code = OSCODE(hk->kbm_code);
	hk->os_modmask = OSMASK(hk->kbm_modmask);

#ifdef __linux__
	/*
	 * The keys NUMDEC through NUM9 are only accessible when Num Lock is
	 * on. Set the Num Lock bit to active to indicate this.
	 */
	if (hk->kbm_code >= KEY_NUMDEC)
		hk->os_modmask |= XCB_MOD_MASK_2;
#endif
}
