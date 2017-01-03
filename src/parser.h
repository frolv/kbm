/*
 * parser.h
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

#ifndef KBM_PARSER_H
#define KBM_PARSER_H

#include "hotkey.h"
#include "kbm.h"
#include "uthash.h"

#define CURR_IND(lex) (lex->pos - lex->line)
#define CURR_START(lex) (CURR_IND(lex) - lex->curr->len)
#define HAS_STR(tok) \
	(tok->tag == TOK_ID || tok->tag == TOK_FUNC \
	 || tok->tag == TOK_STRLIT || tok->tag == TOK_QUAL)

/* parser token types */
enum {
	TOK_NUM = 0x100,
	TOK_ID,
	TOK_ARROW,
	TOK_FUNC,
	TOK_STRLIT,
	TOK_MOD,
	TOK_QUAL
};

struct token {
	int             tag;    /* type of the token */
	size_t          len;    /* length of token's lexeme */
	union {
		int     val;    /* each token has either a numeric */
		char    *str;   /* or string value associated with it */
	};
	UT_hash_handle  hh;     /* handle for hashtable */
};

struct lexer {
	const char      *file_path;             /* path to the file */
	unsigned int    line_num;               /* number of line in file */
	char            line[BUFFER_SIZE];      /* line being read */
	char            *pos;                   /* current position in line */
	unsigned int    err_num;                /* line number of err_line */
	unsigned int    err_len;                /* length of error lexeme */
	char            err_line[BUFFER_SIZE];  /* full line of error */
	char            *err_pos;               /* error start position */
	FILE            *err_file;              /* error output file */
	struct token    *curr;                  /* the current parsed token */
};

void reserve_symbols(void);
void free_symbols(void);

/* basename: strip directories from file name */
const char *basename(const char *path);

/* parse_file: parse hotkeys from the file at path into head */
int parse_file(const char *path, struct hotkey **head, FILE *err);

#endif /* KBM_PARSER_H */
