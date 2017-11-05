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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <b6b.h>

#define B6B_FILE_DEF_FMODE "r"

static const struct b6b_strm_ops b6b_ro_file_ops = {
	.peeksz = b6b_stdio_peeksz,
	.read = b6b_stdio_read,
	.fd = b6b_stdio_fd,
	.close = b6b_stdio_close
};

static const struct b6b_strm_ops b6b_wo_file_ops = {
	.write = b6b_stdio_write,
	.fd = b6b_stdio_fd,
	.close = b6b_stdio_close
};

static const struct b6b_strm_ops b6b_rw_file_ops = {
	.peeksz = b6b_stdio_peeksz,
	.read = b6b_stdio_read,
	.write = b6b_stdio_write,
	.fd = b6b_stdio_fd,
	.close = b6b_stdio_close
};

static struct b6b_obj *b6b_file_new(struct b6b_interp *interp,
                                    FILE *fp,
                                    const int fd,
                                    const int bmode,
                                    const struct b6b_strm_ops *ops)
{
	struct b6b_obj *o;
	struct b6b_stdio_strm *s;

	s = (struct b6b_stdio_strm *)malloc(sizeof(*s));
	if (b6b_unlikely(!s))
		return NULL;

	s->fp = fp;
	s->interp = interp;
	s->buf = NULL;
	s->fd = fd;

	o = b6b_strm_fmt(interp, ops, s, "file");
	if (b6b_unlikely(!o)) {
		b6b_stdio_close(s);
		return NULL;
	}

	if (bmode == _IOFBF) {
		s->buf = malloc(B6B_STRM_BUFSIZ);
		if (b6b_unlikely(!s->buf)) {
			b6b_destroy(o);
			return NULL;
		}

		if (setvbuf(fp, s->buf, _IOFBF, B6B_STRM_BUFSIZ) != 0) {
			b6b_destroy(o);
			return NULL;
		}
	} else if (bmode == _IONBF)
		setbuf(fp, NULL);

	return o;
}

static const char *b6b_file_mode(const char *mode,
                                 int *bmode,
                                 const struct b6b_strm_ops **ops)
{
	if (strcmp("r", mode) == 0) {
		*bmode = _IOLBF;
		*ops = &b6b_ro_file_ops;
		return mode;
	}
	else if ((strcmp("w", mode) == 0) || (strcmp("a", mode) == 0)) {
		*bmode = _IOLBF;
		*ops = &b6b_wo_file_ops;
		return mode;
	}
	else if (strcmp("rb", mode) == 0) {
		*bmode = _IOFBF;
		*ops = &b6b_ro_file_ops;
		return "r";
	}
	else if (strcmp("wb", mode) == 0) {
		*bmode = _IOFBF;
		*ops = &b6b_wo_file_ops;
		return "w";
	}
	else if (strcmp("ab", mode) == 0) {
		*bmode = _IOFBF;
		*ops = &b6b_wo_file_ops;
		return "a";
	}
	else if (strcmp("ru", mode) == 0) {
		*bmode = _IONBF;
		*ops = &b6b_ro_file_ops;
		return "r";
	}
	else if (strcmp("wu", mode) == 0) {
		*bmode = _IONBF;
		*ops = &b6b_wo_file_ops;
		return "w";
	}
	else if (strcmp("au", mode) == 0) {
		*bmode = _IONBF;
		*ops = &b6b_wo_file_ops;
		return "a";
	}
	else if ((strcmp("r+", mode) == 0) ||
	         (strcmp("w+", mode) == 0) ||
	         (strcmp("a+", mode) == 0)) {
		*bmode = _IOLBF;
		*ops = &b6b_rw_file_ops;
		return mode;
	}
	else if (strcmp("r+b", mode) == 0) {
		*bmode = _IOFBF;
		*ops = &b6b_rw_file_ops;
		return "r+";
	}
	else if (strcmp("w+b", mode) == 0) {
		*bmode = _IOFBF;
		*ops = &b6b_rw_file_ops;
		return "w+";
	}
	else if (strcmp("a+b", mode) == 0) {
		*bmode = _IOFBF;
		*ops = &b6b_rw_file_ops;
		return "a+";
	}
	else if (strcmp("r+u", mode) == 0) {
		*bmode = _IONBF;
		*ops = &b6b_rw_file_ops;
		return "r+";
	}
	else if (strcmp("w+u", mode) == 0) {
		*bmode = _IONBF;
		*ops = &b6b_rw_file_ops;
		return "w+";
	}
	else if (strcmp("a+u", mode) == 0) {
		*bmode = _IONBF;
		*ops = &b6b_rw_file_ops;
		return "a+";
	}

	return NULL;
}

struct b6b_file_fopen_data {
	char *path;
	const char *mode;
	FILE *fp;
	int rerrno;
};

static void b6b_file_do_fopen(void *arg)
{
	struct b6b_file_fopen_data *data = (struct b6b_file_fopen_data *)arg;

	data->fp = fopen(data->path, data->mode);
	if (!data->fp)
		data->rerrno = errno;
}

static enum b6b_res b6b_file_proc_open(struct b6b_interp *interp,
                                       struct b6b_obj *args)

{
	struct b6b_file_fopen_data data = {
		.fp = NULL,
		.mode = B6B_FILE_DEF_FMODE
	};
	struct b6b_obj *path, *mode, *f;
	int err, fd, bmode = _IONBF;
	const struct b6b_strm_ops *ops = &b6b_ro_file_ops;

	switch (b6b_proc_get_args(interp, args, "os|s", NULL, &path, &mode)) {
		case 3:
			if (!b6b_as_str(mode))
				return B6B_ERR;

			data.mode = b6b_file_mode(mode->s, &bmode, &ops);
			if (!data.mode)
				return b6b_return_strerror(interp, EINVAL);

		case 2:
			/* path->s may be freed during context switch, while b6b_offload()
			 * blocks */
			data.path = b6b_strndup(path->s, path->slen);
			if (b6b_unlikely(!data.path))
				return B6B_ERR;

			if (!b6b_offload(interp, b6b_file_do_fopen, &data)) {
				if (data.fp)
					b6b_offload(interp, b6b_stdio_do_fclose, data.fp);
				free(data.path);
				return B6B_ERR;
			}

			free(data.path);

			if (!data.fp)
				return b6b_return_strerror(interp, data.rerrno);

			fd = fileno(data.fp);
			if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
				err = errno;
				b6b_offload(interp, b6b_stdio_do_fclose, data.fp);
				return b6b_return_strerror(interp, err);
			}

			f = b6b_file_new(interp, data.fp, fd, bmode, ops);
			if (!f) {
				b6b_offload(interp, b6b_stdio_do_fclose, data.fp);
				return B6B_ERR;
			}

			return b6b_return(interp, f);
	}

	return B6B_ERR;
}

static const struct b6b_ext_obj b6b_file[] = {
	{
		.name = "open",
		.type = B6B_TYPE_STR,
		.val.s = "open",
		.proc = b6b_file_proc_open
	}
};
__b6b_ext(b6b_file);
