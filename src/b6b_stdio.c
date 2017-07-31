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

#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include <b6b.h>

ssize_t b6b_stdio_peeksz(struct b6b_interp *interp, void *priv)
{
	const struct b6b_stdio_strm *s = (const struct b6b_stdio_strm *)priv;
	off_t here, end;

	here = ftello(s->fp);
	if ((here == (off_t)-1) || (here > SSIZE_MAX))
		return -1;

	if (fseek(s->fp, 0, SEEK_END) < 0) {
		b6b_return_strerror(interp, errno);
		return -1;
	}

	end = ftello(s->fp);
	if ((end == (off_t)-1) || (end > SSIZE_MAX))
		return -1;

	if ((end > here) && (fseeko(s->fp, here, SEEK_SET) < 0)) {
		b6b_return_strerror(interp, errno);
		return -1;
	}

	return (ssize_t)(end - here);
}

ssize_t b6b_stdio_read(struct b6b_interp *interp,
                       void *priv,
                       unsigned char *buf,
                       const size_t len,
                       int *eof,
                       int *again)
{
	const struct b6b_stdio_strm *s = (const struct b6b_stdio_strm *)priv;
	size_t ret;

	ret = fread(buf, 1, len, s->fp);
	if (ret < len) {
		if (ferror(s->fp)) {
			b6b_return_strerror(interp, errno);
			return -1;
		}
		if (feof(s->fp))
			*eof = 1;
	}

	return (ssize_t)ret;
}

ssize_t b6b_stdio_write(struct b6b_interp *interp,
                        void *priv,
                        const unsigned char *buf,
                        const size_t len)
{
	const struct b6b_stdio_strm *s = (const struct b6b_stdio_strm *)priv;
	size_t chunk, total = 0;

	do {
		chunk = fwrite(buf + total, 1, len - total, s->fp);
		if (chunk == 0) {
			if (ferror(s->fp)) {
				b6b_return_strerror(interp, errno);
				return -1;
			}
			break;
		}
		total += chunk;
	} while (total < len);

	return total;
}

int b6b_stdio_fd(void *priv)
{
	const struct b6b_stdio_strm *s = (const struct b6b_stdio_strm *)priv;

	return s->fd;
}

void b6b_stdio_close(void *priv)
{
	struct b6b_stdio_strm *s = (struct b6b_stdio_strm *)priv;

	fclose(s->fp);

	if (s->buf)
		free(s->buf);

	free(s);
}

static const struct b6b_strm_ops b6b_stdio_ro_pipe_ops = {
	.read = b6b_stdio_read,
	.fd = b6b_stdio_fd,
	.close = free
};

static const struct b6b_strm_ops b6b_stdio_wo_pipe_ops = {
	.write = b6b_stdio_write,
	.fd = b6b_stdio_fd,
	.close = free
};

static int b6b_stdio_wrap(struct b6b_interp *interp,
                          const char *name,
                          const size_t len,
                          FILE *fp,
                          const int fd,
                          const struct b6b_strm_ops *ops)
{
	struct b6b_obj *o;
	struct b6b_stdio_strm *s;

	s = (struct b6b_stdio_strm *)malloc(sizeof(*s));
	if (b6b_unlikely(!s))
		return 0;

	s->fp = fp;
	s->buf = NULL;
	s->fd = fd;

	o = b6b_strm_copy(interp, ops, s, name, len);
	if (b6b_unlikely(!o)) {
		free(s);
		return 0;
	}

	if (b6b_unlikely(!b6b_global(interp, o, o))) {
		b6b_destroy(o);
		return 0;
	}

	if (interp->opts & B6B_OPT_NBF)
		setbuf(fp, NULL);

	b6b_unref(o);
	return 1;
}

#define B6B_STDIO_WRAP(interp, fp, fd, ops) \
	b6b_stdio_wrap(interp, #fp, sizeof(#fp) - 1, fp, fd, ops)

static int b6b_stdio_init(struct b6b_interp *interp)
{
	return B6B_STDIO_WRAP(interp,
	                      stdin,
	                      STDIN_FILENO,
	                      &b6b_stdio_ro_pipe_ops) &&
	       B6B_STDIO_WRAP(interp,
	                      stdout,
	                      STDOUT_FILENO,
	                      &b6b_stdio_wo_pipe_ops) &&
	       B6B_STDIO_WRAP(interp,
	                      stderr,
	                      STDERR_FILENO,
	                      &b6b_stdio_wo_pipe_ops);
}
__b6b_init(b6b_stdio_init);
