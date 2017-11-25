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

#include <stdlib.h>
#include <assert.h>
#include <locale.h>

#include <b6b.h>

int main()
{
	struct b6b_interp interp;

	setlocale(LC_ALL, "");

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d}", 10) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d encode}", 17) == B6B_ERR);
	b6b_interp_destroy(&interp);

	/* decoding of a Hebrew word should succeed */
	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d decode}", 17) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == 2);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "\xd7\xa9") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->slen == 2);
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "\xd7\x9c") == 0);
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_str(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o->slen == 2);
	assert(strcmp(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o->s, "\xd7\x95") == 0);
	assert(b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))));
	assert(b6b_as_str(b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))))->o->slen == 2);
	assert(strcmp(b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))))->o->s, "\xd7\x9d") == 0);
	assert(!b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))))));
	b6b_interp_destroy(&interp);

	/* decoding of a string containing a \0 should fail */
	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{\xd7\xa9\xd7\x9c\x00\xd7\x9d decode}", 16) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{\xd7\xa9\xd7\x9c\xd7\x95\x00 decode}", 16) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{\xd7\xa9\xd7\x9c\xd7\x95\x00\x9d decode}", 17) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{\x00\xa9\xd7\x9c\xd7\x95\xd7\x9d decode}", 17) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{\x00a decode}", 12) == B6B_ERR);
	b6b_interp_destroy(&interp);

	/* decoding of a single-letter Hebrew word should succeed */
	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{\xd7\x90 decode}", 11) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "\xd7\x90") == 0);
	assert(!b6b_list_next(b6b_list_first(interp.fg->_)));
	b6b_interp_destroy(&interp);

	/* decoding of a Hebrew word containing an English letter should succeed */
	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{\xd7\xa9\xd7\x9c\x61\xd7\x9d decode}", 16) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == 2);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "\xd7\xa9") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->slen == 2);
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "\xd7\x9c") == 0);
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_str(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o->slen == 1);
	assert(strcmp(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o->s, "a") == 0);
	assert(b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))));
	assert(b6b_as_str(b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))))->o->slen == 2);
	assert(strcmp(b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))))->o->s, "\xd7\x9d") == 0);
	assert(!b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))))));
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
