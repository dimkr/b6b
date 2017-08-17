/*
 * This file is part of b6b.
 *
 * Copyright 2017 Dima Krasner
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <locale.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "linenoise/linenoise.h"
#include <b6b.h>

#define B6B_SHELL \
	"{$global chld [$b6b $@]}\n" \
	"{$loop {" \
		"{$local stmt [$linenoise.read {>>> }]}\n" \
		"{$if [$str.len $stmt] {" \
			"{$try {" \
				"{$stdout writeln [$repr [$chld call [$list.new $stmt]]]}" \
			"} {" \
				"{$local err $_}\n" \
				"{$stdout write {Error: }}\n" \
				"{$stdout writeln [$repr $err]}" \
			"} {" \
				"{$try {" \
					"{$linenoise.add $stmt}" \
				"}}" \
			"}}" \
		"}}" \
	"}}"

static struct b6b_interp interp, *chld = NULL;

static struct b6b_interp *get_chld(void)
{
	struct b6b_obj *s, *v = NULL;

	if (!chld) {
		s = b6b_str_copy("chld", 4);
		if (b6b_likely(s)) {
			if (b6b_dict_get(interp.global->locals, s, &v) && v) {
				/* we assume the REPL user, whose statements run in a child
				 * interpreter, cannot replace the chld global of the outer
				 * interpreter with another object */
				chld = (struct b6b_interp *)v->priv;
			}

			b6b_destroy(s);
		}
	}

	return chld;
}

static void complete(const char *s, linenoiseCompletions *c)
{
	struct b6b_obj *ds[2], *o, *tok, *ctok;
	struct b6b_litem *li;
	size_t len;
	int i;

	len = strlen(s);
	if (!len)
		return;

	/* make s a b6b list and get the last item */
	o = b6b_str_copy(s, len);
	if (b6b_unlikely(!o))
		return;

	if (!b6b_as_list(o) || !b6b_list_first(o)) {
		b6b_destroy(o);
		return;
	}

	tok = b6b_ref(b6b_list_last(o)->o);

	if (b6b_unlikely(!b6b_as_str(tok)) ||
	    (tok->s[0] != '$') ||
	    !get_chld()) {
		b6b_unref(tok);
		b6b_destroy(o);
		return;
	}

	/* we offer local variables before global ones */
	ds[0] = chld->fg->curr->locals;
	ds[1] = chld->global->locals;

	for (i = 0; i < sizeof(ds) / sizeof(ds[0]); ++i) {
		if (!b6b_as_list(ds[i]))
			break;

		for (li = b6b_list_first(ds[i]); li; li = b6b_list_next(li)) {
			if (b6b_as_str(li->o) &&
			    (strncmp(li->o->s, &tok->s[1], tok->slen - 1) == 0)) {
				if (li->o->slen > SIZE_MAX - 2)
					continue;

				/* replace the last list item with the proposed one, prefixed
				 * with $ */
				b6b_unref(b6b_list_pop(o, b6b_list_last(o)));

				ctok = b6b_str_fmt("$%s", li->o->s);
				if (b6b_unlikely(!ctok))
					break;

				if (b6b_unlikely(!b6b_list_add(o, ctok))) {
					b6b_destroy(ctok);
					break;
				}
				b6b_unref(ctok);

				if (b6b_unlikely(!b6b_as_str(o)))
					break;

				linenoiseAddCompletion(c, o->s);
			}

			li = b6b_list_next(li);
			if (!li)
				break;
		}
	}

	b6b_unref(tok);
	b6b_destroy(o);
}

static char *hint(const char *s, int *color, int *bold)
{
	if (!s[0])
		return NULL;

	switch (s[strlen(&s[1])]) {
		case '{':
			*bold = 1;
			return "}";

		case '[':
			*bold = 1;
			return "]";
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	enum b6b_res res;
	int ret = EXIT_FAILURE;
	uint8_t opts = 0;

	do {
		switch (getopt(argc, argv, "ceux")) {
			case 'c':
				opts |= B6B_OPT_CMD;
				break;

			case 'e':
				opts |= B6B_OPT_RAISE;
				break;

			case 'u':
				opts |= B6B_OPT_NBF;
				break;

			case 'x':
				opts |= B6B_OPT_TRACE;
				break;

			case -1:
				goto done;

			default:
				return EXIT_FAILURE;
		}
	} while (1);

done:
	setlocale(LC_ALL, "");

	if (optind < argc) {
		if (!b6b_interp_new_argv(&interp,
		                         argc - optind,
		                         (const char **)&argv[optind],
		                         opts))
			return EXIT_FAILURE;

		if (opts & B6B_OPT_CMD)
			res = b6b_call_copy(&interp, argv[optind], strlen(argv[optind]));
		else
			res = b6b_source(&interp, argv[optind]);
	}
	else {
		if (!b6b_interp_new_argv(&interp, argc, (const char **)argv, opts))
			return EXIT_FAILURE;

		linenoiseSetCompletionCallback(complete);
		linenoiseSetHintsCallback(hint);

		res = b6b_call_copy(&interp, B6B_SHELL, sizeof(B6B_SHELL) - 1);
	}

	switch (res) {
		case B6B_OK:
			ret = EXIT_SUCCESS;
			break;

		case B6B_EXIT:
			if (b6b_obj_isnull(interp.fg->_))
				ret = EXIT_SUCCESS;
			else if (b6b_as_int(interp.fg->_) &&
			         (interp.fg->_->i >= INT_MIN) &&
			         (interp.fg->_->i <= INT_MAX))
				ret = (int)interp.fg->_->i;

		default:
			break;
	}

	b6b_interp_destroy(&interp);
	return ret;
}
