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
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>

#include <b6b.h>

#define B6B_SERVER_DEF_BACKLOG 5

struct b6b_socket {
	struct sockaddr_storage peer;
	struct sockaddr_storage addr;
	int fd;
};

static ssize_t b6b_socket_peeksz(struct b6b_interp *interp, void *priv)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;

	return b6b_fd_peeksz(interp, (void *)(intptr_t)s->fd);
}

static ssize_t b6b_socket_read(struct b6b_interp *interp,
                               void *priv,
                               unsigned char *buf,
                               const size_t len,
                               int *eof,
                               int *again)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;

	return b6b_fd_recv(interp, (void *)(intptr_t)s->fd, buf, len, eof, again);
}

static ssize_t b6b_socket_readfrom(struct b6b_interp *interp,
                                   void *priv,
                                   unsigned char *buf,
                                   const size_t len,
                                   int *eof,
                                   int *again)
{
	struct b6b_socket *s = (struct b6b_socket *)priv;
	socklen_t plen = sizeof(s->peer);

	*again = 0;
	return b6b_fd_on_read(interp,
	                      recvfrom(s->fd,
	                               buf,
	                               len,
	                               0,
	                               (struct sockaddr *)&s->peer,
	                               &plen),
	                      eof);
}

static ssize_t b6b_socket_write(struct b6b_interp *interp,
                                void *priv,
                                const unsigned char *buf,
                                const size_t len)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;

	return b6b_fd_send(interp, (void *)(intptr_t)s->fd, buf, len);
}

