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

#include <sys/eventfd.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>

#include <b6b.h>

static ssize_t b6b_sem_read(struct b6b_interp *interp,
                            void *priv,
                            unsigned char *buf,
                            const size_t len,
                            int *eof,
                            int *again)
{
	uint64_t c;
	ssize_t out;

	out = b6b_fd_read(interp, priv, (unsigned char *)&c, sizeof(c), eof, again);
	if (out <= 0)
		return out;

	*again = 0;
	return 0;
}

static ssize_t b6b_sem_write(struct b6b_interp *interp,
                             void *priv,
                             const unsigned char *buf,
                             const size_t len)
{
	unsigned long long ull;
	char *end;
	uint64_t c;
	ssize_t out;

	errno = 0;
	ull = strtoull((const char *)buf, &end, 0);
	if (errno || (end == (char *)buf) || *end || (ull > UINT64_MAX))
		return -1;

	c = (uint64_t)ull;
	out = b6b_fd_write(interp, priv, (const unsigned char *)&c, sizeof(c));
	if (out == sizeof(c))
		return (ssize_t)len;

	return -1;
}

static const struct b6b_strm_ops b6b_sem_ops = {
	.read = b6b_sem_read,
	.write = b6b_sem_write,
	.fd = b6b_fd_fd,
	.close = b6b_fd_close
};

static enum b6b_res b6b_sem_proc_sem(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *n, *o;
	int fd;

	if (!b6b_proc_get_args(interp, args, "o n", NULL, &n) ||
	    (n->n > UINT_MAX))
		return B6B_ERR;

	fd = eventfd((unsigned int)n->n, EFD_NONBLOCK | EFD_SEMAPHORE);
	if (fd < 0)
		return b6b_return_strerror(interp, errno);

	o = b6b_strm_fmt(interp, &b6b_sem_ops, (void *)(intptr_t)fd, "sem");
	if (b6b_unlikely(!o)) {
		close(fd);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_sem[] = {
	{
		.name = "sem",
		.type = B6B_OBJ_STR,
		.val.s = "sem",
		.proc = b6b_sem_proc_sem
	}
};
__b6b_ext(b6b_sem);
