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

int main(int argc, char *argv[]) {
	struct b6b_interp interp;
	int ret = EXIT_FAILURE;

	if (argc == 2) {
		setlocale(LC_ALL, "");

		if (b6b_interp_new(&interp, argc, (const char **)argv)) {
			switch (b6b_source(&interp, argv[1])) {
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
		}
	}
	else
		fprintf(stderr, "Usage: %s PATH\n", argv[0]);

	return ret;
}