static struct b6b_obj *b6b_socket_inet_peer(struct b6b_interp *interp,
                                            void *priv)
{
	char *buf;
	struct b6b_obj *o, *l;
	const struct b6b_socket *s = (const struct b6b_socket *)priv;
	struct sockaddr_in *sin = (struct sockaddr_in *)&s->peer;
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&s->peer;
	const void *addr = &sin->sin_addr;
	size_t len = INET6_ADDRSTRLEN + sizeof(":65535");
	unsigned short port = ntohs(sin->sin_port);

	buf = (char *)malloc(len);
	if (b6b_unlikely(!buf))
		return NULL;

	switch (s->peer.ss_family) {
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

	if (!inet_ntop(s->peer.ss_family, addr, buf, len)) {
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

	o = b6b_int_new((b6b_int)port);
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

static struct b6b_obj *b6b_socket_file_peer(struct b6b_interp *interp,
                                            void *priv)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;
	const struct sockaddr_un *sun = (const struct sockaddr_un *)&s->peer;

	return b6b_str_copy(sun->sun_path, strlen(sun->sun_path));
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

static void b6b_socket_close_unlink(void *priv)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;
	const struct sockaddr_un *sun = (const struct sockaddr_un *)&s->addr;

	close(s->fd);
	unlink(sun->sun_path);
	free(priv);
}

static struct b6b_obj *b6b_socket_new(struct b6b_interp *interp,
                                      const int fd,
                                      const struct sockaddr *addr,
                                      const socklen_t alen,
                                      const struct sockaddr *peer,
                                      const socklen_t plen,
                                      const char *type,
                                      const struct b6b_strm_ops *ops)
{
	struct b6b_obj *o;
	struct b6b_socket *s;

	if (plen > sizeof(s->peer)) {
		close(fd);
		return NULL;
	}

	s = (struct b6b_socket *)malloc(sizeof(*s));
	if (b6b_unlikely(!s)) {
		close(fd);
		return NULL;
	}

	s->fd = fd;
	memcpy(&s->addr, addr, (size_t)alen);
	if (peer)
		memcpy(&s->peer, peer, (size_t)plen);

	o = b6b_strm_fmt(interp, ops, s, type);
	if (b6b_unlikely(!o))
		b6b_socket_close(s);

	return o;
}

struct b6b_socket_gai_data {
	struct addrinfo hints;
	char *host;
	char *service;
	struct addrinfo *res;
	int out;
};

static void b6b_socket_do_getaddrinfo(void *arg)
{
	struct b6b_socket_gai_data *data = (struct b6b_socket_gai_data *)arg;

	data->out = getaddrinfo(data->host,
	                        data->service,
	                        &data->hints,
	                        &data->res);
}

static struct addrinfo *b6b_socket_resolve(struct b6b_interp *interp,
                                           struct b6b_obj *host,
                                           struct b6b_obj *service,
                                           const int socktype)
{
	struct b6b_socket_gai_data data = {.hints = {0}};
	const char *s;
	int out;

	/* both strings must be copied since they may be freed during
	 * b6b_offload() */
	data.host = b6b_strndup(host->s, host->slen);
	if (b6b_unlikely(!data.host))
		return NULL;

	data.service = b6b_strndup(service->s, service->slen);
	if (b6b_unlikely(!data.service)) {
		free(data.host);
		return NULL;
	}

	data.hints.ai_socktype = socktype;
	data.hints.ai_family = AF_UNSPEC;
	out = b6b_offload(interp, b6b_socket_do_getaddrinfo, &data);

	free(data.service);
	free(data.host);

	if (!out) {
		if (data.res)
			freeaddrinfo(data.res);
		return NULL;
	}

	if (data.out == 0)
		return data.res;

	s = gai_strerror(data.out);
	if (s)
		b6b_return_str(interp, s, strlen(s));

	return NULL;
}

static struct b6b_obj *b6b_socket_client_new(struct b6b_interp *interp,
                                             const struct addrinfo *res,
                                             const struct b6b_strm_ops *ops,
                                             const char *type)
{
	struct sockaddr_storage ss = {.ss_family = res->ai_socktype};
	socklen_t sslen = sizeof(ss);
	int fd, err;

	fd = socket(res->ai_family,
	            res->ai_socktype | SOCK_NONBLOCK | SOCK_CLOEXEC,
	            res->ai_protocol);
	if (fd < 0) {
		b6b_return_strerror(interp, errno);
		return NULL;
	}

	if (((connect(fd, res->ai_addr, res->ai_addrlen) < 0) &&
	     (errno != EINPROGRESS)) ||
	    (getsockname(fd, (struct sockaddr *)&ss, &sslen) < 0)) {
		err = errno;
		close(fd);
		b6b_return_strerror(interp, err);
		return NULL;
	}

	return b6b_socket_new(interp,
	                      fd,
	                      (const struct sockaddr *)&ss,
	                      sslen,
	                      res->ai_addr,
	                      res->ai_addrlen,
	                      type,
	                      ops);
}

static const struct b6b_strm_ops b6b_tcp_client_ops = {
	.peeksz = b6b_socket_peeksz,
	.read = b6b_socket_read,
	.write = b6b_socket_write,
	.peer = b6b_socket_inet_peer,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close
};

static const struct b6b_strm_ops b6b_udp_client_ops = {
	.peeksz = b6b_socket_peeksz,
	.read = b6b_socket_readfrom,
	.write = b6b_socket_write,
	.peer = b6b_socket_inet_peer,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close
};

static enum b6b_res b6b_socket_proc_inet_client(struct b6b_interp *interp,
                                                struct b6b_obj *args)
{
	struct b6b_obj *t, *h, *s, *o;
	struct addrinfo *res;
	const struct b6b_strm_ops *ops = &b6b_tcp_client_ops;

	if (!b6b_proc_get_args(interp, args, "osss", NULL, &t, &h, &s))
		return B6B_ERR;

	if (strcmp(t->s, "udp") == 0) {
		res = b6b_socket_resolve(interp, h, s, SOCK_DGRAM);
		ops = &b6b_udp_client_ops;
	} else if (strcmp(t->s, "tcp") == 0)
		res = b6b_socket_resolve(interp, h, s, SOCK_STREAM);
	else
		return B6B_ERR;

	if (!res)
		return B6B_ERR;

	o = b6b_socket_client_new(interp, res, ops, t->s);
	freeaddrinfo(res);

	if (!o)
		return B6B_ERR;

	return b6b_return(interp, o);
}

static const struct b6b_strm_ops b6b_un_stream_client_ops = {
	.peeksz = b6b_socket_peeksz,
	.read = b6b_socket_read,
	.write = b6b_socket_write,
	.peer = b6b_socket_file_peer,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close
};

static const struct b6b_strm_ops b6b_un_dgram_client_ops = {
	.peeksz = b6b_socket_peeksz,
	.read = b6b_socket_readfrom,
	.write = b6b_socket_write,
	.peer = b6b_socket_file_peer,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close
};

static enum b6b_res b6b_socket_proc_un_client(struct b6b_interp *interp,
                                              struct b6b_obj *args)
{
	struct sockaddr_un sun = {.sun_family = AF_UNIX};
	struct addrinfo res = {
		.ai_family = AF_UNIX,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
		.ai_addr = (struct sockaddr *)&sun,
		.ai_addrlen = sizeof(sun)
	};
	struct b6b_obj *t, *p, *o;
	const struct b6b_strm_ops *ops = &b6b_un_stream_client_ops;

	if (!b6b_proc_get_args(interp, args, "oss", NULL, &t, &p))
		return B6B_ERR;

	if (strcmp(t->s, "dgram") == 0) {
		res.ai_socktype = SOCK_DGRAM;
		ops = &b6b_un_dgram_client_ops;
	} else if (strcmp(t->s, "stream"))
		return B6B_ERR;

	strncpy(sun.sun_path, p->s, sizeof(sun.sun_path) - 1);
	sun.sun_path[sizeof(sun.sun_path) - 1] = '\0';
	o = b6b_socket_client_new(interp, &res, ops, t->s);
	if (!o)
		return B6B_ERR;

	return b6b_return(interp, o);
}

static struct b6b_obj *b6b_server_socket_new(struct b6b_interp *interp,
                                             const struct addrinfo *res,
                                             const b6b_int backlog,
                                             const struct b6b_strm_ops *stream_ops,
                                             const struct b6b_strm_ops *dgram_ops,
                                             const char *type)
{
	const struct b6b_strm_ops *ops = stream_ops;
	int fd, err, one = 1;

	if (res->ai_socktype == SOCK_DGRAM)
		ops = dgram_ops;
	else if ((backlog <= 0) || (backlog > INT_MAX))
		return NULL;

	fd = socket(res->ai_family,
	            res->ai_socktype | SOCK_NONBLOCK | SOCK_CLOEXEC,
	            res->ai_protocol);
	if (fd < 0) {
		b6b_return_strerror(interp, errno);
		return NULL;
	}

	if ((setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) ||
		(bind(fd, res->ai_addr, res->ai_addrlen) < 0) ||
		((res->ai_socktype == SOCK_STREAM) && (listen(fd, (int)backlog) < 0))) {
		err = errno;
		close(fd);
		b6b_return_strerror(interp, err);
		return NULL;
	}

	return b6b_socket_new(interp,
	                      fd,
	                      res->ai_addr,
	                      res->ai_addrlen,
	                      NULL,
	                      0,
	                      type,
	                      ops);
}

static int b6b_socket_stream_accept(struct b6b_interp *interp,
                                    void *priv,
                                    struct b6b_obj **o,
                                    const struct b6b_strm_ops *ops,
                                    const char *type)
{
	struct sockaddr_storage addr, peer;
	socklen_t alen = sizeof(addr), plen = sizeof(peer);
	const struct b6b_socket *s = (const struct b6b_socket *)priv;
	int fd, fl, err;

	fd = accept(s->fd, (struct sockaddr *)&peer, &plen);
	if (fd < 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EMFILE)) {
			*o = NULL;
			return 1;
		}

		b6b_return_strerror(interp, errno);
		return 0;
	}

	if (getpeername(fd, (struct sockaddr *)&addr, &alen) < 0) {
		err = errno;
		close(fd);
		b6b_return_strerror(interp, err);
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
	                    (const struct sockaddr *)&addr,
	                    alen,
	                    (const struct sockaddr *)&peer,
	                    plen,
	                    type,
	                    ops);
	if (*o)
		return 1;

	return 0;
}

