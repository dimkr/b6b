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

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/ioctl.h>

#include <b6b.h>

#define B6B_SERVER_DEF_BACKLOG 5

struct b6b_socket {
	struct sockaddr peer;
	int fd;
	socklen_t len;
};

static ssize_t b6b_socket_peeksz(struct b6b_interp *interp, void *priv)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;
	int ilen;

	if (ioctl(s->fd, FIONREAD, &ilen) < 0)
		return -1;

	return (ssize_t)ilen;
}

static ssize_t b6b_socket_readfrom(struct b6b_interp *interp,
                                   void *priv,
                                   unsigned char *buf,
                                   const size_t len,
                                   int *eof,
                                   int *again)
{
	struct b6b_socket *s = (struct b6b_socket *)priv;

	*again = 0;
	s->len = sizeof(s->peer);
	return b6b_fd_on_read(interp,
	                      recvfrom(s->fd, buf, len, 0, &s->peer, &s->len),
	                      eof);
}

static struct b6b_obj *b6b_socket_peer(struct b6b_interp *interp, void *priv)
{
	char *buf;
	struct b6b_obj *o, *l;
	const struct b6b_socket *s = (const struct b6b_socket *)priv;
	struct sockaddr_in *sin = (struct sockaddr_in *)&s->peer;
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&s->peer;
	const void *addr = &sin->sin_addr;
	size_t len = INET6_ADDRSTRLEN + sizeof(":65535");
	unsigned short port = ntohs(sin->sin_port);

	if (!s->len)
		return NULL;

	buf = (char *)malloc(len);
	if (b6b_unlikely(!buf))
		return NULL;

	switch (s->peer.sa_family) {
		case AF_INET:
			break;

		case AF_INET6:
			addr = &sin6->sin6_addr;
			port = ntohs(sin6->sin6_port);
			break;

		default:
			free(buf);
			return NULL;
	}

	if (!inet_ntop(s->peer.sa_family, addr, buf, len)) {
		free(buf);
		return NULL;
	}

	o = b6b_str_new(buf, strlen(buf));
	if (b6b_unlikely(!o)) {
		free(buf);
		return NULL;
	}

	l = b6b_list_new();
	if (b6b_unlikely(!l)) {
		b6b_destroy(o);
		return NULL;
	}

	if (b6b_unlikely(!b6b_list_add(l, o))) {
		b6b_destroy(l);
		b6b_destroy(o);
		return NULL;
	}
	b6b_unref(o);

	o = b6b_num_new((b6b_num)port);
	if (b6b_unlikely(!o)) {
		b6b_destroy(l);
		return NULL;
	}

	if (b6b_unlikely(!b6b_list_add(l, o))) {
		b6b_destroy(l);
		b6b_destroy(o);
		return NULL;
	}

	b6b_unref(o);
	return l;
}

static int b6b_socket_fd(void *priv)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;

	return s->fd;
}

static void b6b_socket_close(void *priv)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;

	close(s->fd);
	free(priv);
}

static struct b6b_obj *b6b_socket_new(struct b6b_interp *interp,
                                      const int fd,
                                      const struct sockaddr *peer,
                                      const socklen_t len,
                                      const char *type,
                                      const struct b6b_strm_ops *ops)
{
	struct b6b_obj *o;
	struct b6b_strm *strm;
	struct b6b_socket *s;

	s = (struct b6b_socket *)malloc(sizeof(*s));
	if (b6b_unlikely(!s))
		return NULL;

	strm = (struct b6b_strm *)malloc(sizeof(struct b6b_strm));
	if (b6b_unlikely(!strm)) {
		free(s);
		return NULL;
	}

	strm->ops = ops;
	strm->flags = 0;
	strm->priv = s;

	s->fd = fd;
	s->len = len;
	memcpy(&s->peer, peer, (size_t)len);

	o = b6b_strm_fmt(interp, strm, type);
	if (b6b_unlikely(!o))
		b6b_strm_destroy(strm);

	return o;
}

