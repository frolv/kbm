/*
 * parser.c
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "parser.h"

#if defined(__linux__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define SCAN_SIZE  64U
#define MAX_STRING 1024U

#define ISMOD(lexeme) \
	((lexeme) == '^' || (lexeme) == '!' \
	 || (lexeme) == '~' || (lexeme) == '@')

#define IS_RESERVED(tok) (tok->tag == TOK_FUNC || tok->tag == TOK_QUAL)

/* set bitmask mask to mods with duplicate notice */
#define SET_MODS(mods, mask, lex) \
	do { \
		if (CHECK_MASK(mods, mask)) \
			note_duplicate(lex); \
		mods |= mask; \
	} while (0)

/* hash table of reserved words */
static struct token *reserved;

static FILE *open_file(const char *path);
static struct token *scan(FILE *f, struct lexer *lex);
static struct token *read_str(FILE *f, struct lexer *lex);
static struct token *create_token(int tag, void *info);
static void free_token(struct token *t);
static void reserve(struct token *word);
static char *next_line(FILE *f, struct lexer *lex);
static int next_token(FILE *f, struct lexer *lex, int free, int err);

static int parse_func(FILE *f, struct lexer *lex, uint8_t *op, uint64_t *args);
static int parse_num(FILE *f, struct lexer *lex, uint32_t *num);
static int parse_exec(FILE *f, struct lexer *lex, uint64_t *retval);
static int parse_qual(FILE *f, struct lexer *lex, uint32_t *flags);
static int validkey(uint64_t *key, struct lexer *lex);

/* reserve_symbols: populate the reserved hashtable with keyword tokens */
void reserve_symbols(void)
{
	reserved = NULL;
	reserve(create_token(TOK_FUNC, "click"));
	reserve(create_token(TOK_FUNC, "rclick"));
	reserve(create_token(TOK_FUNC, "jump"));
	reserve(create_token(TOK_FUNC, "key"));
	reserve(create_token(TOK_FUNC, "toggle"));
	reserve(create_token(TOK_FUNC, "quit"));
	reserve(create_token(TOK_FUNC, "exec"));
	reserve(create_token(TOK_QUAL, "norepeat"));
	reserve(create_token(TOK_GDEF, "active_window"));
}

/* free_symbols: free all tokens in the reserved hashtable */
void free_symbols(void)
{
	struct token *t, *tmp;

	HASH_ITER(hh, reserved, t, tmp) {
		HASH_DEL(reserved, t);
		free_token(t);
	}
}

#if defined(__CYGWIN__) || defined (__MINGW32__)
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

/* basename: strip directories from file name */
const char *basename(const char *path)
{
	const char *s = strrchr(path, PATH_SEP);
	return s ? s + 1 : path;
}

static void parse_globals(FILE *f, struct lexer *lex, struct keymap *k);
static struct hotkey *parse_binding(FILE *f, struct lexer *lex);

/*
 * parse_file:
 * Read the file at path, if it is accessible.
 * Process the keybindings in the file and store a
 * list of struct hotkeys representing them in head.
 */
