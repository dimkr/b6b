/*
 * This file is part of b6b.
 *
 * Copyright 2017, 2018 Dima Krasner
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

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0x0));
	assert(b6b_call_copy(&interp, "{$sem}", 6) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0x0));
	assert(b6b_call_copy(&interp, "{$global s [$sem 0]} {$global x {}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x a]]} {$yield}}}}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x b]]} {$yield}}}}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x c]]} {$yield}}}}} {$map i {1 2 3} {{$map j [$range 1 1000] {{$nop}}} {$s write 1} {$yield}}} {$map j [$range 1 1000] {{$nop}}} {$return $x}", 490) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen);
	switch (interp.fg->_->s[0]) {
		case 'a':
			assert(interp.fg->_->s[1] == 'a');
			assert(strstr(interp.fg->_->s, "b") >= interp.fg->_->s + strspn(interp.fg->_->s, "a"));
			assert(strstr(interp.fg->_->s, "c") >= interp.fg->_->s + strspn(interp.fg->_->s, "a"));
			break;

		case 'b':
			assert(interp.fg->_->s[1] == 'b');
			assert(strstr(interp.fg->_->s, "a") >= interp.fg->_->s + strspn(interp.fg->_->s, "b"));
			assert(strstr(interp.fg->_->s, "c") >= interp.fg->_->s + strspn(interp.fg->_->s, "b"));
			break;

		case 'c':
			assert(interp.fg->_->s[1] == 'c');
			assert(strstr(interp.fg->_->s, "b") >= interp.fg->_->s + strspn(interp.fg->_->s, "c"));
			assert(strstr(interp.fg->_->s, "a") >= interp.fg->_->s + strspn(interp.fg->_->s, "c"));
			break;
	}
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0x0));
	assert(b6b_call_copy(&interp, "{$global s [$sem 0]} {$global x {}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x a]]} {$yield}}}}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x b]]} {$yield}}}}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x c]]} {$yield}}}}} {$map i {1 2 3 4} {{$map j [$range 1 1000] {{$nop}}} {$s write 1} {$yield}}} {$map j [$range 1 1000] {{$nop}}} {$return $x}", 492) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen);
	switch (interp.fg->_->s[0]) {
		case 'a':
			assert(interp.fg->_->s[1] == 'a');
			assert(strstr(interp.fg->_->s, "b") >= interp.fg->_->s + strspn(interp.fg->_->s, "a"));
			assert(strstr(interp.fg->_->s, "c") >= interp.fg->_->s + strspn(interp.fg->_->s, "a"));
			break;

		case 'b':
			assert(interp.fg->_->s[1] == 'b');
			assert(strstr(interp.fg->_->s, "a") >= interp.fg->_->s + strspn(interp.fg->_->s, "b"));
			assert(strstr(interp.fg->_->s, "c") >= interp.fg->_->s + strspn(interp.fg->_->s, "b"));
			break;

		case 'c':
			assert(interp.fg->_->s[1] == 'c');
			assert(strstr(interp.fg->_->s, "b") >= interp.fg->_->s + strspn(interp.fg->_->s, "c"));
			assert(strstr(interp.fg->_->s, "a") >= interp.fg->_->s + strspn(interp.fg->_->s, "c"));
			break;
	}
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0x0));
	assert(b6b_call_copy(&interp, "{$global s [$sem 0]} {$global x {}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x a]]} {$yield}}}}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x b]]} {$yield}}}}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x c]]} {$yield}}}}} {$map i {1 2} {{$map j [$range 1 1000] {{$nop}}} {$s write 1} {$yield}}} {$map j [$range 1 1000] {{$nop}}} {$return $x}", 488) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen);
	switch (interp.fg->_->s[0]) {
		case 'a':
			assert(interp.fg->_->s[1] == 'a');
			assert((strstr(interp.fg->_->s, "b") && !strstr(interp.fg->_->s, "c")) || (strstr(interp.fg->_->s, "c") && !strstr(interp.fg->_->s, "b")));
			break;

		case 'b':
			assert(interp.fg->_->s[1] == 'b');
			assert((strstr(interp.fg->_->s, "a") && !strstr(interp.fg->_->s, "c")) || (strstr(interp.fg->_->s, "c") && !strstr(interp.fg->_->s, "a")));
			break;

		case 'c':
			assert(interp.fg->_->s[1] == 'c');
			assert((strstr(interp.fg->_->s, "a") && !strstr(interp.fg->_->s, "b")) || (strstr(interp.fg->_->s, "b") && !strstr(interp.fg->_->s, "a")));
			break;
	}
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0x0));
	assert(b6b_call_copy(&interp, "{$global s [$sem 0]} {$global x {}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x a]]} {$yield}}}}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x b]]} {$yield}}}}} {$spawn {{$loop {{$if [$s read] {{$break}}}}} {$loop {{$global x [$str.join {} [$list.new $x c]]} {$yield}}}}} {$map i {1 2 3} {{$map j [$range 1 1000] {{$nop}}} {$yield}}} {$map j [$range 1 1000] {{$nop}}} {$return $x}", 477) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(!interp.fg->_->slen);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
