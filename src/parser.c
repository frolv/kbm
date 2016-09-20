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

#define MAX_STRING 1024U

#define CURR_IND (pos - line)
#define CURR_START (CURR_IND - curr->len)

/* print a nice looking error message */
#define PUTERR(lnum, ind, fmt, ...) \
	fprintf(stderr, KWHT "%s:%u:%ld: " KNRM \
		KRED "error: " KNRM fmt, \
		file_path, lnum, (ind) + 1, \
		##__VA_ARGS__)

#define PUTWARN(lnum, ind, fmt, ...) \
	fprintf(stderr, KWHT "%s:%u:%ld: " KNRM \
		KMAG "warning: " KNRM fmt, \
		file_path, lnum, (ind) + 1, \
		##__VA_ARGS__)

#define PUTNOTE(lnum, ind, fmt, ...) \
	fprintf(stderr, KWHT "%s:%u:%ld: " KNRM \
		KBLU "note: " KNRM fmt, \
		file_path, lnum, (ind) + 1, \
		##__VA_ARGS__)

#define SUB_TO_ZERO(a,b) (((a) + (b) < 0) ? 0 : (a) + (b))

enum {
	TOK_NUM = 0x100,
	TOK_ID,
	TOK_ARROW,
	TOK_FUNC,
	TOK_STRLIT,
	TOK_MOD
};

struct token {
	int		tag;	/* type of the token */
	size_t		len;	/* length of token's lexeme */
	union {
		int	val;	/* each token has either a numeric */
		char	*str;	/* or string value associated with it */
	};
	UT_hash_handle	hh;	/* handle for hashtable */
};

static const char *file_path;

/* number of current line in file */
static unsigned int line_num;
/* current line being read */
static char line[BUFFER_SIZE];
/* the current character processed */
static char *pos;

static unsigned int err_num;
static unsigned int err_len;
static char err_line[BUFFER_SIZE];
static char *err_pos;

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
static void free_reserved(void);
static char *next_line(FILE *f);
static int next_token(FILE *f, struct token **ret, int free, int err);

static struct hotkey *parse_binding(FILE *f);
static int parse_key(FILE *f, uint64_t *retval, int failnext);
static int parse_mod(FILE *f, uint64_t *retval, int failnext);
static int parse_id(FILE *f, uint64_t *retval, int failnext);
static int parse_keynum(FILE *f, uint64_t *retval, int failnext);
static int parse_misc(FILE *f, uint64_t *retval, int failnext);
static int parse_func(FILE *f, uint8_t *op, uint64_t *args);
static int parse_num(FILE *f, uint32_t *num);
static int parse_exec(FILE *f, uint64_t *retval);
static void set_mods(uint32_t *mods, uint32_t mask);

/* error/warning message functions */
static void print_segment(const char *buf, size_t start,
			  size_t end, const char *colour);
static void print_caret(size_t nspace, size_t len, const char *colour);
static void print_token(const struct token *t, const char *colour);

static void err_unterm(void);
static void err_generic(const char *err);
static void err_invkey(void);
static void note_duplicate(void);

/*
 * parse_file:
 * Read the file at path, if it is accessible.
 * Process the keybindings in the file and return
 * a list of struct hotkeys representing them.
 */
struct hotkey *parse_file(const char *path)
{
	struct hotkey *head, *hk;
	FILE *f;

	head = NULL;
	file_path = path;

	if (strcmp(path, "-") == 0) {
		f = stdin;
		file_path = "<stdin>";
	} else if (!(f = open_file(path))) {
		keymap_free();
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
	while (curr) {
		if (!(hk = parse_binding(f))) {
			if (curr && (curr->tag == TOK_ID ||
						curr->tag == TOK_STRLIT))
				free_token(curr);
			free_reserved();
			keymap_free();
			if (head)
				free_keys(head);
			fclose(f);
			exit(1);
		}
		PRINT_DEBUG("hotkey parsed: %s\n",
				keystr(hk->kbm_code, hk->kbm_modmask));
		add_hotkey(&head, hk);
	}

	free_reserved();
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
	int quote;
	size_t i, start;
	char buf[MAX_STRING];

	/* record where the string literal started */
	strcpy(err_line, line);
	err_num = line_num;
	err_pos = err_line + CURR_IND;
	err_len = 1;

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
		PUTWARN(line_num, CURR_IND, "string literal exceeding "
				"%d characters truncated\n", MAX_STRING - 1);
		start = SUB_TO_ZERO(CURR_IND, -79);
		print_segment(line, start, CURR_IND, NULL);
		printf(KMAG "%c" KNRM "\n", quote);
		print_caret(CURR_IND - start, 1, KMAG);

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

/* free_reserved: free reserved key hash table contents */
static void free_reserved(void)
{
	struct token *t, *tmp;

	HASH_ITER(hh, reserved, t, tmp) {
		HASH_DEL(reserved, t);
		free_token(t);
	}
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
	size_t start, end, col, err_end;

	if (free && *ret)
		free_token(*ret);
	if (err) {
		strcpy(err_line, line);
		err_num = line_num;
		err_pos = err_line + CURR_START;
		err_len = curr->len;
		col = err_pos - err_line;
		err_end = col + err_len;
	}

	if (!(*ret = scan(f))) {
		if (err) {
			PUTERR(line_num, -1L, "unexpected EOF when parsing\n");
			pos = strchr(line, '\n');
			start = SUB_TO_ZERO(CURR_IND, -79);
			print_segment(line, start, CURR_IND, NULL);
			putc('\n', stderr);
			print_caret(CURR_IND - start, 1, KRED);
			PUTNOTE(err_num, col, "last statement here\n");
			end = strlen(err_line);
			start = SUB_TO_ZERO((int)end, -79);
			print_segment(err_line, start, col, NULL);
			print_segment(err_line, col, err_end, KBLU);
			print_segment(err_line, err_end, end, NULL);
			print_caret(col, err_len, KBLU);
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
	uint64_t key, args;
	uint8_t op;

	key = args = op = 0;
	if (parse_key(f, &key, 1) != 0)
		return NULL;

	/* match the arrow following the key */
	if (curr->tag != TOK_ARROW) {
		err_generic("expected '->' after key");
		return NULL;
	}
	if (next_token(f, &curr, 1, 1) != 0)
		return NULL;

	/* match the hotkey operation */
	if (curr->tag != TOK_FUNC) {
		err_generic("expected function after '->'");
		return NULL;
	}
	if (parse_func(f, &op, &args) != 0)
		return NULL;

	return create_hotkey(key & 0xFFFFFFFF,
			(key >> 32) & 0xFFFFFFFF,
			op, args);
}

/* parse_key: parse a key declaration and its modifiers */
static int parse_key(FILE *f, uint64_t *retval, int failnext)
{
	/* valid nonalphanumeric key lexemes */
	static const char *misc_keys = "`-=[]\\;',./";

	if (curr->tag == TOK_MOD) {
		if (parse_mod(f, retval, failnext) != 0)
			return 1;
	} else if (curr->tag == TOK_ID) {
		if (parse_id(f, retval, failnext) != 0)
			return 1;
	} else if (curr->tag == TOK_NUM) {
		if (parse_keynum(f, retval, failnext) != 0)
			return 1;
	} else if (strchr(misc_keys, curr->tag)) {
		if (parse_misc(f, retval, failnext) != 0)
			return 1;
	} else {
		err_invkey();
		return 1;
	}
	return 0;
}

/* parse_mod: process a token of type MOD */
static int parse_mod(FILE *f, uint64_t *retval, int failnext)
{
	uint32_t *mods;

	mods = (uint32_t *)retval + 1;

	/* mark start of token for potential error reporting */
	strcpy(err_line, line);
	err_num = line_num;
	err_pos = err_line + CURR_START;
	err_len = curr->len;

	switch (curr->val) {
	case '^':
		set_mods(mods, KBM_CTRL_MASK);
		break;
	case '!':
		set_mods(mods, KBM_SHIFT_MASK);
		break;
	case '@':
		set_mods(mods, KBM_SUPER_MASK);
		break;
	case '~':
		set_mods(mods, KBM_META_MASK);
		break;
	default:
		return 1;
	}

	if (next_token(f, &curr, 1, 1) != 0)
		return 1;

	if (curr->tag == TOK_MOD)
		return parse_mod(f, retval, failnext);

	return parse_key(f, retval, failnext);
}

static int parse_id(FILE *f, uint64_t *retval, int failnext)
{
	uint32_t *key, *mods;

	key = (uint32_t *)retval;
	mods = (uint32_t *)retval + 1;
	if (!(*key = lookup_keycode(curr->str))) {
		err_invkey();
		return 1;
	}

	if (K_ISMOD(*key)) {
		/* mark start of token for potential error reporting */
		strcpy(err_line, line);
		err_num = line_num;
		err_pos = err_line + CURR_START;
		err_len = curr->len;
	}

	if (next_token(f, &curr, 1, failnext) != 0)
		return failnext;

	if (K_ISMOD(*key) && curr->tag == '-') {
		switch (*key) {
		case KEY_CTRL:
			set_mods(mods, KBM_CTRL_MASK);
			break;
		case KEY_SHIFT:
			set_mods(mods, KBM_SHIFT_MASK);
			break;
		case KEY_SUPER:
			set_mods(mods, KBM_SUPER_MASK);
			break;
		case KEY_META:
			set_mods(mods, KBM_META_MASK);
			break;
		}
		if (next_token(f, &curr, 1, 1) != 0)
			return 1;

		return parse_key(f, retval, failnext);
	}

	return 0;
}

static int parse_keynum(FILE *f, uint64_t *retval, int failnext)
{
	uint32_t *key;

	key = (uint32_t *)retval;
	if (curr->val + KEY_0 > KEY_9) {
		err_invkey();
		return 1;
	}

	*key = curr->val + KEY_0;
	if (next_token(f, &curr, 1, failnext) != 0)
		return failnext;
	return 0;
}

static int parse_misc(FILE *f, uint64_t *retval, int failnext)
{
	uint32_t *key;

	key = (uint32_t *)retval;
	switch (curr->tag) {
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
	if (next_token(f, &curr, 1, failnext) != 0)
		return failnext;
	return 0;
}

/*
 * parse_func:
 * Parse an operation and its arguments from f.
 * Store opcode in op and arguments into args.
 */
static int parse_func(FILE *f, uint8_t *op, uint64_t *args)
{
	uint32_t *x, *y;

	if (strcmp(curr->str, "click") == 0) {
		*op = OP_CLICK;
		next_token(f, &curr, 0, 0);
		return 0;
	}
	if (strcmp(curr->str, "rclick") == 0) {
		*op = OP_RCLICK;
		next_token(f, &curr, 0, 0);
		return 0;
	}
	if (strcmp(curr->str, "jump") == 0) {
		*op = OP_JUMP;
		x = (uint32_t *)args;
		y = (uint32_t *)args + 1;
		if (next_token(f, &curr, 0, 1) != 0 || parse_num(f, x) != 0)
			return 1;
		if (next_token(f, &curr, 1, 1) != 0 || parse_num(f, y) != 0)
			return 1;
		next_token(f, &curr, 1, 1);
		return 0;
	}
	if (strcmp(curr->str, "key") == 0) {
		*op = OP_KEY;
		next_token(f, &curr, 0, 1);
		return parse_key(f, args, 0) != 0;
	}
	if (strcmp(curr->str, "toggle") == 0) {
		*op = OP_TOGGLE;
		next_token(f, &curr, 0, 0);
		return 0;
	}
	if (strcmp(curr->str, "quit") == 0) {
		*op = OP_QUIT;
		next_token(f, &curr, 0, 0);
		return 0;
	}
	if (strcmp(curr->str, "exec") == 0) {
		*op = OP_EXEC;
		/* at least one argument is required */
		if (next_token(f, &curr, 0, 1) != 0)
			return 1;
		if (curr->tag != TOK_STRLIT) {
			err_generic("invalid token - expected a string");
			return 1;
		}
		return parse_exec(f, args);
	}
	return 1;
}

/* parse_num: read a number from f into num */
static int parse_num(FILE *f, uint32_t *num)
{
	int mult;

	if (curr->tag != '-' && curr->tag != TOK_NUM) {
		err_generic("invalid token - expected a number");
		return 1;
	}

	mult = 1;
	if (curr->tag == '-') {
		mult = -1;
		if (next_token(f, &curr, 1, 1) != 0)
			return 1;
		if (curr->tag != TOK_NUM) {
			err_generic("invalid token - expected a number");
			return 1;
		}
	}

	*num = mult * curr->val;
	return 0;
}

static int parse_exec(FILE *f, uint64_t *retval)
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
	argv = malloc(allocsz * sizeof(*argv));
	argc = 0;

	while (curr && curr->tag == TOK_STRLIT) {
		if (argc == allocsz - 1) {
			allocsz *= 2;
			argv = realloc(argv, allocsz);
		}
		argv[argc++] = curr->str;
		free(curr);
		next_token(f, &curr, 0, 0);
	}
	argv[argc] = NULL;
	memcpy(retval, &argv, sizeof(*retval));
#endif

#if defined(__CYGWIN__) || defined (__MINGW32__)
	allocsz = 4096;
	args = malloc(allocsz * sizeof(*args));
	s = args;
	len = 0;

	while (curr && curr->tag == TOK_STRLIT) {
		/* the length of the string literal itself, without quotes */
		litlen = curr->len - 2;
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
		if ((t = strchr(curr->str, ' ')))
			*s++ = '"';
		strcpy(s, curr->str);
		s += litlen;
		len += litlen;
		if (t)
			*s++ = '"';
		*s++ = ' ';
		free_token(curr);
		next_token(f, &curr, 0, 0);
	}
	/* get rid of final space */
	*--s = '\0';
	memcpy(retval, &args, sizeof(*retval));
#endif

	return 0;
}

/* set_mods: set bitmask mask to mods with duplicate notice */
static void set_mods(uint32_t *mods, uint32_t mask)
{
	if (CHECK_MASK(*mods, mask))
		note_duplicate();
	*mods |= mask;
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
		fprintf(stderr, colour);
	for (i = start; i < end; ++i)
		putc(buf[i], stderr);
	if (colour)
		fprintf(stderr, KNRM);
}

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
	size_t start, end, col;

	PUTERR(line_num, CURR_IND, "unterminated string literal\n");
	start = SUB_TO_ZERO(CURR_IND, -79);
	print_segment(line, start, CURR_IND, NULL);
	putc('\n', stderr);
	print_caret(CURR_IND - start, 1, KRED);

	if (err_num != line_num) {
		col = err_pos - err_line;
		start = SUB_TO_ZERO((int)col, -79);
		end = start + 80;
		PUTNOTE(err_num, col, "started here\n");
		print_segment(err_line, start, col, NULL);
		print_segment(err_line, col, end, KBLU);
		if (end < strlen(err_line))
			putc('\n', stderr);
		else
			end = strlen(err_line);
		print_caret(col, end - col, KBLU);
	}
}

static void err_generic(const char *err)
{
	size_t start, end;

	PUTERR(line_num, CURR_START, "%s\n", err);
	start = SUB_TO_ZERO(CURR_IND, -40);
	end = start + 80;
	if (start > CURR_START)
		start = CURR_START;
	if (end < (size_t)CURR_IND)
		end = CURR_IND;
	print_segment(line, start, CURR_START, NULL);
	print_token(curr, KRED);
	print_segment(line, CURR_IND, end, NULL);
	if (end < strlen(line))
		putc('\n', stderr);
	print_caret(CURR_IND - start - curr->len, curr->len, KRED);
}

static void err_invkey(void)
{
	size_t start, end;

	if (curr->tag == TOK_NUM)
		PUTERR(line_num, CURR_START, "invalid key '%d'\n", curr->val);
	else if (curr->tag == TOK_ID || curr->tag == TOK_FUNC
			|| curr->tag == TOK_STRLIT)
		PUTERR(line_num, CURR_START, "invalid key '%s'\n", curr->str);
	else if (curr->tag == TOK_ARROW)
		PUTERR(line_num, CURR_START, "invalid key '->'\n");
	else
		PUTERR(line_num, CURR_START, "invalid key '%c'\n", curr->tag);
	start = SUB_TO_ZERO(CURR_IND, -40);
	end = start + 80;
	if (start > CURR_START)
		start = CURR_START;
	if (end < (size_t)CURR_IND)
		end = CURR_IND;
	print_segment(line, start, CURR_START, NULL);
	print_token(curr, KRED);
	print_segment(line, CURR_IND, end, NULL);
	if (end < strlen(line))
		putc('\n', stderr);
	print_caret(CURR_IND - start - curr->len, curr->len, KRED);
}

static void note_duplicate(void)
{
	size_t start, end, err_end;
	long col;

	col = err_pos - err_line;
	start = SUB_TO_ZERO(col, -40);
	end = start + 80;
	err_end = col + err_len;

	PUTNOTE(line_num, col, "duplicate modifier declaration\n");
	print_segment(err_line, start, col, NULL);
	print_segment(err_line, col, err_end, KBLU);
	print_segment(err_line, err_end, end, NULL);
	if (end < strlen(err_line))
		putc('\n', stderr);
	print_caret(col, err_len, KBLU);
}
