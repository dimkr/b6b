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
#include <string.h>

#include <b6b.h>

int main()
{
	struct b6b_interp interp;

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$str.split}", 12) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$str.split {} ,}", 17) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == 0);
	assert(!b6b_list_next(b6b_list_first(interp.fg->_)));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$str.split abc ,}", 18) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "abc") == 0);
	assert(!b6b_list_next(b6b_list_first(interp.fg->_)));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$str.split abc abc}", 20) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(!b6b_list_first(interp.fg->_)->o->slen);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->slen == 0);
	assert(!b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$str.split ab,cd,ef ,}", 23) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "ab") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "cd") == 0);
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_str(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(strcmp(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o->s, "ef") == 0);
	assert(!b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$str.split ab,cd, ,}", 21) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "ab") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "cd") == 0);
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_str(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o->slen == 0);
	assert(!b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$str.split ab,cde ,c}", 22) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "ab") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "de") == 0);
	assert(!b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
