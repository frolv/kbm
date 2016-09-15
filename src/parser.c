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
#define MAX_STRING	1024

#define CURR_IND (pos - line)
#define CURR_START (CURR_IND - curr->len)

/* print a nice looking error message */
#define PUTERR(ind, fmt, ...) \
	fprintf(stderr, KWHT "%s:%u:%ld: " KNRM \
		KRED "error: " KNRM fmt, \
		file_path, line_num, (ind) + 1, \
		##__VA_ARGS__)

#define PUTWARN(ind, fmt, ...) \
	fprintf(stderr, KWHT "%s:%u:%ld: " KNRM \
		KMAG "warning: " KNRM fmt, \
		file_path, line_num, (ind) + 1, \
		##__VA_ARGS__)

#define GET_OFFSET(offset) \
	(((CURR_IND + (offset)) < 0) \
	 ? 0 : (CURR_IND + (offset)))

enum {
	TOK_NUM = 0x100,
	TOK_ID,
	TOK_ARROW,
	TOK_FUNC,
	TOK_STRLIT,
	TOK_MOD
};

struct token {
	int tag;		/* type of the token */
	size_t len;		/* length of token's lexeme */
	union {
		int val;	/* each token has either a numeric */
		char *str;	/* or string value associated with it */
	};
	UT_hash_handle hh;	/* handle for hashtable */
};

static const char *file_path;
static unsigned int line_num;
static char line[BUFFER_SIZE];

/* the current character processed */
static char *pos;

/* token currently being processed */
static struct token *curr;

/* hash table of reserved words */
static struct token *reserved;

static FILE *open_file(const char *path);
static struct token *scan(FILE *f);
static struct token *read_str(FILE *f);
static struct token *create_token(int tag, void *info);
static void free_token(struct token *t);
static void reserve(struct token *word);
static char *next_line(FILE *f);
static int next_token(FILE *f, struct token **ret, int free, int err);

static struct hotkey *parse_binding(FILE *f);
static int parse_key(FILE *f, uint64_t *retval);
static int parse_mod(FILE *f, uint64_t *retval);
static int parse_id(FILE *f, uint64_t *retval);

/* error/warning message functions */
static void print_segment(size_t start, size_t end);
static void print_carat(size_t nspace, size_t len, const char *colour);
static void print_token(const struct token *t, const char *colour);

static void warn_duplicate(void);
static void err_unterm(void);

/*
 * parse_file:
 * Read the file at path, if it is accessible.
 * Process the keybindings in the file and return
 * a list of struct hotkeys representing them.
 */
struct hotkey *parse_file(const char *path)
{
	struct hotkey *head, *hk;
	struct token *t, *tmp;
	FILE *f;

	head = NULL;
	file_path = path;

	if (strcmp(path, "-") == 0) {
		f = stdin;
		file_path = "<stdin>";
	} else if (!(f = open_file(path))) {
		exit(1);
	}

	reserved = NULL;
	reserve(create_token(TOK_FUNC, "click"));
	reserve(create_token(TOK_FUNC, "rclick"));
	reserve(create_token(TOK_FUNC, "jump"));
	reserve(create_token(TOK_FUNC, "key"));
	reserve(create_token(TOK_FUNC, "toggle"));
	reserve(create_token(TOK_FUNC, "quit"));
	reserve(create_token(TOK_FUNC, "exec"));

	line_num = 0;
	if (!(pos = next_line(f)))
		return head;

	/* grab the first token */
	next_token(f, &curr, 0, 0);
	while (1) {
		if (!(hk = parse_binding(f)))
			exit(1);
		PRINT_DEBUG("hotkey parsed: %s\n",
				keystr(hk->kbm_code, hk->kbm_modmask));
		add_hotkey(&head, hk);
	}

	/* free hash table contents */
	HASH_ITER(hh, reserved, t, tmp) {
		HASH_DEL(reserved, t);
		free_token(t);
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
	int i;
	char buf[BUFFER_SIZE];
	struct token *t;

	/* skip over whitespace */
	for (;; ++pos) {
		while (!*pos || *pos == '\n' || *pos == '#') {
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
		return t ? t : create_token(TOK_ID, &buf);
	}
	if (*pos == '-') {
		if (*++pos == '>') {
			++pos;
			return create_token(TOK_ARROW, NULL);
		}
		return create_token('-', NULL);
	}
	if (*pos == '^' || *pos == '~' || *pos == '!' || *pos == '@') {
		i = *pos++;
		return create_token(TOK_MOD, &i);
	}
	if (*pos == '"')
		return read_str(f);

	return create_token(*pos++, NULL);
}

/* read_str: read a string literal from f, return token containing it */
static struct token *read_str(FILE *f)
{
	int quote, i, start;
	char buf[MAX_STRING];

	quote = *pos++;
	for (i = 0; i < MAX_STRING - 1; ++i) {
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

	if (i == MAX_STRING - 1) {
		PUTWARN(CURR_IND, "string literal exceeding "
				"%d characters truncated\n", MAX_STRING - 1);
		start = GET_OFFSET(-79);
		print_segment(start, CURR_IND);
		printf(KMAG "%c" KNRM "\n", quote);
		print_carat(CURR_IND - start, 1, KMAG);

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
	int i;

	t = malloc(sizeof(*t));
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
	case TOK_STRLIT:
		t->str = strdup((char *)info);
		t->len = strlen(t->str);
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

/*
 * next_token:
 * Read the next token from f and store pointer to it in ret.
 * The existing token in ret is freed if free is set.
 * With err, an error message is printed if no token can be read.
 */
static int next_token(FILE *f, struct token **ret, int free, int err)
{
	char buf[BUFFER_SIZE];
	size_t start;

	if (free && *ret)
		free_token(*ret);
	if (err)
		strcpy(buf, line);

	if (!(*ret = scan(f))) {
		if (err) {
			PUTERR(-1L, "unexpected end of file when parsing\n");
			pos = strchr(line, '\n');
			start = GET_OFFSET(-79);
			print_segment(start, CURR_IND);
			putc('\n', stderr);
			print_carat(CURR_IND - start, 1, KRED);
			fprintf(stderr, "last statement here:\n%s", buf);
		}
		return 1;
	}
	return 0;
}

/*
 * parse_binding:
 * Read a complete keybinding declaration from f.
 * The format of a keybinding is KEY -> FUNC [ARGS].
 * Return a struct hotkey representing the binding.
 */
static struct hotkey *parse_binding(FILE *f)
{
	size_t start, end;
	uint64_t key;

	key = 0;
	if (parse_key(f, &key) != 0)
		return NULL;

	/* match the arrow following the key */
	if (curr->tag != TOK_ARROW) {
		PUTERR(CURR_START, "expected '->' after key\n");
		start = GET_OFFSET(-40);
		end = start + 80;
		print_segment(start, CURR_START);
		print_token(curr, KRED);
		print_segment(CURR_IND, end);
		if (end < strlen(line))
			putc('\n', stderr);
		print_carat(CURR_IND - start - curr->len, curr->len, KRED);
		return NULL;
	}
	if (next_token(f, &curr, 1, 1) != 0)
		return NULL;

	/* match the hotkey operation */
	if (curr->tag != TOK_FUNC) {
		PUTERR(CURR_START, "expected function after '->'\n");
		start = GET_OFFSET(-40);
		end = start + 80;
		print_segment(start, CURR_START);
		print_token(curr, KRED);
		print_segment(CURR_IND, end);
		if (end < strlen(line))
			putc('\n', stderr);
		print_carat(CURR_IND - start - curr->len, curr->len, KRED);
		return NULL;
	}
	if (next_token(f, &curr, 1, 1) != 0)
		return NULL;

	return create_hotkey(key & 0xFFFFFFFF, (key >> 32) & 0xFFFFFFFF, 0, 0);
}

/* parse_key: parse a key declaration and its modifiers */
static int parse_key(FILE *f, uint64_t *retval)
{
	/* valid nonalphanumeric key lexemes */
	static const char *misc_keys = "`-=[]\\;',./";
	size_t start, end;

	if (curr->tag == TOK_MOD) {
		if (parse_mod(f, retval) != 0)
			return 1;

		return parse_key(f, retval);
	} else if (curr->tag == TOK_ID) {
		if (parse_id(f, retval) == 1)
			return 1;
	} else if (strchr(misc_keys, curr->tag)) {
	} else {
		PUTERR(CURR_START, "invalid token - expected a key\n");
		start = GET_OFFSET(-40);
		end = start + 80;
		print_segment(start, CURR_START);
		print_token(curr, KRED);
		print_segment(CURR_IND, end);
		if (end < strlen(line))
			putc('\n', stderr);
		print_carat(CURR_IND - start - curr->len, curr->len, KRED);
		return 1;
	}
	return 0;
}

/* parse_mod: process a token of type MOD */
static int parse_mod(FILE *f, uint64_t *retval)
{
	uint32_t *mods;

	mods = (uint32_t *)retval + 1;
	switch (curr->val) {
	case '^':
		if (CHECK_MOD(*mods, KBM_CTRL_MASK))
			warn_duplicate();
		*mods |= KBM_CTRL_MASK;
		break;
	case '!':
		if (CHECK_MOD(*mods, KBM_SHIFT_MASK))
			warn_duplicate();
		*mods |= KBM_SHIFT_MASK;
		break;
	case '@':
		if (CHECK_MOD(*mods, KBM_SUPER_MASK))
			warn_duplicate();
		*mods |= KBM_SUPER_MASK;
		break;
	case '~':
		if (CHECK_MOD(*mods, KBM_META_MASK))
			warn_duplicate();
		*mods |= KBM_META_MASK;
		break;
	default:
		return 1;
	}

	if (next_token(f, &curr, 1, 1) != 0)
		return 1;

	if (curr->tag == TOK_MOD)
		return parse_mod(f, retval);

	return 0;
}

static int parse_id(FILE *f, uint64_t *retval)
{
	uint32_t *key;

	key = (uint32_t *)retval;
	if (strcmp(curr->str, "E") == 0) {
		*key = KEY_E;
	}

	if (next_token(f, &curr, 1, 1) != 0)
		return 1;

	return 0;
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

static void print_token(const struct token *t, const char *colour)
{
	char *s;

	fprintf(stderr, "%s", colour);
	for (s = pos - t->len; s < pos; ++s)
		putc(*s, stderr);
	fprintf(stderr, KNRM);
}

static void err_unterm(void)
{
	size_t start;

	PUTERR(CURR_IND, "unterminated string literal\n");
	start = GET_OFFSET(-79);
	print_segment(start, CURR_IND);
	putc('\n', stderr);
	print_carat(CURR_IND - start, 1, KRED);
}

static void warn_duplicate(void)
{
	size_t start, end;

	PUTWARN(CURR_IND, "duplicate modifier declaration\n");
	start = GET_OFFSET(-40);
	end = start + 80;
	print_segment(start, CURR_START);
	print_token(curr, KMAG);
	print_segment(CURR_IND, end);
	if (end < strlen(line))
		putc('\n', stderr);
	print_carat(CURR_IND - start - curr->len, curr->len, KMAG);
}