static int b6b_socket_inet_accept(struct b6b_interp *interp,
                                  void *priv,
                                  struct b6b_obj **o)
{
	return b6b_socket_stream_accept(interp,
	                                priv,
	                                o,
	                                &b6b_tcp_client_ops,
	                                "tcp");
}

static int b6b_socket_un_accept(struct b6b_interp *interp,
                                void *priv,
                                struct b6b_obj **o)
{
	return b6b_socket_stream_accept(interp,
	                                priv,
	                                o,
	                                &b6b_un_stream_client_ops,
	                                "stream");
}

static const struct b6b_strm_ops b6b_tcp_server_ops = {
	.accept = b6b_socket_inet_accept,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close
};

static const struct b6b_strm_ops b6b_udp_server_ops = {
	.peeksz = b6b_socket_peeksz,
	.read = b6b_socket_readfrom,
	.peer = b6b_socket_inet_peer,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close
};

static enum b6b_res b6b_socket_proc_inet_server(struct b6b_interp *interp,
                                                struct b6b_obj *args)
{
	struct b6b_obj *t, *h, *s, *b, *o;
	struct addrinfo *res;
	b6b_int backlog = B6B_SERVER_DEF_BACKLOG;
	unsigned int argc;

	argc = b6b_proc_get_args(interp, args, "osss|i", NULL, &t, &h, &s, &b);
	if (argc < 4)
		return B6B_ERR;

