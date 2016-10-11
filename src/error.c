/*
 * error.c
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

#include "error.h"

/* print a nice looking error message */
#define PUTERR(lex, lnum, ind, fmt, ...) \
	fprintf(stderr, KWHT "%s:%u:%ld: " KNRM \
		KRED "error: " KNRM fmt, \
		lex->file_path, lnum, (ind) + 1, \
		##__VA_ARGS__)

#define PUTWARN(lex, lnum, ind, fmt, ...) \
	fprintf(stderr, KWHT "%s:%u:%ld: " KNRM \
		KMAG "warning: " KNRM fmt, \
		lex->file_path, lnum, (ind) + 1, \
		##__VA_ARGS__)

#define PUTNOTE(lex, lnum, ind, fmt, ...) \
	fprintf(stderr, KWHT "%s:%u:%ld: " KNRM \
		KBLU "note: " KNRM fmt, \
		lex->file_path, lnum, (ind) + 1, \
		##__VA_ARGS__)

#define SUB_TO_ZERO(a, b) (((a) < (b)) ? 0 : (a) - (b))

static void print_segment(const char *buf, size_t start,
			  size_t end, const char *colour);
static void print_caret(size_t nspace, size_t len, const char *colour);
static void print_token(const struct lexer *lex, const struct token *t,
			const char *colour);

/* err_unterm: print error detailing an unterminated string literal */
void err_unterm(struct lexer *lex)
{
	size_t start, end, col;

	start = SUB_TO_ZERO(CURR_IND(lex), 79);

	PUTERR(lex, lex->line_num, CURR_IND(lex),
			"unterminated string literal\n");
	print_segment(lex->line, start, CURR_IND(lex), NULL);
	putc('\n', stderr);
	print_caret(CURR_IND(lex) - start, 1, KRED);

	/* print the line where the error started */
	if (lex->err_num != lex->line_num) {
		col = lex->err_pos - lex->err_line;
		start = SUB_TO_ZERO(col, 79);
		end = start + 80;

		PUTNOTE(lex, lex->err_num, col, "started here\n");
		print_segment(lex->err_line, start, col, NULL);
		print_segment(lex->err_line, col, end, KBLU);
		if (end < strlen(lex->err_line))
			putc('\n', stderr);
		else
			end = strlen(lex->err_line);
		print_caret(col, end - col, KBLU);
	}
}

/* err_generic: print a generic error with message err */
void err_generic(struct lexer *lex, const char *err)
{
	size_t start, end;

	start = SUB_TO_ZERO(CURR_IND(lex), 40);
	end = start + 80;

	if (start > CURR_START(lex))
		start = CURR_START(lex);
	if (end < (size_t)CURR_IND(lex))
		end = CURR_IND(lex);

	PUTERR(lex, lex->line_num, CURR_START(lex), "%s\n", err);
	print_segment(lex->line, start, CURR_START(lex), NULL);
	print_token(lex, lex->curr, KRED);
	print_segment(lex->line, CURR_IND(lex), end, NULL);
	if (end < strlen(lex->line))
		putc('\n', stderr);
	print_caret(CURR_IND(lex) - start - lex->curr->len, lex->curr->len, KRED);
}

/* err_invkey: print error showing that the parsed token is not a valid key */
void err_invkey(struct lexer *lex)
{
	size_t start, end;

	if (lex->curr->tag == TOK_NUM)
		PUTERR(lex, lex->line_num, CURR_START(lex),
				"invalid key '%d'\n", lex->curr->val);
	else if (lex->curr->tag == TOK_ID || lex->curr->tag == TOK_FUNC
					  || lex->curr->tag == TOK_STRLIT)
		PUTERR(lex, lex->line_num, CURR_START(lex),
				"invalid key '%s'\n", lex->curr->str);
	else if (lex->curr->tag == TOK_ARROW)
		PUTERR(lex, lex->line_num, CURR_START(lex),
				"invalid key '->'\n");
	else
		PUTERR(lex, lex->line_num, CURR_START(lex),
				"invalid key '%c'\n", lex->curr->tag);

	start = SUB_TO_ZERO(CURR_IND(lex), 40);
	end = start + 80;

	if (start > CURR_START(lex))
		start = CURR_START(lex);
	if (end < (size_t)CURR_IND(lex))
		end = CURR_IND(lex);

	print_segment(lex->line, start, CURR_START(lex), NULL);
	print_token(lex, lex->curr, KRED);
	print_segment(lex->line, CURR_IND(lex), end, NULL);
	if (end < strlen(lex->line))
		putc('\n', stderr);
	print_caret(CURR_IND(lex) - start - lex->curr->len, lex->curr->len, KRED);
}

/* err_selfmod: print error indicating a key has been modified with itself */
void err_selfmod(struct lexer *lex)
{
	size_t start, end;
	long col;

	col = lex->err_pos - lex->err_line;
	start = SUB_TO_ZERO(col, 40);
	end = start + 80;

	PUTERR(lex, lex->err_num, col, "key modified with itself\n");
	print_segment(lex->err_line, start, col, NULL);
	print_segment(lex->err_line, col, col + lex->err_len, KRED);
	print_segment(lex->err_line, col + lex->err_len, end, NULL);
	if (end < strlen(lex->err_line))
		putc('\n', stderr);
	print_caret(col, lex->err_len, KRED);
}

/* err_eof: print error indicating parser has hit EOF unexpectedly */
void err_eof(struct lexer *lex)
{
	size_t start, end, col, err_end;

	col = lex->err_pos - lex->err_line;
	err_end = col + lex->err_len;
	lex->pos = strchr(lex->line, '\n');
	start = SUB_TO_ZERO(CURR_IND(lex), 79);

	PUTERR(lex, lex->line_num, -1L, "unexpected EOF when parsing\n");
	print_segment(lex->line, start, CURR_IND(lex), NULL);
	putc('\n', stderr);
	print_caret(CURR_IND(lex) - start, 1, KRED);

	PUTNOTE(lex, lex->err_num, col, "last statement here\n");
	end = strlen(lex->err_line);
	start = SUB_TO_ZERO(end, 79);
	print_segment(lex->err_line, start, col, NULL);
	print_segment(lex->err_line, col, err_end, KBLU);
	print_segment(lex->err_line, err_end, end, NULL);
	print_caret(col, lex->err_len, KBLU);
}

/* warn_literal: print warning that string literal is being truncated */
void warn_literal(struct lexer *lex, size_t lim, int quote)
{
	size_t start;

	start = SUB_TO_ZERO(CURR_IND(lex), 79);

	PUTWARN(lex, lex->line_num, CURR_IND(lex), "string literal exceeding "
			"%zu characters truncated\n", lim);
	print_segment(lex->line, start, CURR_IND(lex), NULL);
	printf(KMAG "%c" KNRM "\n", quote);
	print_caret(CURR_IND(lex) - start, 1, KMAG);
}

/* note_duplicate: print a note informing duplicate modifier declaration */
void note_duplicate(struct lexer *lex)
{
	size_t start, end, err_end;
	long col;

	col = lex->err_pos - lex->err_line;
	start = SUB_TO_ZERO(col, 40);
	end = start + 80;
	err_end = col + lex->err_len;

	PUTNOTE(lex, lex->line_num, col, "duplicate modifier declaration\n");
	print_segment(lex->err_line, start, col, NULL);
	print_segment(lex->err_line, col, err_end, KBLU);
	print_segment(lex->err_line, err_end, end, NULL);
	if (end < strlen(lex->err_line))
		putc('\n', stderr);
	print_caret(col, lex->err_len, KBLU);
}

/* print_segment: print buf from start to end */
static void print_segment(const char *buf, size_t start,
			  size_t end, const char *colour)
{
	size_t i;

	if (end > (i = strlen(buf)))
		end = i;
	if (start > end)
		return;

	if (colour)
		fprintf(stderr, "%s", colour);
	for (i = start; i < end; ++i)
		putc(buf[i], stderr);
	if (colour)
		fprintf(stderr, KNRM);
}

/* print_caret: print nspace spaces followed by a caret indicator of size len */
static void print_caret(size_t nspace, size_t len, const char *colour)
{
	size_t i;

	for (i = 0; i < nspace; ++i)
		putc(' ', stderr);
	fprintf(stderr, "%s^", colour);
	for (i = 0; i < len - 1; ++i)
		putc('~', stderr);
	fprintf(stderr, KNRM "\n");
}

/* print_token: print token t as it appears in lex */
static void print_token(const struct lexer *lex, const struct token *t,
		 const char *colour)
{
	char *s;

	fprintf(stderr, "%s", colour);
	for (s = lex->pos - t->len; s < lex->pos; ++s)
		putc(*s, stderr);
	fprintf(stderr, KNRM);
}