static struct addrinfo *b6b_socket_resolve(struct b6b_interp *interp,
                                           const char *host,
                                           const char *service,
                                           const int socktype)
{
	struct addrinfo hints = {0}, *res;
	const char *s;
	int out;

	hints.ai_socktype = socktype;
	hints.ai_family = AF_UNSPEC;

	out = getaddrinfo(host, service, &hints, &res);
	if (out != 0) {
		s = gai_strerror(out);
		if (s)
			b6b_return_str(interp, s, strlen(s));
		return NULL;
	}

	return res;
}

static struct b6b_obj *b6b_client_socket_new(struct b6b_interp *interp,
                                             const char *host,
                                             const char *service,
                                             const int socktype,
                                             int backlog,
                                             const struct b6b_strm_ops *ops,
                                             const char *type)
{
	struct addrinfo *res;
	struct b6b_obj *o;
	int fd, err;

	res = b6b_socket_resolve(interp, host, service, socktype);
	if (!res)
		return NULL;

	fd = socket(res->ai_family,
	            res->ai_socktype | SOCK_NONBLOCK,
	            res->ai_protocol);
	if (fd < 0) {
		err = errno;
		freeaddrinfo(res);
		b6b_return_strerror(interp, err);
		return NULL;
	}

	if ((connect(fd, res->ai_addr, res->ai_addrlen) < 0) &&
	    (errno != EINPROGRESS)) {
		err = errno;
		close(fd);
		freeaddrinfo(res);
		b6b_return_strerror(interp, err);
		return NULL;
	}

	o = b6b_socket_new(interp, fd, res->ai_addr, res->ai_addrlen, type, ops);
	if (b6b_unlikely(!o))
		close(fd);

	freeaddrinfo(res);
	return o;
}

static enum b6b_res b6b_socket_proc(struct b6b_interp *interp,
                                    struct b6b_obj *args,
                                    const int socktype,
                                    int backlog,
                                    const struct b6b_strm_ops *ops,
                                    struct b6b_obj *(*fn)(
                                                    struct b6b_interp *,
                                                    const char *,
                                                    const char *,
                                                    const int,
                                                    int,
                                                    const struct b6b_strm_ops *,
                                                    const char *))
{
	struct b6b_obj *p, *h, *s, *b, *o;

	switch (b6b_proc_get_args(interp, args, "o s s |i", &p, &h, &s, &b)) {
		case 3:
			break;

		case 4:
			/* the 4th parameter shall not be passed if listen() is not
			 * called */
			if (!backlog || ((b->n < 0) || (b->n > INT_MAX)))
				return B6B_ERR;

			backlog = (int)b->n;
			break;

		default:
			return B6B_ERR;
	}

	o = fn(interp, h->s, s->s, socktype, backlog, ops, p->s);
	if (!o)
		return B6B_ERR;

	return b6b_return(interp, o);

}

static const struct b6b_strm_ops b6b_stream_client_ops = {
	.peeksz = b6b_socket_peeksz,
	.read = b6b_fd_recv,
	.write = b6b_fd_send,
	.peer = b6b_socket_peer,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close
};

static enum b6b_res b6b_socket_proc_stream_client(struct b6b_interp *interp,
                                                  struct b6b_obj *args)
{
	return b6b_socket_proc(interp,
	                       args,
	                       SOCK_STREAM,
	                       0,
	                       &b6b_stream_client_ops,
	                       b6b_client_socket_new);
}

static const struct b6b_strm_ops b6b_dgram_client_ops = {
	.peeksz = b6b_socket_peeksz,
	.read = b6b_socket_readfrom,
	.write = b6b_fd_send,
	.peer = b6b_socket_peer,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close
};

static enum b6b_res b6b_socket_proc_dgram_client(struct b6b_interp *interp,
                                                 struct b6b_obj *args)
{
	return b6b_socket_proc(interp,
	                       args,
	                       SOCK_DGRAM,
	                       0,
	                       &b6b_dgram_client_ops,
	                       b6b_client_socket_new);
}

