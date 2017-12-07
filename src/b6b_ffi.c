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
#include <limits.h>
#include <string.h>
#include <dlfcn.h>

#include <ffi.h>

#include <b6b.h>

#define B6B_FFI_MAX_STRUCT_SZ 4096

union b6b_ffi_val {
	char schar;
	unsigned char uchar;
	short sshort;
	unsigned short ushort;
	int sint;
	unsigned int uint;
	long slong;
	unsigned long ulong;
	int64_t int64;
	uint64_t uint64;
	float f;
	double d;
	void *voidp;
};

struct b6b_ffi_func {
	ffi_cif cif;
	ffi_type *rtype;
	ffi_type **atypes;
	const void *p;
	unsigned int argc;
};

static const struct {
	const char c;
	ffi_type *type;
} b6b_ffi_types[] = {
	{'i', &ffi_type_sint},
	{'I', &ffi_type_uint},
	{'l', &ffi_type_slong},
	{'L', &ffi_type_ulong},
	{'p', &ffi_type_pointer},
	{'b', &ffi_type_schar},
	{'B', &ffi_type_uchar},
	{'h', &ffi_type_sshort},
	{'H', &ffi_type_ushort},
	{'q', &ffi_type_sint64},
	{'Q', &ffi_type_uint64},
	{'f', &ffi_type_float},
	{'d', &ffi_type_double}
};

/* creates an integer object that holds an address and holds a reference to the
 * object that address belongs to, to prevent it from being freed while its
 * address is passed around - this makes it possible to pass anonymous function
 * parameters */
static enum b6b_res b6b_ffi_addr_new(struct b6b_interp *interp,
                                     void *addr,
                                     struct b6b_obj *to)
{
	struct b6b_obj *o;

	o = b6b_int_new((b6b_int)(intptr_t)addr);
	if (b6b_unlikely(!o))
		return B6B_ERR;

	o->priv = b6b_ref(to);
	o->del = (b6b_delf)b6b_unref;
	return b6b_return(interp, o);
}

static enum b6b_res b6b_ffi_buf_proc(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *o, *op;

	if (!b6b_proc_get_args(interp, args, "os", &o, &op))
		return B6B_ERR;

	if (strcmp(op->s, "address") == 0)
		return b6b_ffi_addr_new(interp, o->s, o);

	return B6B_ERR;
}

static int b6b_ffi_buf_new(struct b6b_interp *interp, char *s, const size_t len)
{
	struct b6b_obj *o;

	o = b6b_str_new(s, len);
	if (b6b_unlikely(!o))
		return 0;

	o->proc = b6b_ffi_buf_proc;

	b6b_return(interp, o);
	return 1;
}

static enum b6b_res b6b_ffi_proc_buf(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *s;
	char *s2;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s))
		return B6B_ERR;

	s2 = b6b_strndup(s->s, s->slen);
	if (b6b_unlikely(!s2))
		return B6B_ERR;

	if (b6b_unlikely(!b6b_ffi_buf_new(interp, s2, s->slen))) {
		free(s2);
		return B6B_ERR;
	}

	return B6B_OK;
}

static ffi_type *b6b_ffi_type(struct b6b_interp *interp, const char name)
{
	size_t i;

	for (i = 0; i < sizeof(b6b_ffi_types) / sizeof(b6b_ffi_types[0]); ++i) {
		if (b6b_ffi_types[i].c == name)
			return b6b_ffi_types[i].type;
	}

	return NULL;
}

