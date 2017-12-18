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
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <b6b.h>

static const struct b6b_strm_ops b6b_memstream_ops = {
	.peeksz = b6b_stdio_peeksz,
	.read = b6b_stdio_read,
	.write = b6b_stdio_write,
	.fd = b6b_stdio_fd,
	.close = b6b_stdio_close
};

static ssize_t b6b_stdio_slow_write(struct b6b_interp *interp,
                                    void *priv,
                                    const unsigned char *buf,
                                    const size_t len)
{
	return b6b_stdio_write(interp, priv, buf, 1);
}

static const struct b6b_strm_ops b6b_slow_w_memstream_ops = {
	.peeksz = b6b_stdio_peeksz,
	.read = b6b_stdio_read,
	.write = b6b_stdio_slow_write,
	.fd = b6b_stdio_fd,
	.close = b6b_stdio_close
};

static ssize_t b6b_stdio_slow_read(struct b6b_interp *interp,
                                   void *priv,
                                   unsigned char *buf,
                                   const size_t len,
                                   int *eof,
                                   int *again)
{
	return b6b_stdio_read(interp, priv, buf, 1, eof, again);
}

static const struct b6b_strm_ops b6b_slow_r_memstream_ops = {
	.peeksz = b6b_stdio_peeksz,
	.read = b6b_stdio_slow_read,
	.write = b6b_stdio_write,
	.fd = b6b_stdio_fd,
	.close = b6b_stdio_close
};

static const struct b6b_strm_ops b6b_no_peek_memstream_ops = {
	.read = b6b_stdio_slow_read,
	.write = b6b_stdio_write,
	.fd = b6b_stdio_fd,
	.close = b6b_stdio_close
};

static void setup(struct b6b_interp *interp,
                  const struct b6b_strm_ops *ops,
                  struct b6b_obj **o,
                  FILE **fp,
                  char *buf,
                  size_t len)
{
	struct b6b_stdio_strm *s;
	struct b6b_obj *k;
	size_t i;

	s = (struct b6b_stdio_strm *)malloc(sizeof(*s));
	assert(s);

	for (i = 0; i < len; ++i)
		buf[i] = ('a' + i) % CHAR_MAX;

	*fp = fmemopen(buf, len, "ab+");
	assert(*fp);
	rewind(*fp);

	s->fp = *fp;
	s->interp = interp;
	s->buf = NULL;
	s->fd = fileno(*fp);

	assert(b6b_interp_new_argv(interp, 0, NULL, B6B_OPT_TRACE));

	*o = b6b_strm_copy(interp, ops, s, "abc", 3);
	assert(*o);

	k = b6b_str_copy("f", 1);
	assert(k);

	assert(b6b_local(interp, k, *o));
	b6b_unref(k);
	b6b_unref(*o);
}

int main()
{
	struct b6b_interp interp;
	struct b6b_obj *o;
	FILE *fp;
	char buf[8];
	int refc;

	/* writing nothing should succeed */
	setup(&interp, &b6b_memstream_ops, &o, &fp, buf, sizeof(buf));
	assert(b6b_call_copy(&interp, "{$f write {}}", 13) == B6B_OK);
	b6b_interp_destroy(&interp);
	assert(buf[0] == 'a');

	/* writing should succeed */
	setup(&interp, &b6b_memstream_ops, &o, &fp, buf, sizeof(buf));
	assert(b6b_call_copy(&interp, "{$f write hgfedcba}", 19) == B6B_OK);
	b6b_interp_destroy(&interp);
	assert(memcmp(buf, "hgfedcba", 8) == 0);

	/* writing should be retried in the case of partial write */
	setup(&interp, &b6b_slow_w_memstream_ops, &o, &fp, buf, sizeof(buf));
	assert(b6b_call_copy(&interp, "{$f write hgfedcba}", 19) == B6B_OK);
	b6b_interp_destroy(&interp);
	assert(memcmp(buf, "hgfedcba", 8) == 0);

	/* reading should succeed */
	setup(&interp, &b6b_memstream_ops, &o, &fp, buf, sizeof(buf));
	assert(b6b_call_copy(&interp, "{$f read 4}", 11) == B6B_OK);
	b6b_as_str(interp.fg->_);
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	b6b_interp_destroy(&interp);

	/* reading should advance the file pointer */
	setup(&interp, &b6b_memstream_ops, &o, &fp, buf, sizeof(buf));
	assert(b6b_call_copy(&interp, "{$f read 4}", 11) == B6B_OK);
	b6b_as_str(interp.fg->_);
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	assert(b6b_call_copy(&interp, "{$f read 4}", 11) == B6B_OK);
	b6b_as_str(interp.fg->_);
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "efgh") == 0);
	b6b_interp_destroy(&interp);

	/* reading after EOF should return nothing */
	setup(&interp, &b6b_memstream_ops, &o, 	&fp, buf, sizeof(buf));
	assert(b6b_call_copy(&interp, "{$f read 4}", 11) == B6B_OK);
	b6b_as_str(interp.fg->_);
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	assert(b6b_call_copy(&interp, "{$f read 4}", 11) == B6B_OK);
	b6b_as_str(interp.fg->_);
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "efgh") == 0);
	assert(b6b_call_copy(&interp, "{$f read 4}", 11) == B6B_OK);
	b6b_as_str(interp.fg->_);
	assert(interp.fg->_->slen == 0);
	b6b_interp_destroy(&interp);

	/* reading should be retried in the case of partial read */
	setup(&interp, &b6b_slow_r_memstream_ops, &o, &fp, buf, sizeof(buf));
	assert(b6b_call_copy(&interp, "{$f read 4}", 11) == B6B_OK);
	b6b_as_str(interp.fg->_);
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	b6b_interp_destroy(&interp);

	/* reading should succeed if the amount of available data is unknown */
	setup(&interp, &b6b_no_peek_memstream_ops, &o, &fp, buf, sizeof(buf));
	assert(b6b_call_copy(&interp, "{$f read}", 9) == B6B_OK);
	b6b_as_str(interp.fg->_);
	assert(interp.fg->_->slen == 8);
	assert(strcmp(interp.fg->_->s, "abcdefgh") == 0);
	b6b_interp_destroy(&interp);

	/* a file descriptor should hold a reference to keep the stream open */
	setup(&interp, &b6b_memstream_ops, &o, &fp, buf, sizeof(buf));
	refc = o->refc;
	assert(refc > 0);
	assert(b6b_call_copy(&interp, "{$f fd}", 7) == B6B_OK);
	assert(o->refc == refc + 1);
	b6b_interp_destroy(&interp);

	/* additional file descriptors should do so as well */
	setup(&interp, &b6b_memstream_ops, &o, &fp, buf, sizeof(buf));
	refc = o->refc;
	assert(refc > 0);
	assert(b6b_call_copy(&interp, "{$list.new [$f fd] [$f fd]}", 27) == B6B_OK);
	assert(o->refc == refc + 2);
	b6b_interp_destroy(&interp);

	/* once the file descriptor is garbage collected, the extra reference should
	 * be dropped */
	setup(&interp, &b6b_memstream_ops, &o, &fp, buf, sizeof(buf));
	refc = o->refc;
	assert(refc > 0);
	assert(b6b_call_copy(&interp, "{$f fd} {$nop}", 14) == B6B_OK);
	assert(o->refc == refc);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
