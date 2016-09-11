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
	TOK_ARROW,
	TOK_FUNC,
	TOK_STRLIT,
	TOK_SMOD
};

struct token {
	int tag;
	union {
		int num;
		char *lexeme;
		char *fname;
		char *str;
	};
	UT_hash_handle hh;
};

static int peek;
static unsigned int line;

/* hash table of words/symbols */
static struct token *words;

static struct token *scan(FILE *f);
static struct token *read_str(FILE *f);
static void reserve(struct token *word);
static struct token *create_token(int tag, void *info);
static void free_token(struct token *t);

struct hotkey *parse_file(FILE *f)
{
	struct hotkey *head;
	struct token *t, *tmp;

	head = NULL;
	peek = ' ';
	line = 1;
	words = NULL;

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
			printf("%d\n", t->num);
			break;
		case TOK_ID:
			printf("%s\n", t->lexeme);
			break;
		case TOK_ARROW:
			printf("->\n");
			break;
		case TOK_FUNC:
			printf("%s\n", t->fname);
			break;
		case TOK_STRLIT:
			printf("%s\n", t->str);
			break;
		default:
			printf("%c\n", t->tag);
			break;
		}
	}

	/* free hash table contents */
	HASH_ITER(hh, words, t, tmp) {
		HASH_DEL(words, t);
		free_token(t);
	}

	return head;
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
		HASH_FIND_STR(words, buf, t);
		if (t)
			return t;
		t = create_token(TOK_ID, &buf);
		HASH_ADD_KEYPTR(hh, words, t->lexeme, strlen(t->lexeme), t);
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
	if (peek == '"' || peek == '\'')
		return read_str(f);

	if (peek == EOF)
		return NULL;

	t = create_token(peek, NULL);
	peek = ' ';
	return t;
}

/* read_str: read a string literal from file, return token containing it */
static struct token *read_str(FILE *f)
{
	int quote, i;
	char buf[BUFFER_SIZE];

	quote = peek;
	for (i = 0; (peek = fgetc(f)) != EOF; ++i) {
		if (peek == quote) {
			if (buf[i - 1] == '\\')
				--i;
			else
				break;
		}
		if (peek == '\n') {
			if (buf[i - 1] != '\\') {
				peek = EOF;
				break;
			} else {
				/* skip backslash and newline */
				i -= 2;
				++line;
				continue;
			}
		}
		buf[i] = peek;
	}
	buf[i] = '\0';
	if (peek == EOF) {
		fprintf(stderr, "line %u: unterminated string literal\n", line);
		return NULL;
	}

	peek = ' ';
	return create_token(TOK_STRLIT, &buf);
}

static void reserve(struct token *word)
{
	HASH_ADD_KEYPTR(hh, words, word->lexeme, strlen(word->lexeme), word);
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
	case TOK_FUNC:
		t->fname = strdup((char *)info);
		break;
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
	if (t->tag == TOK_ID)
		free(t->lexeme);
	if (t->tag == TOK_FUNC)
		free(t->fname);
	if (t->tag == TOK_STRLIT)
		free(t->str);
	free(t);
}