static struct b6b_obj *b6b_ffi_decode(const void *val,
                                      const size_t len,
                                      ffi_type *type)
{
	if (len < type->size)
		return NULL;

	if (type == &ffi_type_sint)
		return b6b_int_new((b6b_int)*(int *)val);
	else if (type == &ffi_type_uint)
		return b6b_int_new((b6b_int)*(unsigned int *)val);
	else if (type == &ffi_type_slong)
		return b6b_int_new((b6b_int)*(long *)val);
	else if (type == &ffi_type_ulong)
		return b6b_int_new((b6b_int)*(unsigned long *)val);
	else if (type == &ffi_type_pointer)
		return b6b_int_new((b6b_int)(intptr_t)*(void **)val);
	else if (type == &ffi_type_schar)
		return b6b_int_new((b6b_int)*(char *)val);
	else if (type == &ffi_type_uchar)
		return b6b_int_new((b6b_int)*(unsigned char *)val);
	else if (type == &ffi_type_sshort)
		return b6b_int_new((b6b_int)*(short *)val);
	else if (type == &ffi_type_ushort)
		return b6b_int_new((b6b_int)*(unsigned short *)val);
	else if (type == &ffi_type_sint64)
		return b6b_int_new((b6b_int)*(int64_t *)val);
	else if (type == &ffi_type_uint64)
		return b6b_int_new((b6b_int)*(uint64_t *)val);
	else if (type == &ffi_type_float)
		return b6b_float_new((b6b_float)*(float *)val);
	else if (type == &ffi_type_double)
		return b6b_float_new((b6b_float)*(double *)val);
	else
		return NULL;
}

static enum b6b_res b6b_ffi_proc_memcpy(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *addr, *len;
	char *s;
	const char *p;

	if (!b6b_proc_get_args(interp, args, "oii", NULL, &addr, &len) || !len->i)
		return B6B_ERR;

	p = (char *)(intptr_t)addr->i;
	if (!p)
		return B6B_ERR;

	s = b6b_strndup(p, (size_t)len->i);
	if (b6b_unlikely(!s))
		return B6B_ERR;

	if (b6b_unlikely(!b6b_ffi_buf_new(interp, s, (size_t)len->i))) {
		free(s);
		return B6B_ERR;
	}

	return B6B_OK;
}

static void b6b_ffi_func_del(void *priv)
{
	struct b6b_ffi_func *f = (struct b6b_ffi_func *)priv;

	free(f->atypes);
	free(f);
}

static enum b6b_res b6b_ffi_func_proc(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *o, *argpos, *outo;
	struct b6b_ffi_func *f;
	const struct b6b_litem *li;
	void **argps;
	union b6b_ffi_val out;
	unsigned int i;

	if (!b6b_proc_get_args(interp, args, "ol", &o, &argpos))
		return B6B_ERR;

	f = (struct b6b_ffi_func *)o->priv;

	argps = (void **)malloc(sizeof(void *) * f->argc);
	if (!b6b_allocated(argps))
		return B6B_ERR;

	li = b6b_list_first(argpos);
	for (i = 0; i < f->argc; ++i, li = b6b_list_next(li)) {
		if (!li) {
			free(argps);
			return B6B_ERR;
		}

		if (!b6b_as_int(li->o)) {
			free(argps);
			return B6B_ERR;
		}

		argps[i] = (void *)(intptr_t)li->o->i;
	}

	ffi_call(&f->cif, f->p, &out, argps);

	free(argps);
	outo = b6b_ffi_decode(&out, sizeof(out), f->rtype);
	if (!outo)
		return B6B_ERR;

	return b6b_return(interp, outo);
}

static int b6b_ffi_get_types(struct b6b_interp *interp,
                             struct b6b_obj *names,
                             ffi_type ***types)
{
	size_t i;

	*types = (ffi_type **)malloc(sizeof(ffi_type *) * names->slen);
	if (!b6b_allocated(*types))
		return 0;

	for (i = 0; i < names->slen; ++i) {
		(*types)[i] = b6b_ffi_type(interp, names->s[i]);
		if (!(*types)[i]) {
			free(*types);
			return 0;
		}
	}

	return 1;
}

