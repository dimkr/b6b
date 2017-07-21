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

#include <b6b.h>

int main()
{
	struct b6b_interp interp;
	struct b6b_obj *s;

	s = b6b_str_copy("x", 1);
	assert(s);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$try {{$return 5}}}", 20) == B6B_RET);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 5);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$try {{$return 5}} {{$global x 1}}}",
	                     36) == B6B_RET);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 5);
	assert(!b6b_get(&interp, s));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$try {{$return 5}} {{$return 6}} {{$global x 0}}}",
	                     50) == B6B_RET);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 5);
	assert(b6b_get(&interp, s));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$try {{$throw 5}}}", 19) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$try {{$throw 5}} {{$return 6}}}",
	                     33) == B6B_RET);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 6);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$try {{$throw 5}} {{$throw 6}}}",
	                     32) == B6B_ERR);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 6);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$try {{$throw 5}} {{$throw 6}} {{$global x 0}}}",
	                     48) == B6B_ERR);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 6);
	assert(b6b_get(&interp, s));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$try {{$throw 5}} {{$return 6}} {{$global x 0}}}",
	                     49) == B6B_RET);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 6);
	assert(b6b_get(&interp, s));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$try {{$throw 5}} {} {{$global x 0}}}",
	                     38) == B6B_OK);
	assert(b6b_get(&interp, s));
	b6b_interp_destroy(&interp);

	b6b_unref(s);

	return EXIT_SUCCESS;
}