	if ((argc == 4) && (strcmp(t->s, "udp") == 0))
		res = b6b_socket_resolve(interp, h, s, SOCK_DGRAM);
	else if (strcmp(t->s, "tcp") == 0) {
		if (argc == 5)
			backlog = b->i;

		res = b6b_socket_resolve(interp, h, s, SOCK_STREAM);
	}
	else
		return B6B_ERR;

	if (!res)
		return B6B_ERR;

	o = b6b_server_socket_new(interp,
	                          res,
	                          backlog,
	                          &b6b_tcp_server_ops,
	                          &b6b_udp_server_ops,
	                          t->s);
	freeaddrinfo(res);

	if (!o)
		return B6B_ERR;

	return b6b_return(interp, o);
}

static const struct b6b_strm_ops b6b_un_stream_server_ops = {
	.accept = b6b_socket_un_accept,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close_unlink
};

static const struct b6b_strm_ops b6b_un_dgram_server_ops = {
	.peeksz = b6b_socket_peeksz,
	.read = b6b_socket_readfrom,
	.peer = b6b_socket_file_peer,
	.fd = b6b_socket_fd,
	.close = b6b_socket_close_unlink
};

static enum b6b_res b6b_socket_proc_un_server(struct b6b_interp *interp,
                                              struct b6b_obj *args)
{
	struct sockaddr_un sun = {.sun_family = AF_UNIX};
	struct addrinfo res = {
		.ai_family = AF_UNIX,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
		.ai_addr = (struct sockaddr *)&sun,
		.ai_addrlen = sizeof(sun)
	};
	struct b6b_obj *t, *p, *b, *o;
	b6b_int backlog = B6B_SERVER_DEF_BACKLOG;
	unsigned int argc;

	argc = b6b_proc_get_args(interp, args, "oss|i", NULL, &t, &p, &b);
	if (argc < 3)
		return B6B_ERR;

	if ((argc == 3) && (strcmp(t->s, "dgram") == 0)) {
		res.ai_socktype = SOCK_DGRAM;
		backlog = 0;
	}
	else if (strcmp(t->s, "stream") == 0) {
		if (argc == 4)
			backlog = b->i;
	}
	else
		return B6B_ERR;

	strncpy(sun.sun_path, p->s, sizeof(sun.sun_path) - 1);
	sun.sun_path[sizeof(sun.sun_path) - 1] = '\0';
	o = b6b_server_socket_new(interp,
	                          &res,
	                          backlog,
	                          &b6b_un_stream_server_ops,
	                          &b6b_un_dgram_server_ops,
	                          t->s);
	if (!o)
		return B6B_ERR;

	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_socket[] = {
	{
		.name = "inet.client",
		.type = B6B_TYPE_STR,
		.val.s = "inet.client",
		.proc = b6b_socket_proc_inet_client
	},
	{
		.name = "un.client",
		.type = B6B_TYPE_STR,
		.val.s = "un.client",
		.proc = b6b_socket_proc_un_client
	},
	{
		.name = "inet.server",
		.type = B6B_TYPE_STR,
		.val.s = "inet.server",
		.proc = b6b_socket_proc_inet_server
	},
	{
		.name = "un.server",
		.type = B6B_TYPE_STR,
		.val.s = "un.server",
		.proc = b6b_socket_proc_un_server
	}
};
__b6b_ext(b6b_socket);
