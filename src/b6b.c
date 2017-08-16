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
	struct b6b_obj *ds[2];
	struct b6b_litem *li;
	char *buf;
	int i;

	if (!get_chld())
		return;

	/* we offer local variables before global ones */
	ds[0] = chld->fg->curr->locals;
	ds[1] = chld->global->locals;

	for (i = 0; i < sizeof(ds) / sizeof(ds[0]); ++i) {
		if (!b6b_as_list(ds[i]))
			return;

		for (li = b6b_list_first(ds[i]); li; li = b6b_list_next(li)) {
			if ((s[0] == '$') &&
				b6b_as_str(li->o) &&
				(strncmp(li->o->s, &s[1], strlen(s) - 1) == 0)) {
				if (li->o->slen > SIZE_MAX - 2)
					return;

				buf = (char *)malloc(li->o->slen + 2);
				buf[0] = '$';
				strcpy(&buf[1], li->o->s);

				linenoiseAddCompletion(c, buf);

				free(buf);
			}

			li = b6b_list_next(li);
			if (!li)
				break;
		}
	}
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
