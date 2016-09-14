/*
 * parser.c
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "uthash.h"

#if defined(__linux__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define BUFFER_SIZE	4096
#define MAX_STR		1024

/* print a nice looking error message */
#define PUTERR(fmt, ...) \
	fprintf(stderr, KWHT "%s:%u:%ld: " KNRM \
		KRED "error: " KNRM fmt, \
		file_path, line_num, pos - line + 1, \
		##__VA_ARGS__)

#define PUTWARN(fmt, ...) \
	fprintf(stderr, KWHT "%s:%u:%ld: " KNRM \
		KMAG "warning: " KNRM fmt, \
		file_path, line_num, pos - line + 1, \
		##__VA_ARGS__)

#define GET_OFFSET(offset) ((((pos - line) + (offset)) < 0) \
		? 0 : ((pos - line) + (offset)))

enum {
	TOK_NUM = 0x100,
	TOK_ID,
	TOK_ARROW,
	TOK_FUNC,
	TOK_STRLIT,
	TOK_MOD
};

struct token {
	int tag;
	union {
		int val;
		char *str;
	};
	UT_hash_handle hh;
};

static const char *file_path;
static unsigned int line_num;
static char line[BUFFER_SIZE];
static char *pos;

/* hash table of reserved words */
static struct token *reserved;

static FILE *open_file(const char *path);
static struct token *scan(FILE *f);
static struct token *read_str(FILE *f);
static struct token *create_token(int tag, void *info);
static void free_token(struct token *t);
static void reserve(struct token *word);
static char *next_line(FILE *f);

static void print_segment(size_t start, size_t end);
static void print_carat(size_t nspace, size_t len, const char *colour);

struct hotkey *parse_file(const char *path)
{
	struct hotkey *head;
	struct token *t, *tmp;
	FILE *f;
	size_t start, end;

	head = NULL;
	file_path = path;

	if (strcmp(path, "-") == 0) {
		f = stdin;
		file_path = "<stdin>";
	} else if (!(f = open_file(path))) {
		exit(1);
	}

	line_num = 0;
	pos = next_line(f);

	reserved = NULL;
	reserve(create_token(TOK_FUNC, "click"));
	reserve(create_token(TOK_FUNC, "rclick"));
	reserve(create_token(TOK_FUNC, "jump"));
	reserve(create_token(TOK_FUNC, "key"));
	reserve(create_token(TOK_FUNC, "toggle"));
	reserve(create_token(TOK_FUNC, "quit"));
	reserve(create_token(TOK_FUNC, "exec"));

	while ((t = scan(f))) {
		switch (t->tag) {
		case TOK_NUM:
			PRINT_DEBUG("%d\n", t->val);
			break;
		case TOK_ARROW:
			PRINT_DEBUG("->\n");
			break;
		case TOK_ID:
		case TOK_FUNC:
		case TOK_STRLIT:
			PRINT_DEBUG("%s\n", t->str);
			break;
		case TOK_MOD:
			PRINT_DEBUG("%c\n", t->val);
			break;
		default:
			PRINT_DEBUG("%c\n", t->tag);
			break;
		}

		if (t->tag != TOK_FUNC)
			free_token(t);
	}

	/* free hash table contents */
	HASH_ITER(hh, reserved, t, tmp) {
		HASH_DEL(reserved, t);
		free_token(t);
	}

	if (pos) {
		PUTERR("unrecognized token '%c'\n", *pos);
		start = GET_OFFSET(-40);
		end = start + 80;
		print_segment(start, pos - line);
		fprintf(stderr, KRED "%c" KNRM, *pos);
		print_segment(pos - line + 1, end);
		if (end < strlen(line))
			putc('\n', stderr);
		print_carat(pos - line - start, 1, KRED);
		exit(1);
	}

	fclose(f);
	return head;
}

#if defined(__linux__) || defined(__APPLE__)
/* open_file: open the file at path with error checking */
static FILE *open_file(const char *path)
{
	struct stat statbuf;
	FILE *f;

	if (stat(path, &statbuf) != 0) {
		perror(path);
		return NULL;
	}

	if (!S_ISREG(statbuf.st_mode)) {
		fprintf(stderr, "%s: not a regular file\n", path);
		return NULL;
	}

	if (!(f = fopen(path, "r"))) {
		perror(path);
		return NULL;
	}
	return f;
}
#endif

/* scan: read the next token from f */
static struct token *scan(FILE *f)
{
	static const char *misc_keys = "`-=[]\\;',./";

	int i;
	char buf[BUFFER_SIZE];
	struct token *t;

	/* skip over whitespace */
	for (;; ++pos) {
		while (*pos == '\n' || *pos == '#') {
			if (!(pos = next_line(f)))
				return NULL;
		}

		if (*pos != ' ' && *pos != '\t')
			break;

		/* makes printing an arrow on error messages easier */
		if (*pos == '\t')
			*pos = ' ';
	}

	i = 0;
	if (isdigit(*pos)) {
		do {
			i = 10 * i + (*pos - '0');
		} while (isdigit(*++pos));
		return create_token(TOK_NUM, &i);
	}
	if (isalpha(*pos) || *pos == '_') {
		do {
			buf[i++] = *pos++;
		} while ((isalnum(*pos) || *pos == '_') && i < BUFFER_SIZE - 1);
		buf[i] = '\0';
		HASH_FIND_STR(reserved, buf, t);
		if (t)
			return t;
		return create_token(TOK_ID, &buf);
	}
	if (*pos == '-') {
		if (*++pos == '>') {
			++pos;
			return create_token(TOK_ARROW, NULL);
		}
		/* unary minus sign */
		return create_token('-', NULL);
	}
	if (*pos == '^' || *pos == '~' || *pos == '!' || *pos == '#') {
		i = *pos++;
		return create_token(TOK_MOD, &i);
	}
	if (strchr(misc_keys, *pos))
		return create_token(*pos++, NULL);
	if (*pos == '"')
		return read_str(f);

	/* EOF or invalid token */
	return NULL;
}

static void err_unterm();

/* read_str: read a string literal from f, return token containing it */
static struct token *read_str(FILE *f)
{
	int quote, i, start;
	char buf[MAX_STR];

	quote = *pos++;
	for (i = 0; i < MAX_STR - 1; ++i) {
		if (*pos == '\n') {
			if ((i && buf[i - 1] != '\\') || !(pos = next_line(f)))
				break;
			--i;
		}
		if (*pos == quote) {
			if (i && buf[i - 1] == '\\')
				--i;
			else
				break;
		}
		buf[i] = *pos++;
	}
	buf[i] = '\0';

	if (i == MAX_STR - 1) {
		PUTWARN("string literal exceeding %d characters truncated\n",
				MAX_STR - 1);
		start = GET_OFFSET(-79);
		print_segment(start, pos - line);
		printf(KMAG "%c" KNRM "\n", quote);
		print_carat(pos - line - start, 1, KMAG);

		/* skip over the rest of the string */
		while (1) {
			if (*pos == '\n' && (*(pos - 1) != '\\'
						|| !(pos = next_line(f)))) {
				err_unterm();
				exit(1);
			}
			if (*pos == quote && (pos == line || *(pos - 1) != '\\'))
				break;
			++pos;
		}
		++pos;
		return create_token(TOK_STRLIT, &buf);
	}

	if (!pos || *pos != quote) {
		err_unterm();
		exit(1);
	}

	++pos;
	return create_token(TOK_STRLIT, &buf);
}

static void reserve(struct token *word)
{
	HASH_ADD_KEYPTR(hh, reserved, word->str, strlen(word->str), word);
}

static struct token *create_token(int tag, void *info)
{
	struct token *t;

	t = malloc(sizeof(*t));
	t->tag = tag;

	switch (tag) {
	case TOK_NUM:
	case TOK_MOD:
		t->val = *(int *)info;
		break;
	case TOK_ID:
	case TOK_FUNC:
	case TOK_STRLIT:
		t->str = strdup((char *)info);
		break;
	case TOK_ARROW:
	default:
		break;
	}

	return t;
}

static void free_token(struct token *t)
{
	if (t->tag == TOK_ID || t->tag == TOK_FUNC || t->tag == TOK_STRLIT)
		free(t->str);
	free(t);
}

/* next_line: read from file until the next non-empty line */
static char *next_line(FILE *f)
{
	char *s;

	do {
		++line_num;
		s = fgets(line, BUFFER_SIZE, f);
	} while (s && (!*s || *s == '\n'));

	return s;
}

/* print_segment: print line from start to end */
static void print_segment(size_t start, size_t end)
{
	size_t i;

	if (end > (i = strlen(line)))
		end = i;

	for (i = start; i < end; ++i)
		putc(line[i], stderr);
}

static void print_carat(size_t nspace, size_t len, const char *colour)
{
	size_t i;

	for (i = 0; i < nspace; ++i)
		putc(' ', stderr);
	fprintf(stderr, "%s^", colour);
	for (i = 0; i < len - 1; ++i)
		putc('~', stderr);
	fprintf(stderr, KNRM "\n");
}

static void err_unterm()
{
	int start;

	PUTERR("unterminated string literal\n");
	start = GET_OFFSET(-79);
	print_segment(start, pos - line);
	putc('\n', stderr);
	print_carat(pos - line - start, 1, KRED);
}