static struct b6b_obj *b6b_server_socket_new(struct b6b_interp *interp,
                                             const char *host,
                                             const char *service,
                                             const int socktype,
                                             int backlog,
                                             const struct b6b_strm_ops *ops,
                                             const char *type)
{
	struct addrinfo *res;
	struct b6b_obj *o;
	int fd, err, one = 1;

	res = b6b_socket_resolve(interp, host, service, socktype);
	if (!res)
		return NULL;

	fd = socket(res->ai_family,
	            res->ai_socktype | SOCK_NONBLOCK,
	            res->ai_protocol);
	if (fd < 0) {
		err = errno;
		freeaddrinfo(res);
		b6b_return_strerror(interp, err);
		return NULL;
	}

	if ((setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) ||
		(bind(fd, res->ai_addr, res->ai_addrlen) < 0) ||
		(backlog && (listen(fd, backlog) < 0))) {
		err = errno;
		close(fd);
		freeaddrinfo(res);
		b6b_return_strerror(interp, err);
		return NULL;
	}

	o = b6b_socket_new(interp, fd, NULL, 0, type, ops);
	if (b6b_unlikely(!o))
		close(fd);

	freeaddrinfo(res);
	return o;
}

static int b6b_stream_socket_accept(struct b6b_interp *interp,
                                    void *priv,
                                    struct b6b_obj **o)
{
	struct sockaddr peer;
	const struct b6b_socket *s = (const struct b6b_socket *)priv;
	int fd, fl, err;
	socklen_t len = sizeof(peer);

	fd = accept(s->fd, &peer, &len);
	if (fd < 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
			*o = NULL;
			return 1;
		}

		b6b_return_strerror(interp, errno);
		return 0;
	}

	fl = fcntl(fd, F_GETFL);
	if ((fl < 0) || (fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0)) {
		err = errno;
		close(fd);
		b6b_return_strerror(interp, err);
		return 0;
	}

	*o = b6b_socket_new(interp,
	                    fd,
	                    &peer,
	                    len,
	                    "stream.client",
	                    &b6b_stream_client_ops);
	if (*o)
		return 1;

	return 0;
}

static const struct b6b_strm_ops b6b_stream_server_ops = {
	.accept = b6b_stream_socket_accept,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close
};

static enum b6b_res b6b_socket_proc_stream_server(struct b6b_interp *interp,
                                                  struct b6b_obj *args)
{
	return b6b_socket_proc(interp,
	                       args,
	                       SOCK_STREAM,
	                       B6B_SERVER_DEF_BACKLOG,
	                       &b6b_stream_server_ops,
	                       b6b_server_socket_new);
}

static const struct b6b_strm_ops b6b_dgram_server_ops = {
	.peeksz = b6b_socket_peeksz,
	.read = b6b_socket_readfrom,
	.peer = b6b_socket_peer,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close
};

static enum b6b_res b6b_socket_proc_dgram_server(struct b6b_interp *interp,
                                                 struct b6b_obj *args)
{
	return b6b_socket_proc(interp,
	                       args,
	                       SOCK_DGRAM,
	                       0,
	                       &b6b_dgram_server_ops,
	                       b6b_server_socket_new);
}

static const struct b6b_ext_obj b6b_socket[] = {
	{
		.name = "stream.client",
		.type = B6B_OBJ_STR,
		.val.s = "stream.client",
		.proc = b6b_socket_proc_stream_client
	},
	{
		.name = "dgram.client",
		.type = B6B_OBJ_STR,
		.val.s = "dgram.client",
		.proc = b6b_socket_proc_dgram_client
	},
	{
		.name = "stream.server",
		.type = B6B_OBJ_STR,
		.val.s = "stream.server",
		.proc = b6b_socket_proc_stream_server
	},
	{
		.name = "dgram.server",
		.type = B6B_OBJ_STR,
		.val.s = "dgram.server",
		.proc = b6b_socket_proc_dgram_server
	},
};
__b6b_ext(b6b_socket);
