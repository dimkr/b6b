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

#include <b6b.h>

struct b6b_socket {
	int fd;
	struct sockaddr peer;
	socklen_t len;
};

static ssize_t b6b_socket_on_read(struct b6b_interp *interp,
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

static ssize_t b6b_socket_read(struct b6b_interp *interp,
                               void *priv,
                               unsigned char *buf,
                               const size_t len,
                               int *eof)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;

	return b6b_socket_on_read(interp, recv(s->fd, buf, len, 0), eof);
}

static ssize_t b6b_socket_readfrom(struct b6b_interp *interp,
                                   void *priv,
                                   unsigned char *buf,
                                   const size_t len,
                                   int *eof)
{
	struct b6b_socket *s = (struct b6b_socket *)priv;

	s->len = sizeof(s->peer);
	return b6b_socket_on_read(interp,
	                          recvfrom(s->fd, buf, len, 0, &s->peer, &s->len),
	                          eof);
}

static ssize_t b6b_socket_write(struct b6b_interp *interp,
                                void *priv,
                                const unsigned char *buf,
                                const size_t len)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;
	ssize_t out;

	out = send(s->fd, buf, len, MSG_NOSIGNAL);
	if (out < 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
			return 0;

		b6b_return_strerror(interp, errno);
	}

	return out;

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
		return b6b_ref(interp->null);

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

static void b6b_socket_close(void *priv)
{
	const struct b6b_socket *s = (const struct b6b_socket *)priv;

	close(s->fd);
	free(priv);
}

static const struct b6b_strm_ops b6b_stream_socket_ops = {
	.read = b6b_socket_read,
	.write = b6b_socket_write,
	.peer = b6b_socket_peer,
	.close = b6b_socket_close
};

static const struct b6b_strm_ops b6b_dgram_socket_ops = {
	.read = b6b_socket_readfrom,
	.write = b6b_socket_write,
	.peer = b6b_socket_peer,
	.close = b6b_socket_close
};

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

static struct b6b_obj *b6b_client_socket_new(struct b6b_interp *interp,
                                             const char *host,
                                             const char *service,
                                             const int socktype,
                                             const struct b6b_strm_ops *ops,
                                             const char *type)
{
	struct addrinfo hints = {0}, *res;
	struct b6b_obj *o;
	const char *s;
	int fd, err;

	hints.ai_socktype = socktype;
	hints.ai_family = AF_UNSPEC;
	err = getaddrinfo(host, service, &hints, &res);
	if (err != 0) {
		s = gai_strerror(err);
		if (s)
			b6b_return_str(interp, s, strlen(s));
		return NULL;
	}

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

static enum b6b_res b6b_socket_client_proc(struct b6b_interp *interp,
                                           struct b6b_obj *args,
                                           const int socktype,
                                           const struct b6b_strm_ops *ops)
{
	struct b6b_obj *p, *h, *s, *o;

	if (!b6b_proc_get_args(interp, args, "o s s", &p, &h, &s))
		return B6B_ERR;

	o = b6b_client_socket_new(interp, h->s, s->s, socktype, ops, p->s);
	if (!o)
		return B6B_ERR;

	return b6b_return(interp, o);

}

static enum b6b_res b6b_socket_proc_stream_client(struct b6b_interp *interp,
                                                  struct b6b_obj *args)
{
	return b6b_socket_client_proc(interp,
	                              args,
	                              SOCK_STREAM,
	                              &b6b_stream_socket_ops);
}

static enum b6b_res b6b_socket_proc_dgram_client(struct b6b_interp *interp,
                                                 struct b6b_obj *args)
{
	return b6b_socket_client_proc(interp,
	                              args,
	                              SOCK_DGRAM,
	                              &b6b_dgram_socket_ops);
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
};
__b6b_ext(b6b_socket);