static enum b6b_res b6b_ffi_proc_func(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *addr, *rtype, *atypes;
	struct b6b_ffi_func *f;
	struct b6b_obj *o;

	if (!b6b_proc_get_args(interp,
	                       args,
	                       "oiss",
	                       NULL,
	                       &addr,
	                       &rtype,
	                       &atypes) ||
	    (rtype->slen != 1) ||
	    (atypes->slen > UINT_MAX))
		return B6B_ERR;

	f = (struct b6b_ffi_func *)malloc(sizeof(*f));
	if (!b6b_allocated(f))
		return B6B_ERR;

	f->rtype = b6b_ffi_type(interp, rtype->s[0]);
	if (!f->rtype) {
		free(f);
		return B6B_ERR;
	}

	if (!b6b_ffi_get_types(interp, atypes, &f->atypes)) {
		free(f);
		return B6B_ERR;
	}
	f->argc = (unsigned int)atypes->slen;

	if (ffi_prep_cif(&f->cif,
	                 FFI_DEFAULT_ABI,
	                 f->argc,
	                 f->rtype,
	                 f->atypes) != FFI_OK) {
		free(f->atypes);
		free(f);
		return B6B_ERR;
	}

	o = b6b_str_fmt("func:%"PRIxPTR, (uintptr_t)f);
	if (b6b_unlikely(!o)) {
		free(f->atypes);
		free(f);
		return B6B_ERR;
	}

	f->p = (void *)(intptr_t)addr->i;

	o->priv = f;
	o->proc = b6b_ffi_func_proc;
	o->del = b6b_ffi_func_del;
	return b6b_return(interp, o);
}


static int b6b_ffi_encode(struct b6b_interp *interp,
                          struct b6b_obj *o,
                          ffi_type *type,
                          union b6b_ffi_val *val,
                          void **p)
{
	if (type == &ffi_type_sint) {
		if (!b6b_as_int(o) || (o->i < INT_MIN) || (o->i > INT_MAX))
			return 0;

		val->sint = (int)o->i;
		if (p)
			*p = &val->sint;
	}
	else if (type == &ffi_type_uint) {
		if (!b6b_as_int(o) || (o->i < 0) || (o->i > UINT_MAX))
			return 0;

		val->uint = (unsigned int)o->i;
		if (p)
			*p = &val->uint;
	}
	else if (type == &ffi_type_slong) {
		if (!b6b_as_int(o) || (o->i < LONG_MIN) || (o->i > LONG_MAX))
			return 0;

		val->slong = (long)o->i;
		if (p)
			*p = &val->slong;
	}
	else if (type == &ffi_type_ulong) {
		if (!b6b_as_int(o) || (o->i < 0) || (o->i > ULONG_MAX))
			return 0;

		val->ulong = (unsigned long)o->i;
		if (p)
			*p = &val->ulong;
	}
	else if (type == &ffi_type_pointer) {
		if (!b6b_as_int(o))
			return 0;

		val->voidp = (void *)(intptr_t)o->i;
		if (p)
			*p = &val->voidp;
	}
	else if (type == &ffi_type_schar) {
		if (!b6b_as_int(o) || (o->i < CHAR_MIN) || (o->i > CHAR_MAX))
			return 0;

		val->schar = (char)o->i;
		if (p)
			*p = &val->schar;
	}
	else if (type == &ffi_type_uchar) {
		if (!b6b_as_int(o) || (o->i < 0) || (o->i > UCHAR_MAX))
			return 0;

		val->uchar = (unsigned char)o->i;
		if (p)
			*p = &val->uchar;
	}
	else if (type == &ffi_type_sshort) {
		if (!b6b_as_int(o) || (o->i < SHRT_MIN) || (o->i > SHRT_MAX))
			return 0;

		val->sshort = (short)o->i;
		if (p)
			*p = &val->sshort;
	}
	else if (type == &ffi_type_ushort) {
		if (!b6b_as_int(o) || (o->i < 0) || (o->i > USHRT_MAX))
			return 0;

		val->ushort = (unsigned short)o->i;
		if (p)
			*p = &val->ushort;
	}
	else if (type == &ffi_type_sint64) {
		if (!b6b_as_int(o) || (o->i < INT64_MIN) || (o->i > INT64_MAX))
			return 0;

		val->int64 = (int64_t)o->i;
		if (p)
			*p = &val->int64;
	}
	else if (type == &ffi_type_uint64) {
		if (!b6b_as_int(o) || (o->i < 0) || (o->i > UINT64_MAX))
			return 0;

		val->uint64 = (uint64_t)o->i;
		if (p)
			*p = &val->uint64;
	}
	else if (type == &ffi_type_float) {
		if (!b6b_as_float(o))
			return 0;

		val->f = (float)o->f;
		if (p)
			*p = &val->f;
	}
	else if (type == &ffi_type_double) {
		if (!b6b_as_float(o))
			return 0;

		val->d = (double)o->f;
		if (p)
			*p = &val->d;
	}
	else
		return 0;

	return 1;
}

