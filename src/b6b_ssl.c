/*
 * This file is part of b6b.
 *
 * Copyright 2022 Dima Krasner
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
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <poll.h>

#include <mbedtls/net_sockets.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>

#include <b6b.h>

struct b6b_ssl_socket {
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cert;
	struct b6b_obj *fdo;
	int fd;
};

struct b6b_ssl_server {
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cert;
	mbedtls_pk_context pk;
};

struct b6b_ssl_handshake_data {
	struct pollfd pfd;
	int ret;
};

static ssize_t b6b_ssl_peeksz(struct b6b_interp *interp, void *priv)
{
	const struct b6b_ssl_socket *ssl = (const struct b6b_ssl_socket *)priv;

	return b6b_fd_peeksz(interp, (void *)(intptr_t)ssl->fd);
}

static ssize_t b6b_ssl_read(struct b6b_interp *interp,
                            void *priv,
                            unsigned char *buf,
                            const size_t len,
                            int *eof,
                            int *again)
{
	struct b6b_ssl_socket *ssl = (struct b6b_ssl_socket *)priv;
	int out;

	out = mbedtls_ssl_read(&ssl->ssl, buf, len);
	if (out < 0) {
		if ((out == MBEDTLS_ERR_SSL_WANT_READ) ||
		    (out == MBEDTLS_ERR_SSL_WANT_WRITE))
			return 0;

		out = -1;
	} else if (out == 0)
		*eof = 0;

	return (ssize_t)out;
}

static ssize_t b6b_ssl_write(struct b6b_interp *interp,
                             void *priv,
                             const unsigned char *buf,
                             const size_t len)
{
	struct b6b_ssl_socket *ssl = (struct b6b_ssl_socket *)priv;
	int out;

	out = mbedtls_ssl_write(&ssl->ssl, buf, len);
	if (out < 0) {
		if ((out == MBEDTLS_ERR_SSL_WANT_READ) ||
		    (out == MBEDTLS_ERR_SSL_WANT_WRITE))
			return 0;

		out = -1;
	}

	return (ssize_t)out;
}

static void b6b_ssl_close(void *priv)
{
	struct b6b_ssl_socket *ssl = (struct b6b_ssl_socket *)priv;

	mbedtls_x509_crt_free(&ssl->cert);
	mbedtls_ssl_free(&ssl->ssl);
	mbedtls_ssl_config_free(&ssl->conf);
	mbedtls_ctr_drbg_free(&ssl->ctr_drbg);
	mbedtls_entropy_free(&ssl->entropy);

	b6b_unref(ssl->fdo);
	free(priv);
}

static struct b6b_obj *b6b_ssl_peer(struct b6b_interp *interp, void *priv)
{
	return b6b_ref(interp->null);
}

static int b6b_ssl_fd(void *priv)
{
	const struct b6b_ssl_socket *ssl = (const struct b6b_ssl_socket *)priv;

	return ssl->fd;
}

static const struct b6b_strm_ops b6b_ssl_client_ops = {
	.peeksz = b6b_ssl_peeksz,
	.read = b6b_ssl_read,
	.write = b6b_ssl_write,
	.peer = b6b_ssl_peer,
	.fd = b6b_ssl_fd,
	.close = b6b_ssl_close
};

extern const unsigned char *b6b_ca_certs;
extern const size_t b6b_ca_certs_len;

static int b6b_ssl_do_recv(void *ctx, unsigned char *buf, size_t len)
{
	int fd = (int)(intptr_t)ctx;
	ssize_t out;

	out = recv(fd, buf, len % INT_MAX, 0);
	if (out < 0) {
		if (errno == EAGAIN)
			return MBEDTLS_ERR_SSL_WANT_READ;

		return MBEDTLS_ERR_NET_RECV_FAILED;
	}

	return (int)out;
}

static int b6b_ssl_do_send(void *ctx, const unsigned char *buf, size_t len)
{
	int fd = (int)(intptr_t)ctx;
	ssize_t out;

	out = send(fd, buf, len % INT_MAX, MSG_NOSIGNAL);
	if (out < 0) {
		if (errno == EAGAIN)
			return MBEDTLS_ERR_SSL_WANT_WRITE;

		return MBEDTLS_ERR_NET_SEND_FAILED;
	}

	return (int)out;
}

static void b6b_ssl_handshake_poll(void *arg)
{
	struct b6b_ssl_handshake_data *data = (struct b6b_ssl_handshake_data *)arg;

	data->ret = poll(&data->pfd, 1, -1);
}

static struct b6b_obj *b6b_ssl_handshake(struct b6b_interp *interp,
                                         struct b6b_ssl_socket *ssl,
                                         const struct mbedtls_ssl_config *conf,
                                         struct b6b_obj *fd)
{
	struct b6b_ssl_handshake_data data;
	int err;

	if (mbedtls_ssl_setup(&ssl->ssl, conf) != 0)
		return NULL;

	mbedtls_ssl_set_bio(&ssl->ssl,
	                    (void *)(intptr_t)fd->i,
	                    b6b_ssl_do_send,
	                    b6b_ssl_do_recv,
	                    NULL);

	data.pfd.fd = fd->i;

	do {
		err = mbedtls_ssl_handshake(&ssl->ssl);
		if (err == 0)
			break;

		data.pfd.events = POLLIN | POLLRDHUP;
		data.pfd.revents = 0;

		if (err == MBEDTLS_ERR_SSL_WANT_WRITE)
			data.pfd.events |= POLLOUT;
		else if (err != MBEDTLS_ERR_SSL_WANT_READ)
			return NULL;

		b6b_offload(interp, b6b_ssl_handshake_poll, &data);
		if ((data.ret <= 0) || (data.pfd.revents & ~(POLLIN | POLLOUT)))
			return NULL;
	} while (1);

	return b6b_strm_fmt(interp, &b6b_ssl_client_ops, ssl, "ssl");
}

static enum b6b_res b6b_ssl_proc_client(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *fd, *hostname, *verifyo, *o;
	struct b6b_ssl_socket *ssl;
	int verify = 1;

	switch (b6b_proc_get_args(interp,
	                          args,
	                          "ois|o",
	                          NULL,
	                          &fd,
	                          &hostname,
	                          &verifyo)) {
	case 4:
		verify = b6b_obj_istrue(verifyo);
		break;

	case 3:
		break;

	default:
		return B6B_ERR;
	}

	ssl = (struct b6b_ssl_socket *)malloc(sizeof(*ssl));
	if (!b6b_allocated(ssl))
		return B6B_ERR;

	ssl->fdo = b6b_ref(fd);

	mbedtls_ssl_init(&ssl->ssl);
	mbedtls_ssl_config_init(&ssl->conf);
	mbedtls_x509_crt_init(&ssl->cert);
	mbedtls_ctr_drbg_init(&ssl->ctr_drbg);
	mbedtls_entropy_init(&ssl->entropy);

	if (mbedtls_ctr_drbg_seed(&ssl->ctr_drbg,
	                          mbedtls_entropy_func,
	                          &ssl->entropy,
	                          NULL,
	                          0) != 0) {
		b6b_ssl_close(ssl);
		return B6B_ERR;
	}

	if (verify &&
	    (mbedtls_x509_crt_parse(&ssl->cert,
	                            b6b_ca_certs,
	                            b6b_ca_certs_len) != 0)) {
		b6b_ssl_close(ssl);
		return B6B_ERR;
	}

	if (mbedtls_ssl_config_defaults(&ssl->conf,
	                                MBEDTLS_SSL_IS_CLIENT,
	                                MBEDTLS_SSL_TRANSPORT_STREAM,
	                                MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
		b6b_ssl_close(ssl);
		return B6B_ERR;
	}

	if (verify)
		mbedtls_ssl_conf_ca_chain(&ssl->conf, &ssl->cert, NULL);
	else
		mbedtls_ssl_conf_authmode(&ssl->conf, MBEDTLS_SSL_VERIFY_NONE);

	mbedtls_ssl_conf_rng(&ssl->conf, mbedtls_ctr_drbg_random, &ssl->ctr_drbg);

	if (mbedtls_ssl_set_hostname(&ssl->ssl, hostname->s) != 0) {
		b6b_ssl_close(ssl);
		return B6B_ERR;
	}

	o = b6b_ssl_handshake(interp, ssl, &ssl->conf, fd);
	if (!o) {
		b6b_ssl_close(ssl);
		return B6B_ERR;
	}

	ssl->fd = fd->i;

	return b6b_return(interp, o);
}

static enum b6b_res b6b_ssl_server_accept(struct b6b_interp *interp,
                                          struct b6b_ssl_server *server,
                                          struct b6b_obj *fd)
{
	struct b6b_obj *o;
	struct b6b_ssl_socket *ssl;

	ssl = (struct b6b_ssl_socket *)malloc(sizeof(*ssl));
	if (!b6b_allocated(ssl))
		return B6B_ERR;

	ssl->fdo = b6b_ref(fd);

	mbedtls_ssl_init(&ssl->ssl);
	mbedtls_ssl_config_init(&ssl->conf);
	mbedtls_x509_crt_init(&ssl->cert);
	mbedtls_ctr_drbg_init(&ssl->ctr_drbg);
	mbedtls_entropy_init(&ssl->entropy);

	o = b6b_ssl_handshake(interp, ssl, &server->conf, fd);
	if (!o) {
		b6b_ssl_close(ssl);
		return B6B_ERR;
	}

	ssl->fd = fd->i;

	return b6b_return(interp, o);
}

static enum b6b_res b6b_ssl_server_proc(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *o, *op, *fd;
	unsigned int argc;

	argc = b6b_proc_get_args(interp, args, "os|i", &o, &op, &fd);
	if (argc < 2)
		return B6B_ERR;

	if ((argc == 3) && (strcmp(op->s, "accept") == 0))
		return b6b_ssl_server_accept(interp,
		                             (struct b6b_ssl_server *)o->priv,
		                             fd);

	b6b_return_fmt(interp, "bad ssl.server op: %s", op->s);
	return B6B_ERR;
}

static void b6b_ssl_server_close(struct b6b_ssl_server *server)
{
	mbedtls_x509_crt_free(&server->cert);
	mbedtls_ssl_config_free(&server->conf);
	mbedtls_ctr_drbg_free(&server->ctr_drbg);
	mbedtls_entropy_free(&server->entropy);
	mbedtls_pk_free(&server->pk);

	free(server);
}

static void b6b_ssl_server_del(void *priv)
{
	b6b_ssl_server_close((struct b6b_ssl_server *)priv);
}

static enum b6b_res b6b_ssl_proc_server(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *cert, *pk, *o;
	struct b6b_ssl_server *server;

	if (!b6b_proc_get_args(interp, args, "oss", NULL, &cert, &pk))
		return B6B_ERR;

	server = (struct b6b_ssl_server *)malloc(sizeof(*server));
	if (!b6b_allocated(server))
		return B6B_ERR;

	mbedtls_ssl_config_init(&server->conf);
	mbedtls_x509_crt_init(&server->cert);
	mbedtls_ctr_drbg_init(&server->ctr_drbg);
	mbedtls_pk_init(&server->pk);

	mbedtls_entropy_init(&server->entropy);
	if (mbedtls_ctr_drbg_seed(&server->ctr_drbg,
	                          mbedtls_entropy_func,
	                          &server->entropy,
	                          NULL,
	                          0) != 0) {
		b6b_ssl_server_close(server);
		return B6B_ERR;
	}

	if ((mbedtls_x509_crt_parse_file(&server->cert, cert->s) != 0) ||
	    (mbedtls_pk_parse_keyfile(&server->pk,
	                              pk->s,
	                              "",
	                              mbedtls_ctr_drbg_random,
	                              &server->ctr_drbg) != 0)) {
		b6b_ssl_server_close(server);
		return B6B_ERR;
	}

	if (mbedtls_ssl_config_defaults(&server->conf,
	                                MBEDTLS_SSL_IS_SERVER,
	                                MBEDTLS_SSL_TRANSPORT_STREAM,
	                                MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
		b6b_ssl_server_close(server);
		return B6B_ERR;
	}

	mbedtls_ssl_conf_rng(&server->conf,
	                     mbedtls_ctr_drbg_random,
	                     &server->ctr_drbg);

	if (mbedtls_ssl_conf_own_cert(&server->conf,
	                              &server->cert,
	                              &server->pk) != 0) {
		b6b_ssl_server_close(server);
		return B6B_ERR;
	}

	o = b6b_str_fmt("ssl.server:%"PRIxPTR, (uintptr_t)server);
	if (!b6b_allocated(o)) {
		b6b_ssl_server_close(server);
		return B6B_ERR;
	}

	o->priv = server;
	o->proc = b6b_ssl_server_proc;
	o->del = b6b_ssl_server_del;

	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_ssl[] = {
	{
		.name = "ssl.client",
		.type = B6B_TYPE_STR,
		.val.s = "ssl.client",
		.proc = b6b_ssl_proc_client
	},
	{
		.name = "ssl.server",
		.type = B6B_TYPE_STR,
		.val.s = "ssl.server",
		.proc = b6b_ssl_proc_server
	}
};
__b6b_ext(b6b_ssl);
