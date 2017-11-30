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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <assert.h>

#include <b6b.h>

int main()
{
	struct b6b_interp interp;

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map}", 6) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$map {} {1 2} {{$+ $x 1.5}}}",
	                     29) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map x 1 {{$+ $x 1.5}}}", 24) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_first(interp.fg->_));
	assert(b6b_as_float(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->f == 2.5);
	assert(!b6b_list_next(b6b_list_first(interp.fg->_)));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$map x {1 2 3} {{$+ $x 1.5}}}",
	                     30) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_first(interp.fg->_));
	assert(b6b_as_float(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->f == 2.5);
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o);
	assert(b6b_as_float(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f == 3.5);
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o);
	assert(b6b_as_float(
	            b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(
	   b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o->f == 4.5);
	assert(
	!b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(
	            &interp,
	            "{$map x {1 2 3} {{$if [$== $x 2] {{$continue}}} {$+ $x 1.5}}}",
	            61) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_first(interp.fg->_));
	assert(b6b_as_float(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->f == 2.5);
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o);
	assert(b6b_as_float(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f == 4.5);
	assert(!b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(
	            &interp,
	            "{$map x {1 2 3} {{$if [$== $x 2] {{$break}}} {$+ $x 1.5}}}",
	            58) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_first(interp.fg->_));
	assert(b6b_as_float(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->f == 2.5);
	assert(!b6b_list_next(b6b_list_first(interp.fg->_)));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map x {} {{$+ $x 1.5}}}", 25) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_empty(interp.fg->_));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$map {x y} {1 2} {{$+ $x $y}}}",
	                     31) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_first(interp.fg->_));
	assert(b6b_as_float(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->f == 3);
	assert(!b6b_list_next(b6b_list_first(interp.fg->_)));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$map {x y} {1 2 3 4} {{$+ $x $y}}}",
	                     35) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_first(interp.fg->_));
	assert(b6b_as_float(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->f == 3);
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o);
	assert(b6b_as_float(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f == 7);
	assert(!b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$map {x y} {1 2 3} {{$+ $x $y}}}",
	                     33) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$map {x y z} {1 2} {{$+ $x $y}}}",
	                     33) == B6B_ERR);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