static enum b6b_res b6b_ffi_proc_pack(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	union b6b_ffi_val val;
	void *buf;
	struct b6b_obj *fmt;
	struct b6b_litem *vli;
	ffi_type *type;
	void *p;
	size_t i = 0, len = 0, pad, end;
	int pack = 0;

	if (!b6b_proc_get_args(interp, args, "os*", NULL, &fmt, &vli) || !fmt->slen)
		return B6B_ERR;

	if (fmt->s[i] == '.') {
		if (fmt->slen == 1)
			return B6B_ERR;

		pack = 1;
		++i;
	}

	buf = malloc(B6B_FFI_MAX_STRUCT_SZ);
	if (!b6b_allocated(buf))
		return B6B_ERR;

	for (; i < fmt->slen; ++i, vli = b6b_list_next(vli)) {
		if (!vli) {
			free(buf);
			return B6B_ERR;
		}

		type = b6b_ffi_type(interp, fmt->s[i]);
		if (!type) {
			free(buf);
			return B6B_ERR;
		}

		if (!b6b_ffi_encode(interp, vli->o, type, &val, &p)) {
			free(buf);
			return B6B_ERR;
		}

		if (!pack) {
			pad = len % type->alignment;
			end = len + pad;
			if (end > B6B_FFI_MAX_STRUCT_SZ) {
				free(buf);
				return B6B_ERR;
			}

			memset(buf + len, 0, pad);
			len = end;
		}

		end = len + type->size;
		if (end > B6B_FFI_MAX_STRUCT_SZ) {
			free(buf);
			return B6B_ERR;
		}

		memcpy(buf + len, p, type->size);
		len = end;
	}

	if (b6b_unlikely(!b6b_ffi_buf_new(interp, buf, len))) {
		free(buf);
		return B6B_ERR;
	}

	return B6B_OK;
}

static enum b6b_res b6b_ffi_proc_unpack(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *fmt, *o, *l, *dec;
	ffi_type *type;
	size_t i = 0, pad, off = 0;
	int packed = 0;

	if (!b6b_proc_get_args(interp, args, "oss", NULL, &fmt, &o) || !fmt->slen)
		return B6B_ERR;

	if (fmt->s[i] == '.') {
		if (fmt->slen == 1)
			return B6B_ERR;

		packed = 1;
		++i;
	}

	l = b6b_list_new();
	if (b6b_unlikely(!l))
		return B6B_ERR;

	for (; i < fmt->slen; ++i) {
		if (off >= o->slen) {
			b6b_destroy(l);
			return B6B_ERR;
		}

		type = b6b_ffi_type(interp, fmt->s[i]);
		if (!type) {
			b6b_destroy(l);
			return B6B_ERR;
		}

		if (!packed) {
			pad = off % type->alignment;
			if (pad) {
				off += pad;
				if (off >= o->slen) {
					b6b_destroy(l);
					return B6B_ERR;
				}
			}
		}

		dec = b6b_ffi_decode(&o->s[off], o->slen - off, type);
		if (!dec) {
			b6b_destroy(l);
			return B6B_ERR;
		}

		if (b6b_unlikely(!b6b_list_add(l, dec))) {
			b6b_destroy(dec);
			b6b_destroy(l);
			return B6B_ERR;
		}

		b6b_unref(dec);
		off += type->size;
	}

	return b6b_return(interp, l);
}

