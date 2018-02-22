/*
 * This file is part of b6b.
 *
 * Copyright 2017, 2018 Dima Krasner
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

#include <limits.h>
#include <stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include <b6b.h>

#include "miniz/miniz.h"
#include "miniz/miniz_tdef.h"

#define B6B_DEFLATE_CHUNK_SZ 64 * 1024
#define B6B_INFLATE_IN_CHUNK_SZ 64 * 1024
#define B6B_INFLATE_OUT_CHUNK_SZ 256 * 1024

struct b6b_zlib_deflate_data {
	mz_stream strm;
	mz_uint rem;
	size_t slen;
	int ok;
};

static void b6b_zlib_do_deflate(void *arg)
{
	struct b6b_zlib_deflate_data *data = (struct b6b_zlib_deflate_data *)arg;

	while (data->rem) {
		if (data->rem > B6B_DEFLATE_CHUNK_SZ)
			data->strm.avail_in = B6B_DEFLATE_CHUNK_SZ;
		else
			data->strm.avail_in = data->rem;

		if (mz_deflate(&data->strm, 0) != MZ_OK)
			return;

		data->rem = (mz_uint)data->slen - data->strm.total_in;
	}

	data->ok = 1;
}

static enum b6b_res b6b_zlib_deflate(struct b6b_interp *interp,
                                     struct b6b_obj *args,
                                     const int wbits,
                                     const size_t head,
                                     const size_t foot,
                                     void (*finish)(const unsigned char *,
                                                    const size_t,
                                                    const unsigned char *,
                                                    const size_t,
                                                    const int))
{
	struct b6b_zlib_deflate_data data = {.strm = {0}};
	size_t dlen, max, tot;
	struct b6b_obj *s, *l, *o;
	unsigned char *src, *dst;
	int level = MZ_DEFAULT_COMPRESSION;

	switch (b6b_proc_get_args(interp, args, "os|i", NULL, &s, &l)) {
		case 3:
			if ((l->i < INT_MIN) || (l->i > INT_MAX))
				return B6B_ERR;
			level = (int)l->i;

		case 2:
			break;

		default:
			return B6B_ERR;
	}

	max = UINT32_MAX - head - foot;
	if (s->slen > max)
		return B6B_ERR;

	if (mz_deflateInit2(&data.strm,
	                    level,
	                    MZ_DEFLATED,
	                    wbits,
	                    9,
	                    MZ_DEFAULT_STRATEGY) != MZ_OK)
		return B6B_ERR;

	data.slen = s->slen;
	dlen = (size_t)mz_deflateBound(&data.strm, (mz_ulong)data.slen);
	if (dlen >= max) {
		mz_deflateEnd(&data.strm);
		return B6B_ERR;
	}

	/* copy the buffer, since s->s may be freed after context switch */
	src = (unsigned char *)malloc(data.slen);
	if (!b6b_allocated(src)) {
		mz_deflateEnd(&data.strm);
		return B6B_ERR;
	}

	dst = (unsigned char *)malloc(dlen + head + foot + 1);
	if (!b6b_allocated(dst)) {
		free(src);
		mz_deflateEnd(&data.strm);
		return B6B_ERR;
	}

	memcpy(src, s->s, data.slen);

	data.strm.next_in = src;
	/* compressed data should start after the header and mz_deflate() does not
	 * know about the extra room for the footer */
	data.strm.next_out = dst + head;
	data.strm.avail_out = (mz_uint32)dlen;

	data.rem = (mz_uint)data.slen;
	data.ok = 0;
	if (!b6b_offload(interp, b6b_zlib_do_deflate, &data) ||
	    !data.ok ||
	    (mz_deflate(&data.strm, MZ_FINISH) != MZ_STREAM_END) ||
	    (data.strm.total_out > max)) {
		free(dst);
		free(src);
		mz_deflateEnd(&data.strm);
		return B6B_ERR;
	}

	if (finish)
		finish(src, data.slen, dst, (size_t)data.strm.total_out, level);

	free(src);
	tot = (size_t)(data.strm.total_out + head + foot);
	dst[tot] = '\0';
	o = b6b_str_new((char *)dst, tot);
	mz_deflateEnd(&data.strm);
	if (b6b_unlikely(!o)) {
		free(dst);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static enum b6b_res b6b_zlib_proc_deflate(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	return b6b_zlib_deflate(interp, args, -MZ_DEFAULT_WINDOW_BITS, 0, 0, NULL);
}

static enum b6b_res b6b_zlib_proc_compress(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	return b6b_zlib_deflate(interp, args, MZ_DEFAULT_WINDOW_BITS, 0, 0, NULL);
}

struct gzip_hdr {
	uint16_t sig;
	uint8_t meth;
	uint8_t fl;
	uint32_t tm;
	uint8_t cfl;
	uint8_t os;
} __attribute__((packed));

struct gzip_ftr {
	uint32_t crc;
	uint32_t len;
} __attribute__((packed));

static void b6b_zlib_gzip_finish(const unsigned char *buf,
                                 const size_t len,
                                 const unsigned char *cbuf,
                                 const size_t clen,
                                 const int level)
{
	struct gzip_hdr *hdr = (struct gzip_hdr *)cbuf;
	struct gzip_ftr *ftr = (struct gzip_ftr *)(cbuf + sizeof(*hdr) + clen);

	hdr->sig = htons(0x1f8b);
	hdr->meth = 8;
	hdr->fl = 0;
	/* we do not disclose local time - for example, in the case of a
	 * timing-based vulnerability we don't want to let an attacker know the
	 * local time through HTTP gzip compression */
	hdr->tm = 0;
	switch (level) {
		case MZ_BEST_COMPRESSION:
			hdr->cfl = 2;
			break;

		case MZ_BEST_SPEED:
			hdr->cfl = 4;
			break;

		default:
			hdr->cfl = 0;
	}
	hdr->os = 3;

	ftr->crc = mz_crc32(mz_crc32(0, NULL, 0), buf, len);
	ftr->len = (uint32_t)(len % UINT32_MAX);
}

static enum b6b_res b6b_zlib_proc_gzip(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	return b6b_zlib_deflate(interp,
	                        args,
	                        -MZ_DEFAULT_WINDOW_BITS,
	                        sizeof(struct gzip_hdr),
	                        sizeof(struct gzip_ftr),
	                        b6b_zlib_gzip_finish);
}

struct b6b_zlib_inflate_data {
	mz_stream strm;
	size_t slen;
	size_t dlen;
	unsigned char *dst;
	mz_uint rem;
	int ok;
};

static void b6b_zlib_do_inflate(void *arg)
{
	struct b6b_zlib_inflate_data *data = (struct b6b_zlib_inflate_data *)arg;
	unsigned char *mdst;

	do {
		switch (mz_inflate(&data->strm, 0))  {
			case MZ_OK:
				break;

			case MZ_STREAM_END:
				data->ok = 1;
				return;

			default:
				return;
		}

		if (data->dlen >= SIZE_MAX - B6B_INFLATE_OUT_CHUNK_SZ - 1)
			break;

		data->dlen += B6B_INFLATE_OUT_CHUNK_SZ;

		mdst = (unsigned char *)realloc(data->dst, data->dlen + 1);
		if (!b6b_allocated(mdst))
			return;

		data->strm.next_out = mdst + (data->strm.next_out - data->dst);
		data->dst = mdst;
		data->strm.avail_out += data->dlen - data->strm.total_out;

		data->rem = (mz_uint)data->slen - data->strm.total_in;

		if (data->rem > B6B_INFLATE_IN_CHUNK_SZ)
			data->strm.avail_in = B6B_INFLATE_IN_CHUNK_SZ;
		else
			data->strm.avail_in = data->rem;
	} while (1);
}

static enum b6b_res b6b_zlib_inflate(struct b6b_interp *interp,
                                     struct b6b_obj *args,
                                     const int wbits,
                                     const size_t head,
                                     const size_t foot,
                                     size_t (*get_size)(const unsigned char *,
                                                        const size_t))
{
	struct b6b_zlib_inflate_data data = {.strm = {0}};
	struct b6b_obj *s, *o;
	size_t junk;
	unsigned char *src;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s))
		return B6B_ERR;

	junk = head + foot;
	if ((s->slen > UINT32_MAX) || (s->slen <= junk))
		return B6B_ERR;

	if (get_size) {
		data.dlen = get_size((const unsigned char *)s->s, s->slen);
		if (data.dlen == 0) {
			data.dlen = 1;
		} else if (data.dlen == SIZE_MAX)
			return B6B_ERR;
	} else
		data.dlen = B6B_INFLATE_OUT_CHUNK_SZ;

	if (mz_inflateInit2(&data.strm, wbits) != MZ_OK)
		return B6B_ERR;

	data.dst = (unsigned char *)malloc(data.dlen + 1);
	if (!data.dst) {
		mz_inflateEnd(&data.strm);
		return B6B_ERR;
	}

	data.slen = s->slen - junk;
	src = (unsigned char *)malloc(data.slen);
	if (!b6b_allocated(src)) {
		free(data.dst);
		mz_inflateEnd(&data.strm);
		return B6B_ERR;
	}
	memcpy(src, s->s + head, data.slen);

	data.strm.next_in = src;
	data.strm.next_out = data.dst;
	data.strm.avail_in = data.slen;
	data.strm.avail_out = data.dlen;

	data.rem = (mz_uint)data.slen;
	data.ok = 0;
	if (!b6b_offload(interp, b6b_zlib_do_inflate, &data) ||
	    !data.ok ||
	    (mz_inflate(&data.strm, MZ_FINISH) != MZ_STREAM_END)) {
		free(data.dst);
		free(src);
		mz_inflateEnd(&data.strm);
		return B6B_ERR;
	}

	free(src);
	data.dst[data.strm.total_out] = '\0';
	o = b6b_str_new((char *)data.dst, (size_t)data.strm.total_out);
	mz_inflateEnd(&data.strm);
	if (b6b_unlikely(!o)) {
		free(data.dst);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static enum b6b_res b6b_zlib_proc_inflate(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	return b6b_zlib_inflate(interp, args, -MZ_DEFAULT_WINDOW_BITS, 0, 0, NULL);
}

static enum b6b_res b6b_zlib_proc_decompress(struct b6b_interp *interp,
                                             struct b6b_obj *args)
{
	return b6b_zlib_inflate(interp, args, MZ_DEFAULT_WINDOW_BITS, 0, 0, NULL);
}

static size_t b6b_zlib_gzip_get_size(const unsigned char *buf,
                                     const size_t len)
{
	struct gzip_ftr *ftr = (struct gzip_ftr *)(buf + len - sizeof(*ftr));

	return (size_t)ftr->len;
}

static enum b6b_res b6b_zlib_proc_gunzip(struct b6b_interp *interp,
                                             struct b6b_obj *args)
{
	return b6b_zlib_inflate(interp,
	                        args,
	                        -MZ_DEFAULT_WINDOW_BITS,
	                        sizeof(struct gzip_hdr),
	                        sizeof(struct gzip_ftr),
	                        b6b_zlib_gzip_get_size);
}

static const struct b6b_ext_obj b6b_zlib[] = {
	{
		.name = "deflate",
		.type = B6B_TYPE_STR,
		.val.s = "deflate",
		.proc = b6b_zlib_proc_deflate
	},
	{
		.name = "compress",
		.type = B6B_TYPE_STR,
		.val.s = "compress",
		.proc = b6b_zlib_proc_compress
	},
	{
		.name = "gzip",
		.type = B6B_TYPE_STR,
		.val.s = "gzip",
		.proc = b6b_zlib_proc_gzip
	},
	{
		.name = "inflate",
		.type = B6B_TYPE_STR,
		.val.s = "inflate",
		.proc = b6b_zlib_proc_inflate
	},
	{
		.name = "decompress",
		.type = B6B_TYPE_STR,
		.val.s = "decompress",
		.proc = b6b_zlib_proc_decompress
	},
	{
		.name = "gunzip",
		.type = B6B_TYPE_STR,
		.val.s = "gunzip",
		.proc = b6b_zlib_proc_gunzip
	}
};
__b6b_ext(b6b_zlib);