int parse_file(const char *path, struct keymap *k, FILE *err)
{
	struct hotkey *hk;
	struct lexer lex;
	FILE *f;
	int ret;

	lex.file_path = path;
	lex.err_file = err;

	if (strcmp(path, "-") == 0) {
		f = stdin;
		lex.file_path = "<stdin>";
	} else if (!(f = open_file(path))) {
		return 1;
	}

	lex.line_num = 0;
	if (!next_line(f, &lex))
		return 0;

	/* grab the first token */
	next_token(f, &lex, 0, 0);

	/* global definitions at the start of the file */
	parse_globals(f, &lex, k);
	if (!lex.curr) {
		ret = 1;
		goto cleanup;
	}

	ret = 0;
	while (lex.curr) {
		if (!(hk = parse_binding(f, &lex))) {
			if (lex.curr && !IS_RESERVED(lex.curr))
				free_token(lex.curr);
			if (k->keys)
				free_keys(k->keys);
			free_windows(k);
			ret = 1;
			goto cleanup;
		}
		PRINT_DEBUG("hotkey parsed: %s\n",
		            keystr(hk->kbm_code, hk->kbm_modmask));
		add_hotkey(&k->keys, hk);
	}

cleanup:
	fclose(f);
	return ret;
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

#if defined(__CYGWIN__) || defined (__MINGW32__)
static FILE *open_file(const char *path)
{
	FILE *f;

	if (!(f = fopen(path, "r"))) {
		perror(path);
		return NULL;
	}
	return f;
}
#endif

/* scan: read the next token from f */
static struct token *scan(FILE *f, struct lexer *lex)
{
	unsigned int i;
	char buf[SCAN_SIZE];
	struct token *t;

	/* skip over whitespace */
	for (;; lex->pos++) {
		while (!*lex->pos || *lex->pos == '\n' || *lex->pos == '#') {
			if (!next_line(f, lex))
				return NULL;
		}

		if (*lex->pos != ' ' && *lex->pos != '\t')
			break;

		/* makes printing an arrow on error messages easier */
		if (*lex->pos == '\t')
			*lex->pos = ' ';
	}

	i = 0;
	if (isdigit(*lex->pos)) {
		do {
			i = 10 * i + (*lex->pos - '0');
			lex->pos++;
		} while (isdigit(*lex->pos));
		return create_token(TOK_NUM, &i);
	}
	if (isalpha(*lex->pos) || *lex->pos == '_') {
		do {
			buf[i++] = *lex->pos;
			lex->pos++;
		} while ((isalnum(*lex->pos) || *lex->pos == '_')
		         && i < SCAN_SIZE - 1);
		buf[i] = '\0';
		HASH_FIND_STR(reserved, buf, t);
		return t ? t : create_token(TOK_ID, &buf);
	}
	if (*lex->pos == '-') {
		if (*++(lex->pos) == '>') {
			lex->pos++;
			return create_token(TOK_ARROW, NULL);
		}
		return create_token('-', NULL);
	}
	if (ISMOD(*lex->pos)) {
		i = *lex->pos;
		lex->pos++;
		return create_token(TOK_MOD, &i);
	}
	if (*lex->pos == '"')
		return read_str(f, lex);

	return create_token(*(lex->pos)++, NULL);
}

/* read_str: read a string literal from f, return token containing it */
static struct token *read_str(FILE *f, struct lexer *lex)
{
	int quote;
	size_t i;
	char buf[MAX_STRING];

	/* record where the string literal started */
	strcpy(lex->err_line, lex->line);
	lex->err_num = lex->line_num;
	lex->err_pos = lex->err_line + CURR_IND(lex);
	lex->err_len = 1;

	quote = *(lex->pos)++;
	for (i = 0; i < MAX_STRING - 1; ++i) {
		if (*lex->pos == '\n') {
			if ((i && buf[i - 1] != '\\') || !next_line(f, lex))
				break;
			--i;
		}
		if (*lex->pos == quote) {
			if (i && buf[i - 1] == '\\')
				--i;
			else
				break;
		}
		buf[i] = *lex->pos;
		lex->pos++;
	}
	buf[i] = '\0';

	if (i == MAX_STRING - 1) {
		warn_literal(lex, MAX_STRING - 1, quote);

		/* skip over the rest of the string */
		while (1) {
			if (*lex->pos == '\n' && (*(lex->pos - 1) != '\\'
			                          || !next_line(f, lex))) {
				err_unterm(lex);
				exit(1);
			}
			if (*lex->pos == quote && (lex->pos == lex->line ||
			                           *(lex->pos - 1) != '\\'))
				break;
			lex->pos++;
		}
		lex->pos++;
		return create_token(TOK_STRLIT, &buf);
	}

	if (!lex->pos || *lex->pos != quote) {
		err_unterm(lex);
		exit(1);
	}

	lex->pos++;
	return create_token(TOK_STRLIT, &buf);
}

static void reserve(struct token *word)
{
	HASH_ADD_KEYPTR(hh, reserved, word->str, strlen(word->str), word);
}

static struct token *create_token(int tag, void *info)
{
	struct token *t;
	int i;

	t = malloc(sizeof *t);
	t->tag = tag;

	switch (tag) {
	case TOK_NUM:
		t->val = *(int *)info;
		i = t->val;
		t->len = 1;
		while ((i /= 10))
			t->len++;
		break;
	case TOK_MOD:
		t->val = *(int *)info;
		t->len = 1;
		break;
	case TOK_ID:
	case TOK_FUNC:
	case TOK_QUAL:
	case TOK_STRLIT:
	case TOK_GDEF:
		t->str = strdup((char *)info);
		t->len = strlen(t->str);
		/* surrounding quotes */
		if (tag == TOK_STRLIT)
			t->len += 2;
		break;
	case TOK_ARROW:
		t->len = 2;
		break;
	default:
		t->len = 1;
		break;
	}

	return t;
}

/* free_token: free a token and its dynamically allocated data */
static void free_token(struct token *t)
{
	if (HAS_STR(t))
		free(t->str);
	free(t);
}

/* next_line: read from file until the next non-empty line */
static char *next_line(FILE *f, struct lexer *lex)
{
	char *s;

	do {
		lex->line_num++;
		s = fgets(lex->line, BUFFER_SIZE, f);
	} while (s && (!*s || *s == '\n'));

	lex->pos = s;
	return s;
}

/*
 * next_token:
 * Read the next token from f and store pointer to it in lex->curr.
 * The existing token in lex is freed if free is set.
 * With err, an error message is printed if no token can be read.
 */
static int next_token(FILE *f, struct lexer *lex, int free, int err)
{
	if (err) {
		strcpy(lex->err_line, lex->line);
		lex->err_num = lex->line_num;
		lex->err_pos = lex->err_line + CURR_START(lex);
		lex->err_len = lex->curr->len;
	}

	if (free && lex->curr)
		free_token(lex->curr);

	if (!(lex->curr = scan(f, lex))) {
		if (err)
			err_eof(lex);
		return 1;
	}
	return 0;
}

static void parse_windows(FILE *f, struct lexer *lex, struct keymap *k)
{
	if (k->win_size == 0) {
		k->win_size = 10;
		k->windows = malloc(k->win_size * sizeof *k->windows);
	}

	while (lex->curr && lex->curr->tag == TOK_STRLIT) {
		if (k->win_len == k->win_size - 1) {
			k->win_size *= 2;
			k->windows = realloc(k->windows, k->win_size);
		}
		k->windows[k->win_len++] = lex->curr->str;
		PRINT_DEBUG("active_window: %s\n", lex->curr->str);
		free(lex->curr);
		next_token(f, lex, 0, 0);
	}
	k->windows[k->win_len] = NULL;
}

/*
 * parse_globals:
 * Read all global definitions located at the start of the keymap file and
 * set appropriate flags in the program's keymap struct.
 */
static void parse_globals(FILE *f, struct lexer *lex, struct keymap *k)
{
	while (lex->curr && lex->curr->tag == TOK_GDEF) {
		if (strcmp(lex->curr->str, "active_window") == 0) {
			k->flags |= KBM_ACTIVEWIN;
			if (next_token(f, lex, 0, 1) != 0)
				return;

			if (lex->curr->tag != TOK_STRLIT) {
				err_generic(lex, "expected string after "
				                 "active_window");
				free_token(lex->curr);
				lex->curr = NULL;
				return;
			}
			parse_windows(f, lex, k);
		}
	}
}

static int parse_key(FILE *f, struct lexer *lex,
                     uint64_t *retval, int failnext);
static int parse_mod(FILE *f, struct lexer *lex,
                     uint64_t *retval, int failnext);
static int parse_id(FILE *f, struct lexer *lex,
                    uint64_t *retval, int failnext);
static int parse_keynum(FILE *f, struct lexer *lex,
                        uint64_t *retval, int failnext);
static int parse_misc(FILE *f, struct lexer *lex,
                      uint64_t *retval, int failnext);

/*
 * parse_binding:
 * Read a complete keybinding declaration from f.
 * The format of a keybinding is KEY -> FUNC [ARGS].
 * Return a struct hotkey representing the binding.
 */
static struct hotkey *parse_binding(FILE *f, struct lexer *lex)
{
	uint64_t key, args;
	uint32_t flags;
	uint8_t op;

	key = args = flags = op = 0;
	if (parse_key(f, lex, &key, 1) != 0 || !validkey(&key, lex))
		return NULL;

	/* match the arrow following the key */
	if (lex->curr->tag != TOK_ARROW) {
		err_generic(lex, "expected '->' after key");
		return NULL;
	}
	if (next_token(f, lex, 1, 1) != 0)
		return NULL;

	/* match the hotkey operation */
	if (lex->curr->tag != TOK_FUNC) {
		err_generic(lex, "expected function after '->'");
		return NULL;
	}
	if (parse_func(f, lex, &op, &args) != 0)
		return NULL;

	if (lex->curr && lex->curr->tag == TOK_QUAL) {
		if (parse_qual(f, lex, &flags) != 0)
			return NULL;
	}

	return create_hotkey(key & 0xFFFFFFFF,
	                     (key >> 32) & 0xFFFFFFFF,
	                     op, args, flags);
}

/* parse_key: parse a key declaration and its modifiers */
static int parse_key(FILE *f, struct lexer *lex, uint64_t *retval, int failnext)
{
	/* valid nonalphanumeric key lexemes */
	static const char *misc_keys = "`-=[]\\;',./";

	if (lex->curr->tag == TOK_MOD) {
		if (parse_mod(f, lex, retval, failnext) != 0)
			return 1;
	} else if (lex->curr->tag == TOK_ID) {
		if (parse_id(f, lex, retval, failnext) != 0)
			return 1;
	} else if (lex->curr->tag == TOK_NUM) {
		if (parse_keynum(f, lex, retval, failnext) != 0)
			return 1;
	} else if (strchr(misc_keys, lex->curr->tag)) {
		if (parse_misc(f, lex, retval, failnext) != 0)
			return 1;
	} else {
		err_invkey(lex);
		return 1;
	}
	return 0;
}

/* parse_mod: process a token of type MOD */
static int parse_mod(FILE *f, struct lexer *lex, uint64_t *retval, int failnext)
{
	uint32_t *mods;

	mods = (uint32_t *)retval + 1;

	/* mark start of token for potential error reporting */
	strcpy(lex->err_line, lex->line);
	lex->err_num = lex->line_num;
	lex->err_pos = lex->err_line + CURR_START(lex);
	lex->err_len = lex->curr->len;

	switch (lex->curr->val) {
	case '^':
		SET_MODS(*mods, KBM_CTRL_MASK, lex);
		break;
	case '!':
		SET_MODS(*mods, KBM_SHIFT_MASK, lex);
		break;
	case '@':
		SET_MODS(*mods, KBM_SUPER_MASK, lex);
		break;
	case '~':
		SET_MODS(*mods, KBM_META_MASK, lex);
		break;
	default:
		return 1;
	}

	if (next_token(f, lex, 1, 1) != 0)
		return 1;

	if (lex->curr->tag == TOK_MOD)
		return parse_mod(f, lex, retval, failnext);

	return parse_key(f, lex, retval, failnext);
}

static int parse_id(FILE *f, struct lexer *lex, uint64_t *retval, int failnext)
{
	uint32_t *key, *mods;

	key = (uint32_t *)retval;
	mods = (uint32_t *)retval + 1;
	if (!(*key = lookup_keycode(lex->curr->str))) {
		err_invkey(lex);
		return 1;
	}

	if (K_ISMOD(*key)) {
		/* mark start of token for potential error reporting */
		strcpy(lex->err_line, lex->line);
		lex->err_num = lex->line_num;
		lex->err_pos = lex->err_line + CURR_START(lex);
		lex->err_len = lex->curr->len;
	}

	if (next_token(f, lex, 1, failnext) != 0)
		return failnext;

	if (K_ISMOD(*key) && lex->curr->tag == '-') {
		switch (*key) {
		case KEY_CTRL:
			SET_MODS(*mods, KBM_CTRL_MASK, lex);
			break;
		case KEY_SHIFT:
			SET_MODS(*mods, KBM_SHIFT_MASK, lex);
			break;
		case KEY_SUPER:
			SET_MODS(*mods, KBM_SUPER_MASK, lex);
			break;
		case KEY_META:
			SET_MODS(*mods, KBM_META_MASK, lex);
			break;
		}
		if (next_token(f, lex, 1, 1) != 0)
			return 1;

		return parse_key(f, lex, retval, failnext);
	}

	return 0;
}

static int parse_keynum(FILE *f, struct lexer *lex, uint64_t *retval, int failnext)
{
	uint32_t *key;

	key = (uint32_t *)retval;
	if (lex->curr->val + KEY_0 > KEY_9) {
		err_invkey(lex);
		return 1;
	}

	*key = lex->curr->val + KEY_0;
	if (next_token(f, lex, 1, failnext) != 0)
		return failnext;
	return 0;
}

static int parse_misc(FILE *f, struct lexer *lex, uint64_t *retval, int failnext)
{
	uint32_t *key;

	key = (uint32_t *)retval;
	switch (lex->curr->tag) {
	case '`':
		*key = KEY_BTICK;
		break;
	case '-':
		*key = KEY_MINUS;
		break;
	case '=':
		*key = KEY_EQUAL;
		break;
	case '[':
		*key = KEY_LSQBR;
		break;
	case ']':
		*key = KEY_RSQBR;
		break;
	case '\\':
		*key = KEY_BSLASH;
		break;
	case ';':
		*key = KEY_SEMIC;
		break;
	case '\'':
		*key = KEY_QUOTE;
		break;
	case ',':
		*key = KEY_COMMA;
		break;
	case '.':
		*key = KEY_PERIOD;
		break;
	case '/':
		*key = KEY_QUOTE;
		break;
	default:
		return 1;
	}
	if (next_token(f, lex, 1, failnext) != 0)
		return failnext;
	return 0;
}

/*
 * parse_func:
 * Parse an operation and its arguments from f.
 * Store opcode in op and arguments into args.
 */
static int parse_func(FILE *f, struct lexer *lex, uint8_t *op, uint64_t *args)
{
	uint32_t *x, *y;

	if (strcmp(lex->curr->str, "click") == 0) {
		*op = OP_CLICK;
		next_token(f, lex, 0, 0);
		return 0;
	}
	if (strcmp(lex->curr->str, "rclick") == 0) {
		*op = OP_RCLICK;
		next_token(f, lex, 0, 0);
		return 0;
	}
	if (strcmp(lex->curr->str, "jump") == 0) {
		*op = OP_JUMP;
		x = (uint32_t *)args;
		y = (uint32_t *)args + 1;
		if (next_token(f, lex, 0, 1) != 0 || parse_num(f, lex, x) != 0)
			return 1;
		if (next_token(f, lex, 1, 1) != 0 || parse_num(f, lex, y) != 0)
			return 1;
		next_token(f, lex, 1, 1);
		return 0;
	}
	if (strcmp(lex->curr->str, "key") == 0) {
		*op = OP_KEY;
		next_token(f, lex, 0, 1);
		return parse_key(f, lex, args, 0) != 0;
	}
	if (strcmp(lex->curr->str, "toggle") == 0) {
		*op = OP_TOGGLE;
		next_token(f, lex, 0, 0);
		return 0;
	}
	if (strcmp(lex->curr->str, "quit") == 0) {
		*op = OP_QUIT;
		next_token(f, lex, 0, 0);
		return 0;
	}
	if (strcmp(lex->curr->str, "exec") == 0) {
		*op = OP_EXEC;
		/* at least one argument is required */
		if (next_token(f, lex, 0, 1) != 0)
			return 1;
		if (lex->curr->tag != TOK_STRLIT) {
			err_generic(lex, "invalid token - expected a string");
			return 1;
		}
		return parse_exec(f, lex, args);
	}
	return 1;
}

/* parse_num: read a number from f into num */
static int parse_num(FILE *f, struct lexer *lex, uint32_t *num)
{
	int mult;

	if (lex->curr->tag != '-' && lex->curr->tag != TOK_NUM) {
		err_generic(lex, "invalid token - expected a number");
		return 1;
	}

	mult = 1;
	if (lex->curr->tag == '-') {
		mult = -1;
		if (next_token(f, lex, 1, 1) != 0)
			return 1;
		if (lex->curr->tag != TOK_NUM) {
			err_generic(lex, "invalid token - expected a number");
			return 1;
		}
	}

	*num = mult * lex->curr->val;
	return 0;
}

static int parse_exec(FILE *f, struct lexer *lex, uint64_t *retval)
{
#if defined(__linux__) || defined(__APPLE__)
	char **argv;
	size_t argc, allocsz;
#endif
#if defined(__CYGWIN__) || defined (__MINGW32__)
	char *args, *s, *t;
	size_t len, allocsz, litlen;
#endif

#if defined(__linux__) || defined(__APPLE__)
	/*
	 * We initially allocate space for 9 arguments as
	 * this is more than enough for 99% of use cases.
	 */
	allocsz = 10;
	argv = malloc(allocsz * sizeof *argv);
	argc = 0;

#ifdef __APPLE__
	/*
	 * On OS X, we attempt to launch the program as an app
	 * first, and then fall back to a normal exec call.
	 */
	argv[0] = "open";
	argv[1] = "-a";
	argc = 2;
#endif

	while (lex->curr && lex->curr->tag == TOK_STRLIT) {
		if (argc == allocsz - 1) {
			allocsz *= 2;
			argv = realloc(argv, allocsz);
		}
		argv[argc++] = lex->curr->str;
		free(lex->curr);
		next_token(f, lex, 0, 0);
	}
	argv[argc] = NULL;
	memcpy(retval, &argv, sizeof *retval);
#endif

#if defined(__CYGWIN__) || defined (__MINGW32__)
	allocsz = 4096;
	args = malloc(allocsz * sizeof *args);
	s = args;
	len = 0;

	while (lex->curr && lex->curr->tag == TOK_STRLIT) {
		/* the length of the string itself, without quotes */
		litlen = lex->curr->len - 2;
		if (len >= allocsz + litlen + 3) {
			/*
			 * This is guaranteed to provide enough space as
			 * the maximum length of a string literal is 1024.
			 */
			allocsz *= 2;
			args = realloc(args, allocsz);
		}
		/*
		 * Arguments including spaces are surrounded with quotes so
		 * they get processed as a single argument instead of multiple.
		 */
		if ((t = strchr(lex->curr->str, ' ')))
			*s++ = '"';
		strcpy(s, lex->curr->str);
		s += litlen;
		len += litlen;
		if (t)
			*s++ = '"';
		*s++ = ' ';
		free_token(lex->curr);
		next_token(f, lex, 0, 0);
	}
	/* get rid of final space */
	*--s = '\0';
	memcpy(retval, &args, sizeof *retval);
#endif

	return 0;
}

/* parse_qual: parse a hotkey qualifier */
static int parse_qual(FILE *f, struct lexer *lex, uint32_t *flags)
{
	if (strcmp(lex->curr->str, "norepeat") == 0)
		*flags |= KBM_NOREPEAT;

	next_token(f, lex, 0, 0);
	return 0;
}

/* validkey: check if a key-modifier combination is valid */
static int validkey(uint64_t *key, struct lexer *lex)
{
	uint32_t kc, mods, mask;

	kc = *(uint32_t *)key;
	mods = *((uint32_t *)key + 1);

	if (K_ISMOD(kc)) {
		switch (kc) {
		case KEY_CTRL:
			mask = KBM_CTRL_MASK;
			break;
		case KEY_SHIFT:
			mask = KBM_SHIFT_MASK;
			break;
		case KEY_SUPER:
			mask = KBM_SUPER_MASK;
			break;
		case KEY_META:
			mask = KBM_META_MASK;
			break;
		}
		if (CHECK_MASK(mods, mask)) {
			err_selfmod(lex);
			return 0;
		}
	}

	return 1;
}
