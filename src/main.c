/*
 * $NAME - $DESC
 * main.c
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

#include "kbm.h"
#include "display.h"
#include "hotkey.h"

#if defined(__linux__) || defined(__APPLE__)
#include <getopt.h>
#endif

/* temp */
static void add_key(struct hotkey **head, struct hotkey **tail, struct hotkey *hk);

int main(int argc, char **argv)
{
#if defined(__linux__) || defined(__APPLE__)
	int c;
	static struct option long_opts[] = {
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	while ((c = getopt_long(argc, argv, "h", long_opts, NULL)) != EOF) {
		switch (c) {
		case 'h':
			printf("usage: %s FILE\n", PROGRAM_NAME);
			return 0;
		default:
			fprintf(stderr, "usage: %s FILE\n", argv[0]);
			return 1;
		}
	}
#endif
	/* temp for testing purposes */
	struct hotkey *head, *tail;
	head = tail = NULL;
	add_key(&head, &tail, create_hotkey(KEY_Q, 0, OP_RCLICK, 0));
	add_key(&head, &tail, create_hotkey(KEY_W, 0, OP_JUMP, 0));
	add_key(&head, &tail, create_hotkey(KEY_E, 0, OP_CLICK, 0));
	add_key(&head, &tail, create_hotkey(KEY_Q, KBM_SHIFT_MASK, OP_TOGGLE, 0));
	add_key(&head, &tail, create_hotkey(KEY_Q, KBM_CTRL_MASK | KBM_SHIFT_MASK, OP_QUIT, 0));

	init_display(head);
	start_loop();
	close_display();
	return 0;
}

/* temp */
static void add_key(struct hotkey **head, struct hotkey **tail, struct hotkey *hk)
{
	if (!*tail) {
		*head = *tail = hk;
		return;
	}
	(*tail)->next = hk;
	*tail = hk;
}
