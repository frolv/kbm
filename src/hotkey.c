/*
 * hotkey.c
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

#include <stdlib.h>
#include "display.h"
#include "hotkey.h"
#include "kbm.h"
#include "keymap.h"

static void get_os_codes(struct hotkey *hk);

/* create_hotkey: define a new hotkey */
struct hotkey *create_hotkey(uint8_t keycode, uint8_t modmask,
                             uint8_t op, uint64_t opargs, uint32_t flags)
{
	struct hotkey *hk;

	hk = malloc(sizeof *hk);
	hk->kbm_code = keycode;
	hk->kbm_modmask = modmask;
	hk->op = op;
	hk->opargs = opargs;
	hk->key_flags = flags;
	hk->next = NULL;
	get_os_codes(hk);

	return hk;
}

/* add_hotkey: append a key to the end of list head */
void add_hotkey(struct hotkey **head, struct hotkey *hk)
{
	while (*head)
		head = &(*head)->next;
	*head = hk;
}

/* free_keys: free all hotkeys and their data in list starting at head */
void free_keys(struct hotkey *head)
{
#if defined(__linux__) || defined(__APPLE__)
	char **argv;
#endif

	if (head->next)
		free_keys(head->next);
	/*
	 * For an exec operation, opargs stores the
	 * address of some dynamically allocated data.
	 */
	if (head->op == OP_EXEC) {
#if defined(__linux__) || defined(__APPLE__)
		argv = (char **)head->opargs;

#ifdef __APPLE__
		/* skip over "open" "-a", which are not dynamically allocated */
		argv += 2;
#endif

		for (; *argv; ++argv)
			free(*argv);
#endif
		free((void *)head->opargs);
	}
	free(head);
}

/* process_hotkey: perform the operation of hotkey hk */
int process_hotkey(struct hotkey *hk, unsigned int type)
{
	int x, y;
#if defined(KBM_DEBUG) && defined(__linux__) || defined(__APPLE__)
	char **s;
#endif

	if (type == KBM_RELEASE) {
		/* send a release event for a key mapping on key release */
		if (hk->op == OP_KEY) {
			x = hk->opargs & 0xFFFFFFFF;
			y = (hk->opargs >> 32) & 0xFFFFFFFF;
			send_key(OSCODE(x), OSMASK(y), type);
		}
		return 0;
	}

	PRINT_DEBUG("KEYPRESS:  %s\n", keystr(hk->kbm_code, hk->kbm_modmask));
	switch (hk->op) {
	case OP_CLICK:
		/* click operation: send a mouse click event */
		PRINT_DEBUG("OPERATION: click\n");
		send_button(KBM_BUTTON_LEFT);
		return 0;
	case OP_RCLICK:
		/* rclick operation: send a mouse right click event */
		PRINT_DEBUG("OPERATION: rclick\n");
		send_button(KBM_BUTTON_RIGHT);
		return 0;
	case OP_JUMP:
		/* jump operation: move the cursor */
		/* x value is stored in lower 32 bits, y value in upper 32 */
		x = hk->opargs & 0xFFFFFFFF;
		y = (hk->opargs >> 32) & 0xFFFFFFFF;
		PRINT_DEBUG("OPERATION: jump %d %d\n", x, y);
		move_cursor(x, y);
		return 0;
	case OP_KEY:
		/* key operation: simulate a keypress */
		/* keycode is stored in lower 32 bits, modmask in upper 32 */
		x = hk->opargs & 0xFFFFFFFF;
		y = (hk->opargs >> 32) & 0xFFFFFFFF;
		PRINT_DEBUG("OPERATION: key %s\n", keystr(x, y));
		send_key(OSCODE(x), OSMASK(y), type);
		return 0;
	case OP_TOGGLE:
		/* toggle operation: enable/disable hotkeys */
		PRINT_DEBUG("OPERATION: toggle\n");
		toggle_keys();
		return 0;
	case OP_QUIT:
		/* exit operation: quit the program */
		PRINT_DEBUG("OPERATION: quit\n");
		return -1;
	case OP_EXEC:
		/* exec operation: execute command or program */
		PRINT_DEBUG("OPERATION: exec");
#if defined(KBM_DEBUG) && defined(__linux__) || defined(__APPLE__)
		/*
		 * On Unix-based systems, opargs is the start of the
		 * argv array of the command to be executed.
		 */
		s = (char **)hk->opargs;
		for (; *s; ++s)
			PRINT_DEBUG(" %s", *s);
		putchar('\n');
#endif
#if defined(__CYGWIN__) || defined (__MINGW32__)
		/*
		 * On Windows, opargs is the address of the string
		 * detailing the command to execute.
		 */
		PRINT_DEBUG(" %s\n", (char *)hk->opargs);
#endif
		kbm_exec((void *)hk->opargs);
		return 0;
	default:
		return 0;
	}
}

void free_windows(struct keymap *k)
{
	char **s;

	if (k->win_size) {
		for (s = k->windows; *s; ++s)
			free(*s);
		free(k->windows);
	}
	k->win_size = 0;
	k->win_len = 0;
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
