/*
 * This file is part of b6b.
 *
 * Copyright 2018 Dima Krasner
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
	assert(b6b_call_copy(&interp, "{$inet.client}", 14) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$inet.client tcp}", 18) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.client tcp 127.0.0.1}",
	                     28) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$local s [$inet.server tcp 127.0.0.1 2924]}",
	                     44) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                    "{$local c [$inet.client tcp 127.0.0.1 2924]}",
	                    44) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$c peer}", 9) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == sizeof("127.0.0.1") - 1);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_float(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f == 2924);
	assert(!b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_call_copy(&interp,
	                     "{$local a [$list.index [$s accept] 0 ]}",
	                     39) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$c write abcd}", 15) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(b6b_call_copy(&interp, "{$c write efgh}", 15) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(b6b_call_copy(&interp, "{$a read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 8);
	assert(strcmp(interp.fg->_->s, "abcdefgh") == 0);
	assert(b6b_call_copy(&interp, "{$a write ijkl}", 15) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(b6b_call_copy(&interp, "{$a write mnop}", 15) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(b6b_call_copy(&interp, "{$c read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 8);
	assert(strcmp(interp.fg->_->s, "ijklmnop") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$local s [$inet.server udp 127.0.0.1 2924]}",
	                     44) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                    "{$local c [$inet.client udp 127.0.0.1 2924]}",
	                    44) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$s peer}", 9) == B6B_ERR);
	assert(b6b_call_copy(&interp, "{$c write abcd}", 15) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(b6b_call_copy(&interp, "{$c write efgh}", 15) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(b6b_call_copy(&interp, "{$s read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	assert(b6b_call_copy(&interp, "{$s peer}", 9) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == sizeof("127.0.0.1") - 1);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_float(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f > 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f < UINT16_MAX);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp localhost 2924}",
	                     33) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                     "{$inet.client tcp localhost 2924}",
	                     33) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp localhost 2924}",
	                     33) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                     "{$inet.client tcp localhosf 2924}",
	                     33) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp 127.0.0.1 2924}",
	                     33) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                     "{$inet.client tcf 127.0.0.1 2924}",
	                     33) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.client tcp 255.255.255.255 2924}",
	                     39) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp 127.0.0.1 2924}",
	                     33) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                     "{$inet.client tcp {} 2924}",
	                     26) == B6B_ERR);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
