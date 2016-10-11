/*
 * kbm.h
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

#ifndef KBM_H
#define KBM_H

#include <stdio.h>

#define PROGRAM_NAME "kbm"

#define _VERSION_MAJOR "0"
#define _VERSION_MINOR "2"
#define _VERSION_PATCH "0"

#define PROGRAM_VERSION \
	"v" _VERSION_MAJOR "." _VERSION_MINOR "." _VERSION_PATCH

#ifdef KBM_DEBUG
#define PRINT_DEBUG(...) printf(__VA_ARGS__)
#else
#define PRINT_DEBUG(...) ((void)0)
#endif

/* print beautiful coloured output */
#if defined(__linux__) || defined(__APPLE__)
#define KNRM	"\x1B[0m"
#define KRED	"\x1B[1;31m"
#define KGRN	"\x1B[1;32m"
#define KYEL	"\x1B[1;33m"
#define KBLU	"\x1B[1;34m"
#define KMAG	"\x1B[1;35m"
#define KCYN	"\x1B[1;36m"
#define KWHT	"\x1B[1;37m"
#else
#define KNRM	""
#define KRED	""
#define KGRN	""
#define KYEL	""
#define KBLU	""
#define KMAG	""
#define KCYN	""
#define KWHT	""
#endif

#define BUFFER_SIZE 4096

#if defined(__CYGWIN__) || defined (__MINGW32__)
#include <Windows.h>
#endif

struct _program_info {
	int keys_active;	/* whether hotkeys are active */
	int notifications;	/* whether notifications are enabled */
#if defined(__CYGWIN__) || defined (__MINGW32__)
	HINSTANCE instance;	/* program instance */
#endif
};

extern struct _program_info kbm_info;

#endif /* KBM_H */
