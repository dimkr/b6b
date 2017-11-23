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

struct b6b_fseeko_data {
	FILE *fp;
	off_t off;
	int whence;
	int ret;
	int rerrno;
};

static void b6b_stdio_do_fseeko(void *arg)
{
	struct b6b_fseeko_data *data = (struct b6b_fseeko_data *)arg;

	data->ret = fseeko(data->fp, data->off, data->whence);
	if (data->ret < 0)
		data->rerrno = errno;
}

ssize_t b6b_stdio_peeksz(struct b6b_interp *interp, void *priv)
{
	struct b6b_fseeko_data data = {
		.off = 0,
		.whence = SEEK_END
	};
	const struct b6b_stdio_strm *s = (const struct b6b_stdio_strm *)priv;
	off_t here, end;
	ssize_t len;

	here = ftello(s->fp);
	if ((here == (off_t)-1) || (here > SSIZE_MAX))
		return -1;

	data.fp = s->fp;
	if (!b6b_offload(interp, b6b_stdio_do_fseeko, &data))
		return -1;

	if (data.ret < 0) {
		b6b_return_strerror(interp, data.rerrno);
		return -1;
	}

	end = ftello(s->fp);
	if ((end == (off_t)-1) || (end > SSIZE_MAX))
		return -1;

	if (end > here) {
		data.off = here;
		data.whence = SEEK_SET;
		if (!b6b_offload(interp, b6b_stdio_do_fseeko, &data))
			return -1;

		if (data.ret < 0) {
			b6b_return_strerror(interp, data.rerrno);
			return -1;
		}
	}

	len = (ssize_t)(end - here);
	if (len > 0)
		return len;

	/* special, non-seekable files (e.g. proc and /dev/zero) behave like empty
	 * files; therefore, we don't know the real amount we can read and fall back
	 * to B6B_STRM_BUFSIZ */
	return B6B_STRM_BUFSIZ;
}

struct b6b_stdio_fread_data {
	void *buf;
	FILE *fp;
	size_t len;
	size_t ret;
	int rerrno;
};

static void b6b_stdio_do_fread(void *arg)
{
	struct b6b_stdio_fread_data *data = (struct b6b_stdio_fread_data *)arg;

	data->ret = fread(data->buf, 1, data->len, data->fp);
	if (data->ret == 0)
		data->rerrno = errno;
}

ssize_t b6b_stdio_read(struct b6b_interp *interp,
                       void *priv,
                       unsigned char *buf,
                       const size_t len,
                       int *eof,
                       int *again)
{
	struct b6b_stdio_fread_data data = {
		.buf = buf,
		.len = len,
		.rerrno = 0
	};
	const struct b6b_stdio_strm *s = (const struct b6b_stdio_strm *)priv;

	data.fp = s->fp;
	if (!b6b_offload(interp, b6b_stdio_do_fread, &data))
		return -1;

	if (data.ret < len) {
		if (ferror(s->fp)) {
			b6b_return_strerror(interp, data.rerrno);
			return -1;
		}
		if (feof(s->fp))
			*eof = 1;
	}

	return (ssize_t)data.ret;
}

struct b6b_stdio_fwrite_data {
	const void *buf;
	FILE *fp;
	size_t len;
	size_t ret;
	int rerrno;
};

static void b6b_stdio_do_fwrite(void *arg)
{
	struct b6b_stdio_fwrite_data *data = (struct b6b_stdio_fwrite_data *)arg;

	data->ret = fwrite(data->buf, 1, data->len, data->fp);
	if (data->ret == 0)
		data->rerrno = errno;
}

ssize_t b6b_stdio_write(struct b6b_interp *interp,
                        void *priv,
                        const unsigned char *buf,
                        const size_t len)
{
	struct b6b_stdio_fwrite_data data;
	const struct b6b_stdio_strm *s = (const struct b6b_stdio_strm *)priv;
	unsigned char *copy;
	size_t total = 0;

	/* we must copy the buffer, since it may be the string representation of an
	 * object freed during context switch */
	copy = (unsigned char *)malloc(len);
	if (b6b_unlikely(!copy))
		return -1;
	memcpy(copy, buf, len);

	data.fp = s->fp;
	do {
		data.buf = copy + total;
		data.len = len - total;
		if (!b6b_offload(interp, b6b_stdio_do_fwrite, &data)) {
			free(copy);
			return -1;
		}

		if (data.ret == 0) {
			if (ferror(s->fp)) {
				free(copy);
				b6b_return_strerror(interp, data.rerrno);
				return -1;
			}
			break;
		}
		total += data.ret;
	} while (total < len);

	free(copy);
	return total;
}

int b6b_stdio_fd(void *priv)
{
	const struct b6b_stdio_strm *s = (const struct b6b_stdio_strm *)priv;

	return s->fd;
}

void b6b_stdio_do_fclose(void *arg)
{
	fclose((FILE *)arg);
}

void b6b_stdio_close(void *priv)
{
	struct b6b_stdio_strm *s = (struct b6b_stdio_strm *)priv;

	/* fclose() may block due to buffered writing */
	b6b_offload(s->interp, b6b_stdio_do_fclose, s->fp);

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
	s->interp = interp;
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
