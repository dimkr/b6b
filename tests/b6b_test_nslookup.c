/*
 * This file is part of b6b.
 *
 * Copyright 2018 Dima Krasner
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

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <b6b.h>

int main()
{
	struct b6b_interp interp;
	const struct b6b_litem *li;

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$nslookup}", 11) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$nslookup abc}", 15) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$nslookup 0.0.0.0}", 19) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == sizeof("0.0.0.0") - 1);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "0.0.0.0") == 0);
	assert(!b6b_list_next(b6b_list_first(interp.fg->_)));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$nslookup 127.0.0.1}", 21) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == sizeof("127.0.0.1") - 1);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "127.0.0.1") == 0);
	assert(!b6b_list_next(b6b_list_first(interp.fg->_)));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$nslookup ::1}", 15) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == sizeof("::1") - 1);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "::1") == 0);
	assert(!b6b_list_next(b6b_list_first(interp.fg->_)));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$nslookup localhost}", 21) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	b6b_list_foreach(interp.fg->_, li) {
		assert(b6b_as_str(li->o));
		if (li->o->slen == sizeof("127.0.0.1") - 1)
			assert(strcmp(li->o->s, "127.0.0.1") == 0);
		else {
			assert(li->o->slen == sizeof("::1") - 1);
			assert(strcmp(li->o->s, "::1") == 0);
		}
	}
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
