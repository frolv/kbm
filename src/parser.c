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

#define BUFFER_SIZE 4096

enum {
	TOK_NUM = 256,
	TOK_ID,
	TOK_ARROW
};

struct token {
	int tag;
	union {
		int num;
		char *lexeme;
	};
	UT_hash_handle hh;
};

static int peek;
static unsigned int line;

/* hash table of reserved keywords */
static struct token *reserved;

static void reserve(struct token *word);
static struct token *scan(FILE *f);
static struct token *create_token(int tag, void *info);
static void free_token(struct token *t);

struct hotkey *parse_file(FILE *f)
{
	struct hotkey *head;
	struct token *t, *tmp;

	head = NULL;
	peek = ' ';
	line = 1;
	reserved = NULL;

	reserve(create_token(TOK_ID, "click"));
	reserve(create_token(TOK_ID, "rclick"));
	reserve(create_token(TOK_ID, "jump"));
	reserve(create_token(TOK_ID, "key"));
	reserve(create_token(TOK_ID, "toggle"));
	reserve(create_token(TOK_ID, "quit"));
	reserve(create_token(TOK_ID, "exec"));

	while ((t = scan(f))) {
		switch (t->tag) {
		case TOK_NUM:
			printf("%d\n", t->num);
			break;
		case TOK_ID:
			printf("%s\n", t->lexeme);
			break;
		case TOK_ARROW:
			printf("->\n");
			break;
		default:
			printf("%c\n", t->tag);
			break;
		}
	}

	/* free hash table contents */
	HASH_ITER(hh, reserved, t, tmp) {
		HASH_DEL(reserved, t);
		free_token(t);
	}

	return head;
}

static void reserve(struct token *word)
{
	HASH_ADD_KEYPTR(hh, reserved, word->lexeme, strlen(word->lexeme), word);
}

/* scan: read the next token from f */
static struct token *scan(FILE *f)
{
	int i;
	char buf[BUFFER_SIZE];
	struct token *t;

	for (;; peek = fgetc(f)) {
		if (peek == ' ' || peek == '\t') {
			continue;
		} else if (peek == '\n') {
			++line;
		} else if (peek == '#') {
			/* rest of line is a comment */
			while ((peek = fgetc(f)) != EOF && peek != '\n')
				;
			if (peek == EOF)
				return NULL;
			++line;
		} else {
			break;
		}
	}

	if (isdigit(peek)) {
		i = 0;
		do {
			i = 10 * i + (peek - '0');
			peek = fgetc(f);
		} while (isdigit(peek));
		return create_token(TOK_NUM, &i);
	}
	if (isalpha(peek) || peek == '_') {
		i = 0;
		do {
			buf[i++] = peek;
			peek = fgetc(f);
		} while ((isalnum(peek) || peek == '_') && i < BUFFER_SIZE - 1);
		buf[i] = '\0';
		HASH_FIND_STR(reserved, buf, t);
		if (t)
			return t;
		t = create_token(TOK_ID, &buf);
		HASH_ADD_KEYPTR(hh, reserved, t->lexeme, strlen(t->lexeme), t);
		return t;
	}
	if (peek == '-') {
		if ((peek = fgetc(f)) == '>') {
			peek = ' ';
			return create_token(TOK_ARROW, NULL);
		}
		/* unary minus sign */
		return create_token('-', NULL);
	}
	if (peek == EOF)
		return NULL;

	t = create_token(peek, NULL);
	peek = ' ';
	return t;
}

static struct token *create_token(int tag, void *info)
{
	struct token *t;

	t = malloc(sizeof(*t));
	t->tag = tag;

	switch (tag) {
	case TOK_NUM:
		t->num = *(int *)info;
		break;
	case TOK_ID:
		t->lexeme = strdup((char *)info);
		break;
	case TOK_ARROW:
	default:
		break;
	}

	return t;
}

static void free_token(struct token *t)
{
	if (t->tag == TOK_ID)
		free(t->lexeme);
	free(t);
}
