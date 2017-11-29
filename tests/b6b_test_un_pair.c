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

	/* bi-directional reading and writing should succeed */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {a b} [$un.pair dgram] {{$a write abcd} {$b write efgh} {$return [$list.new [$a read] [$b read]]}}}", 105) == B6B_RET);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == 4);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "efgh") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->slen == 4);
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "abcd") == 0);
	b6b_interp_destroy(&interp);

	/* partial bi-directional reading and writing should succeed */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {a b} [$un.pair dgram] {{$a write abcd} {$b write efgh} {$return [$list.new [$a read 2] [$b read 3]]}}}", 109) == B6B_RET);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == 2);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "ef") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->slen == 3);
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "abc") == 0);
	b6b_interp_destroy(&interp);

	/* multiple reads should succeed */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {a b} [$un.pair dgram] {{$a write abcd} {$a write efgh} {$return [$list.new [$b read] [$b read]]}}}", 105) == B6B_RET);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == 4);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "abcd") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->slen == 4);
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "efgh") == 0);
	b6b_interp_destroy(&interp);

	/* writing to a closed stream should fail */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {a b} [$un.pair dgram] {{$b close} {$a write efgh}}}", 58) == B6B_ERR);
	b6b_interp_destroy(&interp);

	/* two buffers written to a stream socket should be received in one piece */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {a b} [$un.pair stream] {{$a write abcd} {$a write efgh} {$return [$b read]}}}", 84) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 8);
	assert(strcmp(interp.fg->_->s, "abcdefgh") == 0);
	b6b_interp_destroy(&interp);

	/* reading from a stream socket should succeed after the peer shuts down the connection */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {a b} [$un.pair stream] {{$a write abcd} {$a close} {$return [$b read]}}}", 79) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	b6b_interp_destroy(&interp);

	/* after a partial read, remaining data sent to a stream socket should be still queued for reading */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {a b} [$un.pair stream] {{$a write abcd} {$return [$list.new [$b read 2] [$b read]]}}}", 92) == B6B_RET);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == 2);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "ab") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->slen == 2);
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "cd") == 0);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
