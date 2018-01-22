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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <b6b.h>

static void setup(const char *buf, const ssize_t len)
{
	int fd;

	fd = open("/tmp/.file", O_WRONLY | O_CREAT | O_TRUNC, 0600);
	assert(fd >= 0);
	assert(write(fd, buf, (size_t)len) == len);
	close(fd);
}

static void teardown(const char *buf, const ssize_t len)
{
	static char tmp[128];
	int fd;

	fd = open("/tmp/.file", O_RDONLY);
	assert(fd >= 0);
	assert(read(fd, tmp, sizeof(tmp)) == len);
	close(fd);
	tmp[len] = '\0';
	assert(strcmp(tmp, buf) == 0);
}

int main()
{
	struct b6b_interp interp;
	struct stat stbuf;
	size_t i;

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$open}", 7) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$open /dev/zero r}",
	                     19) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$open /dev/zero x}",
	                     19) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert((unlink("/tmp/.file") == 0) || (errno == ENOENT));
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /tmp/.file r] read}",
	                     27) == B6B_ERR);
	b6b_interp_destroy(&interp);

	setup("", 0);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{[$open /tmp/.file r] read}", 27) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(!interp.fg->_->slen);
	b6b_interp_destroy(&interp);
	teardown("", 0);

	setup("abcdefgh", 8);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /tmp/.file r] read -1}",
	                     30) == B6B_ERR);
	b6b_interp_destroy(&interp);
	teardown("abcdefgh", 8);

	setup("abcdefgh", 8);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /tmp/.file r] read 3}",
	                     29) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "abc") == 0);
	b6b_interp_destroy(&interp);
	teardown("abcdefgh", 8);

	setup("abcdefgh", 8);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /tmp/.file r] read 30}",
	                     30) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "abcdefgh") == 0);
	b6b_interp_destroy(&interp);
	teardown("abcdefgh", 8);

	setup("abcdefgh", 8);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{[$open /tmp/.file r] read}", 27) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "abcdefgh") == 0);
	b6b_interp_destroy(&interp);
	teardown("abcdefgh", 8);

	setup("abcdefgh", 8);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{[$open /tmp/.file] read}", 25) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "abcdefgh") == 0);
	b6b_interp_destroy(&interp);
	teardown("abcdefgh", 8);

	setup("abcdefgh", 8);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /tmp/.file] write a}",
	                     28) == B6B_ERR);
	b6b_interp_destroy(&interp);
	teardown("abcdefgh", 8);

	setup("abcd", 4);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /tmp/.file w] write efgh}",
	                     33) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	b6b_interp_destroy(&interp);
	teardown("efgh", 4);

	setup("abcd", 4);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /tmp/.file r+] write efgh}",
	                     34) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	b6b_interp_destroy(&interp);
	teardown("efgh", 4);

	setup("abcd", 4);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /tmp/.file a] write efgh}",
	                     33) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	b6b_interp_destroy(&interp);
	teardown("abcdefgh", 8);

	setup("abcd", 4);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /tmp/.file a] read}",
	                     27) == B6B_ERR);
	b6b_interp_destroy(&interp);
	teardown("abcd", 4);

	setup("abcd", 4);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /tmp/.file w] read}",
	                     27) == B6B_ERR);
	b6b_interp_destroy(&interp);
	teardown("", 0);

	assert(unlink("/tmp/.file") == 0);

	/* regression test: /dev/zero is unseekable and b6b_strm_read() always
	 * returned an empty string because b6b_stdio_peeksz() said there's nothing
	 * to read */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$open /dev/zero r] read 2}",
	                     28) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen);
	for (i = 0; i < interp.fg->_->slen; ++i)
		assert(interp.fg->_->s[i] == 0);
	b6b_interp_destroy(&interp);

#ifdef B6B_HAVE_OFFLOAD_THREAD
	/* regression test: use-after-free in b6b_threaded() during cleanup, when
	 * all threads have been freed while the global frame still holds a
	 * reference to a file and its fclose() is offloaded */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global a [$open /dev/null]} {$spawn {{$exit}}} {$loop {{$yield}}}",
	                     67) == B6B_EXIT);
	b6b_interp_destroy(&interp);
#endif

	/* written data to line-buffered files should be flushed upon \n */
	setup("", 0);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global f [$open /tmp/.file w]}",
	                     32) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$f write abc}", 14) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 3);
	assert(stat("/tmp/.file", &stbuf) == 0);
	assert(stbuf.st_size == 0);
	assert(b6b_call_copy(&interp, "{$f write {d\ne}}", 16) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 3);
	teardown("abcd\n", 5);
	b6b_interp_destroy(&interp);

	/* data written to block-buffered files should be flushed after each chunk
	 * of B6B_STRM_BUFSIZ bytes */
	setup("", 0);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global f [$open /tmp/.file wb]}",
	                     33) == B6B_OK);
	for (i = 0; i < B6B_STRM_BUFSIZ; i += 4096) {
		assert(b6b_call_copy(&interp,
		                     "{$f write [[$open /dev/zero ru] read 4096]}",
		                     43) == B6B_OK);
		assert(b6b_as_float(interp.fg->_));
		assert(interp.fg->_->f == 4096);
		assert(stat("/tmp/.file", &stbuf) == 0);
		assert(stbuf.st_size == 0);
	}
	assert(b6b_call_copy(&interp,
	                     "{$f write a}",
	                     12) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 1);
	assert(stat("/tmp/.file", &stbuf) == 0);
	assert(stbuf.st_size == B6B_STRM_BUFSIZ);
	b6b_interp_destroy(&interp);

	/* written data to unbuffered files should be written immediately */
	setup("", 0);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global f [$open /tmp/.file wu]}",
	                     33) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$f write abc}", 14) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 3);
	teardown("abc", 3);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
