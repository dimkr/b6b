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

#include <b6b.h>

#define B6B_SHELL \
	"{$local chld [$interp.new $@]}\n" \
	"{$while 1 {" \
		"{$local stmt [$linenoise.read {>>> }]}\n" \
		"{$if [$str.len $stmt] {" \
			"{$try {" \
				"{$stdout writeln [$repr [$chld eval [$list.new $stmt]]]}" \
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

int main(int argc, char *argv[]) {
	struct b6b_interp interp;
	enum b6b_res res;
	int opt, ret = EXIT_FAILURE;
	uint8_t opts = 0;

	do {
		opt = getopt(argc, argv, "ceux");
		switch (opt) {
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
		                         argc - optind - 1,
		                         (const char **)&argv[optind + 1],
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

		res = b6b_call_copy(&interp, B6B_SHELL, sizeof(B6B_SHELL) - 1);
	}

	switch (res) {
		case B6B_OK:
			ret = EXIT_SUCCESS;
			break;

		case B6B_EXIT:
			if (b6b_obj_isnull(interp.fg->_))
				ret = EXIT_SUCCESS;
			else if (b6b_as_num(interp.fg->_) &&
			         (interp.fg->_->n >= INT_MIN) &&
			         (interp.fg->_->n <= INT_MAX))
				ret = (int)interp.fg->_->n;

		default:
			break;
	}

	b6b_interp_destroy(&interp);
	return ret;
}
