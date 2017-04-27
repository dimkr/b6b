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

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <b6b.h>

ssize_t b6b_fd_on_read(struct b6b_interp *interp,
                       const ssize_t out,
                       int *eof)
{
	if (out < 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
			return 0;

		b6b_return_strerror(interp, errno);
	} else if (out == 0)
		*eof = 0;

	return out;
}

ssize_t b6b_fd_read(struct b6b_interp *interp,
                    void *priv,
                    unsigned char *buf,
                    const size_t len,
                    int *eof,
                    int *again)
{
	return b6b_fd_on_read(interp, read((int)(intptr_t)priv, buf, len), eof);
}

ssize_t b6b_fd_recv(struct b6b_interp *interp,
                    void *priv,
                    unsigned char *buf,
                    const size_t len,
                    int *eof,
                    int *again)
{
	return b6b_fd_on_read(interp, recv((int)(intptr_t)priv, buf, len, 0), eof);
}

static ssize_t b6b_fd_on_write(struct b6b_interp *interp, const ssize_t out)
{
	if (out < 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
			return 0;

		b6b_return_strerror(interp, errno);
	}

	return out;
}

ssize_t b6b_fd_write(struct b6b_interp *interp,
                    void *priv,
                    const unsigned char *buf,
                    const size_t len)
{
	return b6b_fd_on_write(interp, write((int)(intptr_t)priv, buf, len));
}

ssize_t b6b_fd_send(struct b6b_interp *interp,
                    void *priv,
                    const unsigned char *buf,
                    const size_t len)
{
	return b6b_fd_on_write(interp,
	                       send((int)(intptr_t)priv, buf, len, MSG_NOSIGNAL));
}

int b6b_fd_fd(void *priv)
{
	return (int)(intptr_t)priv;
}

void b6b_fd_close(void *priv)
{
	close((int)(intptr_t)priv);
}
