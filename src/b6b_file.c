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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include <b6b.h>

#define B6B_FILE_DEF_FMODE "r"

struct b6b_file {
	FILE *fp;
	void *buf;
	int fd;
};

static ssize_t b6b_file_peeksz(struct b6b_interp *interp, void *priv)
{
	const struct b6b_file *f = (const struct b6b_file *)priv;
	off_t here, end;

	here = ftello(f->fp);
	if ((here == (off_t)-1) || (here > SSIZE_MAX))
		return -1;

	if (fseek(f->fp, 0, SEEK_END) < 0) {
		b6b_return_strerror(interp, errno);
		return -1;
	}

	end = ftello(f->fp);
	if ((end == (off_t)-1) || (end > SSIZE_MAX))
		return -1;

	if (fseeko(f->fp, here, SEEK_SET) < 0) {
		b6b_return_strerror(interp, errno);
		return -1;
	}

	return (ssize_t)(end - here);
}

static ssize_t b6b_file_read(struct b6b_interp *interp,
                             void *priv,
                             unsigned char *buf,
                             const size_t len,
                             int *eof,
                             int *again)
{
	const struct b6b_file *f = (const struct b6b_file *)priv;
	size_t ret;

	ret = fread(buf, 1, len, f->fp);
	if (ret < len) {
		if (ferror(f->fp)) {
			b6b_return_strerror(interp, errno);
			return -1;
		}
		if (feof(f->fp))
			*eof = 1;
	}

	return (ssize_t)ret;
}

static ssize_t b6b_file_write(struct b6b_interp *interp,
                              void *priv,
                              const unsigned char *buf,
                              const size_t len)
{
	const struct b6b_file *f = (const struct b6b_file *)priv;
	size_t chunk, total = 0;

	do {
		chunk = fwrite(buf + total, 1, len - total, f->fp);
		if (chunk == 0) {
			if (ferror(f->fp)) {
				b6b_return_strerror(interp, errno);
				return -1;
			}
			break;
		}
		total += chunk;
	} while (total < len);

	return total;
}

static int b6b_file_fd(void *priv)
{
	const struct b6b_file *f = (const struct b6b_file *)priv;

	return f->fd;
}

static void b6b_file_close(void *priv)
{
	struct b6b_file *f = (struct b6b_file *)priv;

	fclose(f->fp);

	if (f->buf)
		free(f->buf);

	free(f);
}

static const struct b6b_strm_ops b6b_file_ops = {
	.peeksz = b6b_file_peeksz,
	.read = b6b_file_read,
	.write = b6b_file_write,
	.fd = b6b_file_fd,
	.close = b6b_file_close
};

static struct b6b_obj *b6b_file_new(struct b6b_interp *interp,
                                    FILE *fp,
                                    const int fd,
                                    const int bmode)
{
	struct b6b_obj *o;
	struct b6b_strm *strm;
	struct b6b_file *f;

	f = (struct b6b_file *)malloc(sizeof(*f));
	if (b6b_unlikely(!f))
		return NULL;

	strm = (struct b6b_strm *)malloc(sizeof(struct b6b_strm));
	if (b6b_unlikely(!strm)) {
		free(f);
		return NULL;
	}

	strm->ops = &b6b_file_ops;
	strm->flags = 0;
	strm->priv = f;

	f->fp = fp;
	f->buf = NULL;
	f->fd = fd;

	o = b6b_strm_fmt(interp, strm, "file");
	if (b6b_unlikely(!o)) {
		b6b_strm_destroy(strm);
		return NULL;
	}

	if (bmode == _IOFBF) {
		f->buf = malloc(B6B_STRM_BUFSIZ);
		if (b6b_unlikely(!f->buf)) {
			b6b_strm_destroy(strm);
			return NULL;
		}

		if (setvbuf(fp, f->buf, _IOFBF, B6B_STRM_BUFSIZ) != 0) {
			b6b_strm_destroy(strm);
			return NULL;
		}
	} else if (bmode == _IONBF)
		setbuf(fp, NULL);

	return o;
}

