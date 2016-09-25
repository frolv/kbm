/*
 * kbm - a simple hotkey mapper
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
#include <string.h>
#include "kbm.h"
#include "display.h"
#include "hotkey.h"
#include "parser.h"

void print_help(void);

int main(int argc, char **argv)
{
	int c, enable, notify;
	struct hotkey *head;

	static struct option long_opts[] = {
		{ "disable", no_argument, 0, 'd' },
		{ "help", no_argument, 0, 'h' },
		{ "notifications", no_argument, 0, 'n' },
		{ "version", no_argument, 0, 'v' },
		{ 0, 0, 0, 0 }
	};

	enable = 1;
	notify = 0;
	while ((c = getopt_long(argc, argv, "dhnv", long_opts, NULL)) != EOF) {
		switch (c) {
		case 'd':
			enable = 0;
			break;
		case 'h':
			print_help();
			return 0;
		case 'n':
			notify = 1;
			break;
		case 'v':
			printf(PROGRAM_NAME " " PROGRAM_VERSION "\n"
					"Copyright (C) 2016 Alexei Frolov\n\n"
					"This program is distributed as "
					"free software under the terms\n"
					"of the GNU General Public License, "
					"version 3 or later.\n");
			return 0;
		default:
			fprintf(stderr, "usage: %s [FILE]\n", argv[0]);
			return 1;
		}
	}

	keymap_init();

	head = NULL;
	if (optind != argc) {
		if (optind != argc - 1) {
			fprintf(stderr, "usage: %s [FILE]\n", argv[0]);
			keymap_free();
			return 1;
		}
		if (parse_file(argv[optind], &head) != 0) {
			keymap_free();
			return 1;
		}
	}

	init_display(notify);
	load_keys(head, enable);
	start_loop();
	unload_keys();
	close_display();
	keymap_free();
	return 0;
}

void print_help(void)
{
	printf("usage: " PROGRAM_NAME " [OPTION]... [FILE]\n");
	printf(PROGRAM_NAME " - a simple hotkey mapper\n\n");
	printf("    -d, --disable\n");
	printf("        disable hotkeys on load\n");
	printf("    -h, --help\n");
	printf("        display this help text and exit\n");
	printf("    -v, --version\n");
	printf("        print version information and exit\n");
}
