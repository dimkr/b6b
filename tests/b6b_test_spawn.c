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
	assert(b6b_call_copy(
	                &interp,
	                "{$global x {}} " \
	                "{$spawn {{$map y [$range 1 20] {{$list.append $x a}}}}} " \
	                "{$map y [$range 1 20] {{$list.append $x b}}} " \
	                "{$return $x} " ,
	                15 + 56 + 45 + 13) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(strstr(interp.fg->_->s, "a b"));
	assert(strstr(interp.fg->_->s, "b a"));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                "{$global x {}} " \
	                "{$spawn {{$map y [$range 1 20] {{$list.append $x a}}}}} " \
	                "{$spawn {{$map y [$range 1 20] {{$list.append $x b}}}}} " \
	                "{$map y [$range 1 20] {{$list.append $x c}}} " \
	                "{$return $x} " ,
	                15 + 56 + 56 + 45 + 13) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(strstr(interp.fg->_->s, "a b") ||
	       strstr(interp.fg->_->s, "a c"));
	assert(strstr(interp.fg->_->s, "b a") ||
	       strstr(interp.fg->_->s, "b c"));
	assert(strstr(interp.fg->_->s, "c a") ||
	       strstr(interp.fg->_->s, "c b"));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	        "{$global x {}} " \
	        "{$spawn {{$map y [$range 1 3] {{$list.append $x a} {$yield}}}}} " \
	        "{$map y [$range 1 3] {{$list.append $x b} {$yield}}} " \
	        "{$return $x} " ,
	        15 + 64 + 53 + 13) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "b a b a b a") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global x {}} " \
	                     "{$spawn {{$yield} {$exit 7}}} " \
	                     "{$while 1 {{$yield}}}",
	                     15 + 30 + 21) == B6B_EXIT);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
