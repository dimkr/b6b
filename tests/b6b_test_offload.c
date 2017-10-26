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
	size_t i, bi = 0, ci = 0;
	unsigned int times[2] = {0, 0};

	/* output should be something like: cbacbacba */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$global x {}} {$spawn {{$loop {{$sleep 0.1} {$list.append $x a}}}}} {$spawn {{$loop {{$sleep 0.1} {$list.append $x b}}}}} {$spawn {{$loop {{$sleep 0.1} {$list.append $x c}}}}} {$loop {{$if [$>= [$list.len $x] 9] {{$return [$str.join {} $x]}}} {$yield}}}", 254) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen >= 9);
	for (i = 3; i < interp.fg->_->slen; i += 3)
		assert(strncmp(interp.fg->_->s, &interp.fg->_->s[i], 3) == 0);
	b6b_interp_destroy(&interp);

	/* output should be something like: babacbaba */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$global x {}} {$spawn {{$loop {{$sleep 0.1} {$list.append $x a}}}}} {$spawn {{$loop {{$sleep 0.1} {$list.append $x b}}}}} {$spawn {{$map x {1 2 3} {{$sleep 0.1}}} {$list.append $x c} {$return}}} {$loop {{$if [$>= [$list.len $x] 9] {{$return [$str.join {} $x]}}} {$yield}}}", 273) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen > 3);
	i = 2;
	do {
		if (interp.fg->_->s[i] == 'c') {
			++i;
			++times[0];
		}
		else {
			assert(interp.fg->_->s[i] == interp.fg->_->s[0]);
			++i;
			if (i == interp.fg->_->slen)
				break;

			assert(interp.fg->_->s[i] == interp.fg->_->s[1]);
			++i;
		}
	} while (i < interp.fg->_->slen);
	assert(times[0] == 1);
	b6b_interp_destroy(&interp);

	/* output should be something like: aaaacbaaa */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$global x {}} {$spawn {{$loop {{$sleep 0.1} {$list.append $x a}}}}} {$spawn {{$map x {1 2 3 4 5} {{$sleep 0.1}}} {$list.append $x b} {$return}}} {$spawn {{$map x {1 2 3 4 5} {{$sleep 0.1}}} {$list.append $x c} {$return}}} {$loop {{$if [$>= [$list.len $x] 9] {{$return [$str.join {} $x]}}} {$yield}}}", 300) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen >= 9);
	times[0] = 0;
	for (i = 0; i < interp.fg->_->slen; ++i) {
		if (interp.fg->_->s[i] == 'b') {
			++times[0];
			bi = i;
		}
		else if (interp.fg->_->s[i] == 'c') {
			++times[1];
			ci = i;
		}
		else
			assert(interp.fg->_->s[i] == 'a');
	}
	assert(times[0] == 1);
	assert(times[1] == 1);
	assert((bi == ci + 1) || (ci == bi + 1));
	b6b_interp_destroy(&interp);

	/* output should be something like: aabaacaaa */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$global x {}} {$spawn {{$loop {{$sleep 0.1} {$list.append $x a}}}}} {$spawn {{$map x {1 2 3} {{$sleep 0.1}}} {$list.append $x b} {$return}}} {$spawn {{$map x {1 2 3 4 5} {{$sleep 0.1}}} {$list.append $x c} {$return}}} {$loop {{$if [$>= [$list.len $x] 9] {{$return [$str.join {} $x]}}} {$yield}}}", 296) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen >= 9);
	times[0] = times[1] = 0;
	for (i = 0; i < interp.fg->_->slen; ++i) {
		if (interp.fg->_->s[i] == 'b') {
			++times[0];
			bi = i;
		}
		else if (interp.fg->_->s[i] == 'c') {
			++times[1];
			ci = i;
		}
		else
			assert(interp.fg->_->s[i] == 'a');
	}
	assert(times[0] == 1);
	assert(times[1] == 1);
	assert((bi != ci + 1) && (ci != bi + 1));
	b6b_interp_destroy(&interp);

	/* output should be something like: acbd */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$global x {}} {$spawn {{$loop {{$if [$list.len $x] {{$list.append $x b} {$return}}}}}}} {$spawn {{$loop {{$if [$list.len $x] {{$list.append $x c} {$return}}}}}}} {$list.append $x a} {$sleep 1} {$list.append $x d} {$return [$str.join {} $x]}", 241) == B6B_RET);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert((strcmp(interp.fg->_->s, "abcd") == 0) ||
	       (strcmp(interp.fg->_->s, "acbd") == 0));
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
