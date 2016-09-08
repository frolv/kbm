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

#define BUFFER_SIZE 4096

static int peek;
static unsigned int line;

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
};

static void reserve(struct token *word);
static struct token *scan(FILE *f);
static struct token *create_token(int tag, void *info);
static void free_token(struct token *t);

struct hotkey *parse_file(FILE *f)
{
	struct hotkey *head;
	struct token *t;

	head = NULL;
	peek = ' ';
	line = 1;

	return head;
}

static void reserve(struct token *word)
{
}

/* scan: read the next token from f */
static struct token *scan(FILE *f)
{
	int i;
	char buf[BUFFER_SIZE];

	for (;; peek = fgetc(f)) {
		if (peek == ' ' || peek == '\t')
			continue;
		else if (peek == '\n')
			++line;
		else
			break;
	}
	if (isdigit(peek)) {
		i = 0;
		do {
			i = 10 * i + (peek - '0');
			peek = fgetc(f);
		} while (isdigit(peek));
		PRINT_DEBUG("%d\n", i);
		return create_token(TOK_NUM, &i);
	}
	if (isalpha(peek)) {
		i = 0;
		do {
			buf[i++] = peek;
			peek = fgetc(f);
		} while (isalnum(peek) && i < BUFFER_SIZE - 1);
		buf[i] = '\0';
		PRINT_DEBUG("%s\n", buf);
		return create_token(TOK_ID, &buf);
	}
	if (peek == '-') {
		if ((peek = fgetc(f)) != '>') {
			fprintf(stderr, "syntax error on line %u: expected '>' "
					"after '-'\n", line);
			exit(1);
		}
		peek = ' ';
		PRINT_DEBUG("->\n");
		return create_token(TOK_ARROW, NULL);
	}
	if (peek == EOF)
		return NULL;
	fprintf(stderr, "syntax error on line %u: unrecognized token '%c'\n",
			line, peek);
	exit(1);
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
		break;
	default:
		free(t);
		return NULL;
	}

	return t;
}

static void free_token(struct token *t)
{
	if (t->tag == TOK_ID)
		free(t->lexeme);
	free(t);
}
