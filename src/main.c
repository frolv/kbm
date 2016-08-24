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

#include <getopt.h>
#include "kbm.h"
#include "display.h"
#include "hotkey.h"

int main(int argc, char **argv)
{
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
	/* temp for testing purposes */
	struct hotkey *head = NULL;
	uint64_t jump = 55;
	uint32_t *y = ((uint32_t *)&jump) + 1;
	*y = 210;

	add_hotkey(&head, create_hotkey(KEY_Q, 0, OP_RCLICK, 0));
	add_hotkey(&head, create_hotkey(KEY_W, 0, OP_JUMP, jump));
	add_hotkey(&head, create_hotkey(KEY_E, 0, OP_CLICK, 0));
	add_hotkey(&head, create_hotkey(KEY_Q, KBM_SHIFT_MASK, OP_TOGGLE, 0));
	add_hotkey(&head, create_hotkey(KEY_E, KBM_CTRL_MASK, OP_QUIT, 0));

	init_display();
	load_keys(head);
	start_loop();
	unload_keys();
	close_display();
	return 0;
}
