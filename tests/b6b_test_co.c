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
	                "{$co {{$map y [$range 1 20] {{$list.append $x a}}}}} " \
	                "{$map y [$range 1 20] {{$list.append $x b}}} " \
	                "{$return $x} " ,
	                15 + 53 + 45 + 13) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(strstr(interp.fg->_->s, "a b"));
	assert(strstr(interp.fg->_->s, "b a"));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(
	                &interp,
	                "{$global x {}} " \
	                "{$map y [$range 1 20] {" \
	                	"{$co {{$map y [$range 1 5] {{$list.append $x a}}}}} " \
	                	"{$co {{$map y [$range 1 5] {{$list.append $x b}}}}} " \
	                	"{$co {{$map y [$range 1 5] {{$list.append $x c}}}}} "
	                "}} " \
	                "{$map y [$range 1 300] {{$yield}}} " \
	                "{$return $x}" ,
	                15 + 23 + 52 + 52 + 52 + 3 + 35 + 12) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(strstr(interp.fg->_->s, "a b"));
	assert(strstr(interp.fg->_->s, "b c"));
	assert(strstr(interp.fg->_->s, "c a"));
	assert(!strstr(interp.fg->_->s, "a c"));
	assert(!strstr(interp.fg->_->s, "b a"));
	assert(!strstr(interp.fg->_->s, "c b"));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(
	                &interp,
	                "{$global x {}} " \
	                "{$map y [$range 1 20] {" \
	                	"{$co {{$map y [$range 1 5] {{$list.append $x a}}}}} " \
	                	"{$co {{$map y [$range 1 5] {{$list.append $x b}}}}} " \
	                	"{$co {{$map y [$range 1 5] {{$list.append $x c}}}}} " \
	                "}} " \
	                "{$map y [$range 1 300] {{$list.append $x d}}} " \
	                "{$return $x}" ,
	                15 + 23 + 52 + 52 + 52 + 3 + 46 + 12) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(strstr(interp.fg->_->s, "a b"));
	assert(strstr(interp.fg->_->s, "a d"));
	assert(strstr(interp.fg->_->s, "b c"));
	assert(strstr(interp.fg->_->s, "b d"));
	assert(strstr(interp.fg->_->s, "c d"));
	assert(!strstr(interp.fg->_->s, "b a"));
	assert(!strstr(interp.fg->_->s, "c b"));
	b6b_interp_destroy(&interp);
	return EXIT_SUCCESS;
}