static enum b6b_res b6b_ffi_so_proc(struct b6b_interp *interp,
                                    struct b6b_obj *args)
{
	struct b6b_obj *o, *op, *sym;
	void *p;
	const char *err;

	switch (b6b_proc_get_args(interp, args, "os|s", &o, &op, &sym)) {
		case 3:
			if (strcmp(op->s, "dlsym") == 0) {
				dlerror();
				p = dlsym(o->priv, sym->s);
				err = dlerror();
				if (err) {
					b6b_return_str(interp, err, strlen(err));
					return B6B_ERR;
				}

				return b6b_ffi_addr_new(interp, p, o);
			}
			break;

		case 2:
			if (strcmp(op->s, "handle") == 0)
				return b6b_ffi_addr_new(interp, o->priv, o);
	}

	return B6B_ERR;
}

static void b6b_ffi_so_del(void *priv)
{
	dlclose(priv);
}

static enum b6b_res b6b_ffi_proc_dlopen(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *path, *o;
	void *p;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &path))
		return B6B_ERR;

	if (path->slen)
		p = dlopen(path->s, RTLD_LAZY);
	else
		p = dlopen(NULL, RTLD_LAZY);

	if (!p)
		return B6B_ERR;

	o = b6b_str_fmt("so:%"PRIxPTR, (uintptr_t)p);
	if (b6b_unlikely(!o)) {
		dlclose(p);
		return B6B_ERR;
	}

	o->priv = p;
	o->proc = b6b_ffi_so_proc;
	o->del = b6b_ffi_so_del;
	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_ffi[] = {
	{
		.name = "ffi.buf",
		.type = B6B_TYPE_STR,
		.val.s = "ffi.buf",
		.proc = b6b_ffi_proc_buf
	},
	{
		.name = "ffi.memcpy",
		.type = B6B_TYPE_STR,
		.val.s = "ffi.memcpy",
		.proc = b6b_ffi_proc_memcpy
	},
	{
		.name = "ffi.pack",
		.type = B6B_TYPE_STR,
		.val.s = "ffi.pack",
		.proc = b6b_ffi_proc_pack
	},
	{
		.name = "ffi.unpack",
		.type = B6B_TYPE_STR,
		.val.s = "ffi.unpack",
		.proc = b6b_ffi_proc_unpack
	},
	{
		.name = "ffi.dlopen",
		.type = B6B_TYPE_STR,
		.val.s = "ffi.dlopen",
		.proc = b6b_ffi_proc_dlopen
	},
	{
		.name = "ffi.func",
		.type = B6B_TYPE_STR,
		.val.s = "ffi.func",
		.proc = b6b_ffi_proc_func
	}
};
__b6b_ext(b6b_ffi);

#define B6B_FFI_BODY \
	"{$proc ffi.ptr {" \
		"{$ffi.pack p [$1 address]}" \
	"}}\n" \
	"{$proc ffi.call {" \
		"{$1 [$map x [$list.range $@ 2 [$- [$list.len $@] 1]] {{$x address}}]}" \
	"}}"

static int b6b_ffi_init(struct b6b_interp *interp)
{
	return b6b_call_copy(interp,
	                     B6B_FFI_BODY,
	                     sizeof(B6B_FFI_BODY) - 1) == B6B_OK;
}
__b6b_init(b6b_ffi_init);
