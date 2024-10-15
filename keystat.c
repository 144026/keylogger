#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "config.h"
#include "dict.h"

double timespec_delta(struct timespec *a, struct timespec *b)
{
	double ats = a->tv_sec + a->tv_nsec * 1e-9;
	double bts = b->tv_sec + b->tv_nsec * 1e-9;
	return bts - ats;
}

struct keylog {
	const char *path;
	FILE *log;
};

struct cursor {
	int line, col;
	int off;
	int tag_allowed;
};

struct token {
	int line, col;
	int off, len;
	int chopped;
	char name[64];
};

static int dump_tok(const struct keylog *kl, const struct token *tok)
{
	return fprintf(stdout, "%s:%d:%d: token: [%d+%d] '%s'\n", kl->path, tok->line, tok->col,
		tok->off, tok->len, tok->name);
}

static int hash_tok(struct dict *stat, const struct token *tok)
{
	struct dict_val *val = dict_get(stat, tok->name);

	if (val)
		val->number++;
	else
		dict_set(stat, strdup(tok->name), 1);

	return 0;
}

static int eat_tag_token(struct keylog *kl, struct cursor *cursor, struct token *tok)
{
	int c;

	while ((c = fgetc(kl->log)) != EOF) {
		if (c == '[') {
			ungetc(c, kl->log);
			cursor->tag_allowed = 0;
			return 0;
		}
		if (c == '\n') {
			cursor->line++;
			cursor->col = 0;
			cursor->tag_allowed = 1;
			tok->chopped = 1;
			return 1;
		}

		if (tok->len < sizeof(tok->name) - 1)
			tok->name[tok->len++] = c;
		else
			tok->chopped = 1;

		if (c == ']') {
			cursor->tag_allowed = 0;
			return tok->chopped;
		}

		cursor->col++;
		cursor->off++;
	}
	tok->chopped = 1;
	return 1;
}

#define static_strlen(str) (sizeof(str) - 1)

static int builtin_meta_tag(const struct token *tok)
{
	if (!strncmp(tok->name, "[keycount ", static_strlen("[keycount ")))
		return 1;

	if (!strncmp(tok->name, "[Keylogging begin]", static_strlen("[Keylogging begin]")))
		return 1;

	if (!strncmp(tok->name, "[Keylogging end]", static_strlen("[Keylogging end]")))
		return 1;

	return 0;
}

int compare_freq(const void *_a, const void *_b)
{
	const struct dict_item *a = _a;
	const struct dict_item *b = _b;
	return a->val.number - b->val.number;
}

static struct dict compute_keystat(struct keylog *kl) {
	int c;
	struct token tok;
	struct cursor cursor = {.line = 1, .col = 1, .off = 0};
	struct dict stat = dict_new();

	struct timespec begin, end;
	clock_gettime(CLOCK_MONOTONIC, &begin);

	while ((c = fgetc(kl->log)) != EOF) {
		switch (c) {
		case '\n':
			cursor.tag_allowed = 1;
			cursor.line++;
			cursor.col = 0;
			/* fall through */
		case '\t':
		case '\r':
		case '\f':
			goto next_tok;

		default:
			if (!isprint(c)) {
				fprintf(stderr, "character %#x is not printable\n", c);
				goto next_tok;
			}
			tok.line = cursor.line;
			tok.col = cursor.col;
			tok.off = cursor.off;
			tok.len = 1;
			tok.chopped = 0;
			tok.name[0] = c;
			if (cursor.tag_allowed && c == '[') {
				cursor.col++;
				cursor.off++;
				eat_tag_token(kl, &cursor, &tok);
			}
			tok.name[tok.len] = '\0';

			if (builtin_meta_tag(&tok))
				goto next_tok;
			break;
		}

		if (tok.chopped)
			fprintf(stderr, "%s:%d:%d: error: lost closing braket, token '%s'\n",
				kl->path, cursor.line, cursor.col, tok.name);

		// dump_tok(kl, &tok);
		hash_tok(&stat, &tok);

	next_tok:
		cursor.col++;
		cursor.off++;
	}

	clock_gettime(CLOCK_MONOTONIC, &end);
	fprintf(stderr, "Counting freq took %lf secs\n", timespec_delta(&begin, &end));

	qsort(stat.items.data, stat.items.size, stat.items.item_size, compare_freq);
	return stat;
}

int main(int argc, char *argv[])
{
	const char *path = NULL;
	FILE *log;
	struct dict stat;
	struct dict_item *item;

	if (argc <= 1) {
		path = CONFIG_LOGFILE_PATH;
	} else if (argc == 2) {
		path = argv[1];
	} else {
		fprintf(stderr, "usage: %s FILE\n", argv[0]);
		return 1;
	}

	if (0 == strncmp("-", path, 1)) {
		fprintf(stderr, "using stdin\n");
		log = stdin;
	} else {
		log = fopen(path, "rb");
	}

	if (!log) {
		fprintf(stderr, "fail to open %s\n", path);
		return 1;
	}

	stat = compute_keystat(&(struct keylog) {path, log});

	dict_fprint(&stat, stdout);
	fputc('\n', stdout);
	dict_discard(&stat);

	fclose(log);
	return 0;
}
