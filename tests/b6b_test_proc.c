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
	assert(b6b_call_copy(&interp, "{$proc a {}} {$a}", 17) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$proc a {{$+ 5 1} {$+ 6 1}}} {$a}",
	                     34) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 7);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$proc a {{$+ 5 1} {$return 7}}} {$a}",
	                     37) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 7);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$proc a {{$return 6} {$return 7}}} {$a}",
	                     40) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 6);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$proc a {{$exit 7} {$return 6}}} {$a}",
	                     38) == B6B_EXIT);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 7);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$proc a {{$throw 7} {$return 6}}} {$a}",
	                     39) == B6B_ERR);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 7);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$proc a {{$return $0}}} {$a abc def}",
	                     37) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "a") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$proc a {{$return $1}}} {$a abc def}",
	                     37) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "abc") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$proc a {{$return $2}}} {$a abc def}",
	                     37) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "def") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$proc a {{$return $@}}} {$a abc def}",
	                     37) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_first(interp.fg->_));
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "abc") == 0);
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_str(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(strcmp(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o->s, "def") == 0);
	assert(!b6b_list_next(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$proc a {{$list.append $. x}} {}} {$a} {$a}",
	                     44) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_first(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "x") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_str(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(strcmp(b6b_list_next(b6b_list_first(interp.fg->_))->o->s, "x") == 0);
	assert(!b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
