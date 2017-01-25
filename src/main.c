/*
 * kbm - a simple hotkey mapper
 * main.c
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

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "kbm.h"
#include "display.h"
#include "hotkey.h"
#include "parser.h"

#ifdef __APPLE__
extern int NSApplicationMain(int argc, char **argv);
#endif

static const struct option long_opts[] = {
	{ "disable", no_argument, 0, 'd' },
	{ "help", no_argument, 0, 'h' },
	{ "no-notifications", no_argument, 0, 'n' },
	{ "version", no_argument, 0, 'v' },
	{ 0, 0, 0, 0 }
};

struct _program_info kbm_info;

static void parseopts(int argc, char **argv);
static void print_help(void);
#if defined(__linux__) || defined(__CYGWIN__) || defined (__MINGW32__)
static int run(void);
#endif

#ifdef __linux__
int main(int argc, char **argv)
{
	parseopts(argc, argv);
	return run();
}
#endif /* __linux__ */

#if defined(__CYGWIN__) || defined (__MINGW32__)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	KBM_UNUSED(hPrevInstance);
	KBM_UNUSED(lpCmdLine);
	KBM_UNUSED(nCmdShow);

	kbm_info.instance = hInstance;
	parseopts(__argc, __argv);
	return run();
}
#endif /* __CYGWIN__ || __MINGW32__ */

#ifdef __APPLE__
int main(int argc, char **argv)
{
	parseopts(argc, argv);
	return NSApplicationMain(argc, argv);
}
#endif /* __APPLE__ */

/* parseopts: parse program options and load hotkeys */
static void parseopts(int argc, char **argv)
{
	int c;

	kbm_info.keys_active = 1;
	kbm_info.keys_toggled = 1;
	kbm_info.notifications = 1;
	kbm_info.curr_file = NULL;
	memset(&kbm_info.map, 0, sizeof kbm_info.map);

	while ((c = getopt_long(argc, argv, "dhnv", long_opts, NULL)) != EOF) {
		switch (c) {
		case 'd':
			kbm_info.keys_toggled = 0;
			break;
		case 'h':
			print_help();
			exit(0);
		case 'n':
			kbm_info.notifications = 0;
			break;
		case 'v':
			printf(PROGRAM_NAME " " PROGRAM_VERSION "\n"
			       "Copyright (C) 2016-2017 Alexei Frolov\n\n"
			       "This program is distributed as "
			       "free software under the terms\n"
			       "of the GNU General Public License, "
			       "version 3 or later.\n");
			exit(0);
		default:
			fprintf(stderr, "usage: %s [FILE]\n", argv[0]);
			exit(1);
		}
	}

	keymap_init();
	reserve_symbols();

	if (optind != argc) {
		if (optind != argc - 1) {
			fprintf(stderr, "usage: %s [FILE]\n", argv[0]);
			goto err_cleanup;
		}
		if (parse_file(argv[optind], &kbm_info.map, stderr) != 0)
			goto err_cleanup;

		kbm_info.curr_file = basename(argv[optind]);
		if (strcmp(kbm_info.curr_file, "-") == 0)
			kbm_info.curr_file = "stdin";
	}

	if (init_display() != 0)
		goto err_cleanup;

	load_keys(kbm_info.map.keys);
	return;

err_cleanup:
	free_windows(&kbm_info.map);
	keymap_free();
	free_symbols();
	exit(1);
}

#if defined(__linux__) || defined(__CYGWIN__) || defined (__MINGW32__)
static int run(void)
{
	start_listening();
	unload_keys();
	close_display();
	free_windows(&kbm_info.map);
	keymap_free();
	free_symbols();

	return 0;
}
#endif /* __linux__ || __CYGWIN__ || __MINGW32__ */

static void print_help(void)
{
	printf("usage: " PROGRAM_NAME " [OPTION]... [FILE]\n");
	printf(PROGRAM_NAME " - a simple hotkey mapper\n\n");
	printf("    -d, --disable\n");
	printf("        disable hotkeys on load\n");
	printf("    -h, --help\n");
	printf("        display this help text and exit\n");
	printf("    -n, --no-notifications\n");
	printf("        don't send desktop notification when keys are toggled\n");
	printf("    -v, --version\n");
	printf("        print version information and exit\n");
}
