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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <b6b.h>

int main()
{
	struct b6b_interp interp;
	struct sockaddr_un sun = {.sun_family = AF_UNIX};
	int s, c;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	assert(s >= 0);
	strcpy(sun.sun_path, "addr");
	assert((unlink(sun.sun_path) == 0) || (errno == ENOENT));
	assert(bind(s, (const struct sockaddr *)&sun, sizeof(sun)) == 0);
	assert(listen(s, 5) == 0);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$local p [$poll]}", 18) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$local s [$un.client stream addr]}", 35) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$p add [$s fd] $POLLINOUT}", 27) == B6B_OK);

	c = accept(s, NULL, NULL);
	assert(c >= 0);

	assert(b6b_call_copy(&interp, "{$p wait 5 0}", 13) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_as_list(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_empty(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_list_empty(b6b_list_first(interp.fg->_)->o));
	assert(b6b_as_list(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(!b6b_list_empty(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_list(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(b6b_list_empty(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));

	assert(send(c, "abcd", 4, 0) == 4);
	assert(b6b_call_copy(&interp, "{$p wait 5 0}", 13) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_list(b6b_list_first(interp.fg->_)->o));
	assert(!b6b_list_empty(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_list(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(!b6b_list_empty(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_list(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(b6b_list_empty(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));

	assert(shutdown(c, SHUT_RD) == 0);
	assert(b6b_call_copy(&interp, "{$p wait 5 0}", 13) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_list(b6b_list_first(interp.fg->_)->o));
	assert(!b6b_list_empty(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_list(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(!b6b_list_empty(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_list(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(b6b_list_empty(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));

	assert(close(c) == 0);
	assert(b6b_call_copy(&interp, "{$p wait 5 0}", 13) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_list(b6b_list_first(interp.fg->_)->o));
	assert(!b6b_list_empty(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_list(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(!b6b_list_empty(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_list(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(!b6b_list_empty(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));

	assert(b6b_call_copy(&interp, "{$s read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "abcd") == 0);

	assert(b6b_call_copy(&interp, "{$s read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(!interp.fg->_->slen);

	assert(b6b_call_copy(&interp, "{$p wait 5 0}", 13) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_list(b6b_list_first(interp.fg->_)->o));
	assert(!b6b_list_empty(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_list(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(!b6b_list_empty(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_list(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(!b6b_list_empty(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));

	assert(b6b_call_copy(&interp, "{$p remove [$s fd]}", 19) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$p wait 5 0}", 13) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_list(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_empty(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_list(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_empty(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_))));
	assert(b6b_as_list(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));
	assert(b6b_list_empty(b6b_list_next(b6b_list_next(b6b_list_first(interp.fg->_)))->o));

	assert(b6b_call_copy(&interp, "{$p remove [$s fd]}", 19) == B6B_OK);
	b6b_interp_destroy(&interp);

	close(s);

	return EXIT_SUCCESS;
}
