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

#include <b6b.h>

#define B6B_SHELL \
	"{$while 1 {" \
		"{$local stmt [$linenoise.read {>>> }]}\n" \
		"{$if [$str.len $stmt] {" \
			"{$try {" \
				"{$stdout writeln [$call [$list.new $stmt]]}" \
			"} {" \
				"{$local err $_}\n" \
				"{$stdout write {Error: }}\n" \
				"{$stdout write $err}\n" \
				"{$stdout write {\n}}" \
			"} {" \
				"{$try {" \
					"{$linenoise.add $stmt}" \
				"}}" \
			"}}" \
		"}}" \
	"}}"

int main(int argc, char *argv[]) {
	struct b6b_interp interp;
	struct b6b_obj *s;
	enum b6b_res res;
	int ret = EXIT_FAILURE;

	if (argc == 2) {
		setlocale(LC_ALL, "");

		if (!b6b_interp_new(&interp, argc, (const char **)argv))
			return EXIT_FAILURE;

		res = b6b_source(&interp, argv[1]);
	}
	else if (argc == 1) {
		setlocale(LC_ALL, "");

		if (!b6b_interp_new(&interp, argc, (const char **)argv))
			return EXIT_FAILURE;

		s = b6b_str_copy(B6B_SHELL, sizeof(B6B_SHELL) - 1);
		if (!s) {
			b6b_interp_destroy(&interp);
			return EXIT_FAILURE;
		}

		res = b6b_call(&interp, s);
		b6b_unref(s);
	} else {
		fprintf(stderr, "Usage: %s [PATH]\n", argv[0]);
		return EXIT_FAILURE;
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
			         (interp.fg->_->n <= INT_MAX)) {
				ret = (int)interp.fg->_->n;
			}

		default:
			break;
	}

	b6b_interp_destroy(&interp);
	return ret;
}