static const char *b6b_file_mode(const char *mode, int *bmode)
{
	if ((strcmp("r", mode) == 0) ||
	    (strcmp("w", mode) == 0) ||
	    (strcmp("a", mode) == 0)) {
		*bmode = _IOLBF;
		return mode;
	}
	else if (strcmp("rb", mode) == 0) {
		*bmode = _IOFBF;
		return "r";
	}
	else if (strcmp("wb", mode) == 0) {
		*bmode = _IOFBF;
		return "w";
	}
	else if (strcmp("ab", mode) == 0) {
		*bmode = _IOFBF;
		return "a";
	}
	else if (strcmp("ru", mode) == 0) {
		*bmode = _IONBF;
		return "r";
	}
	else if (strcmp("wu", mode) == 0) {
		*bmode = _IONBF;
		return "w";
	}
	else if (strcmp("au", mode) == 0) {
		*bmode = _IONBF;
		return "a";
	}
	else if ((strcmp("r+", mode) == 0) ||
	         (strcmp("w+", mode) == 0) ||
	         (strcmp("a+", mode) == 0)) {
		*bmode = _IOLBF;
		return mode;
	}
	else if (strcmp("r+b", mode) == 0) {
		*bmode = _IOFBF;
		return "r+";
	}
	else if (strcmp("w+b", mode) == 0) {
		*bmode = _IOFBF;
		return "w+";
	}
	else if (strcmp("a+b", mode) == 0) {
		*bmode = _IOFBF;
		return "a+";
	}
	else if (strcmp("r+u", mode) == 0) {
		*bmode = _IONBF;
		return "r+";
	}
	else if (strcmp("w+u", mode) == 0) {
		*bmode = _IONBF;
		return "w+";
	}
	else if (strcmp("a+u", mode) == 0) {
		*bmode = _IONBF;
		return "a+";
	}

	return NULL;
}

static enum b6b_res b6b_file_proc_open(struct b6b_interp *interp,
                                       struct b6b_obj *args)

{
	struct b6b_obj *path, *mode, *f;
	const char *rmode = B6B_FILE_DEF_FMODE;
	FILE *fp;
	int fd, fl, err, bmode = _IONBF;

	switch (b6b_proc_get_args(interp, args, "o s s", NULL, &path, &mode)) {
		case 3:
			if (!b6b_as_str(mode))
				return B6B_ERR;

			rmode = b6b_file_mode(mode->s, &bmode);
			if (!rmode)
				return b6b_return_strerror(interp, EINVAL);

		case 2:
			fp = fopen(path->s, rmode);
			if (!fp)
				return b6b_return_strerror(interp, errno);

			fd = fileno(fp);
			fl = fcntl(fd, F_GETFL);
			if ((fl < 0) || (fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0)) {
				err = errno;
				fclose(fp);
				return b6b_return_strerror(interp, err);
			}

			f = b6b_file_new(interp, fp, fd, bmode);
			if (!f) {
				fclose(fp);
				return B6B_ERR;
			}

			return b6b_return(interp, f);
	}

	return B6B_ERR;
}

static const struct b6b_ext_obj b6b_file[] = {
	{
		.name = "open",
		.type = B6B_OBJ_STR,
		.val.s = "open",
		.proc = b6b_file_proc_open
	}
};
__b6b_ext(b6b_file);

static const struct b6b_strm_ops b6b_stdio_ops = {
	.read = b6b_file_read,
	.write = b6b_file_write,
	.fd = b6b_file_fd,
	.close = free
};

static int b6b_stdio_wrap(struct b6b_interp *interp,
                          const char *name,
                          const size_t len,
                          FILE *fp,
                          const int fd)
{
	struct b6b_obj *o;
	struct b6b_file *f;
	struct b6b_strm *strm;

	f = (struct b6b_file *)malloc(sizeof(*f));
	if (b6b_unlikely(!f))
		return 0;

	strm = (struct b6b_strm *)malloc(sizeof(*strm));
	if (b6b_unlikely(!strm)) {
		free(f);
		return 0;
	}

	strm->ops = &b6b_stdio_ops;
	strm->flags = 0;
	strm->priv = f;

	f->fp = fp;
	f->buf = NULL;
	f->fd = fd;

	o = b6b_strm_copy(interp, strm, name, len);
	if (b6b_unlikely(!o)) {
		b6b_strm_destroy(strm);
		return 0;
	}
	b6b_unref(o);

	return 1;
}

#define B6B_STDIO_WRAP(interp, fp, fd) \
	b6b_stdio_wrap(interp, #fp, sizeof(#fp) - 1, fp, fd)

static int b6b_file_init(struct b6b_interp *interp)
{
	return B6B_STDIO_WRAP(interp, stdin, STDIN_FILENO) &&
	       B6B_STDIO_WRAP(interp, stdout, STDOUT_FILENO) &&
	       B6B_STDIO_WRAP(interp, stderr, STDERR_FILENO);
}
__b6b_init(b6b_file_init);
