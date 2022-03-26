# This file is part of b6b.
#
# Copyright 2017, 2020, 2022 Dima Krasner
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_project_arguments(lolibc.get_variable('lolibc_c_args'), language: 'c')

cc = meson.get_compiler('c')

with_valgrind = get_option('with_valgrind')
if with_valgrind
	if not cc.has_header('valgrind/valgrind.h') or not cc.has_header('valgrind/memcheck.h') or not cc.has_header('valgrind/helgrind.h')
		error('Valgrind headers are missing')
	endif
	add_project_arguments('-DB6B_HAVE_VALGRIND', language: 'c')
endif

if cc.has_header('fenv.h')
	add_project_arguments('-DB6B_HAVE_FENV', language: 'c')
endif

libb6b_srcs = [
	'b6b_hash.c', 'b6b_obj.c', 'b6b_str.c', 'b6b_int.c', 'b6b_float.c', 'b6b_list.c', 'b6b_dict.c',
	'b6b_frame.c', 'b6b_thread.c', 'b6b_syscall.c', 'b6b_interp.c', 'b6b_core.c',
	'b6b_math.c', 'b6b_logic.c', 'b6b_loop.c', 'b6b_exc.c', 'b6b_proc.c',
	'b6b_strm.c', 'b6b_fdops.c', 'b6b_stdio.c', 'b6b_file.c', 'b6b_socket.c', 'b6b_timer.c', 'b6b_signal.c', 'b6b_sh.c', 'b6b_poll.c', 'b6b_evloop.c',
	'b6b_time.c', 'b6b_path.c', 'b6b_rand.c'
]
b6b_deps = [cc.find_library('m', required: true)]
b6b_libs = []

with_offload = false
if get_option('with_threads')
	add_project_arguments('-DB6B_HAVE_THREADS', language: 'c')
	libb6b_srcs += ['b6b_co.c', 'b6b_sem.c']
	b6b_deps += [dependency('threads')]

	with_offload = cc.has_header('stdatomic.h')
	if with_offload
		add_project_arguments('-DB6B_HAVE_OFFLOAD_THREAD', language: 'c')
		libb6b_srcs += ['b6b_offload.c']
	endif
endif

if get_option('optimistic_alloc')
	add_project_arguments('-DB6B_OPTIMISTIC_ALLOC', language: 'c')
endif

with_linenoise = get_option('with_linenoise')
if with_linenoise
	libb6b_srcs += ['linenoise/linenoise.c', 'b6b_linenoise.c']
endif

if get_option('with_miniz')
	if get_option('b_lto')
		error('LTO breaks linking with miniz')
	endif

	miniz_cflags = ['-DMINIZ_NO_ARCHIVE_APIS', '-DMINIZ_NO_ZLIB_COMPATIBLE_NAMES']
	add_project_arguments(miniz_cflags, language: 'c')

	miniz = subproject('miniz')
	b6b_deps += miniz.get_variable('miniz_dependency')

	libb6b_srcs += ['b6b_zlib.c']
endif

libffi = cc.find_library('ffi', required: false)
if libffi.found()
	libdl = cc.find_library('dl', required: false)
	if libdl.found()
		libb6b_srcs += ['b6b_ffi.c']
		b6b_deps += [libffi, libdl]
	endif
endif

if get_option('with_mbedtls')
	mbedtls_includes = include_directories('mbedtls/include')
	mbedtls_cflags = ['-DMBEDTLS_USER_CONFIG_FILE="@0@/mbedtls-config.h"'.format(meson.current_source_dir())]

	libmbedtls = static_library('mbedtls',
	                            'mbedtls/library/debug.c',
	                            'mbedtls/library/net_sockets.c',
	                            'mbedtls/library/ssl_cache.c',
	                            'mbedtls/library/ssl_ciphersuites.c',
	                            'mbedtls/library/ssl_cli.c',
	                            'mbedtls/library/ssl_cookie.c',
	                            'mbedtls/library/ssl_srv.c',
	                            'mbedtls/library/ssl_ticket.c',
	                            'mbedtls/library/ssl_tls.c',
	                            'mbedtls/library/ssl_msg.c',
	                            include_directories: mbedtls_includes,
	                            c_args: mbedtls_cflags)
	mbedtls = declare_dependency(link_with: libmbedtls)

	libmbedx509 = static_library('mbedx509',
	                             'mbedtls/library/x509.c',
	                             'mbedtls/library/x509_create.c',
	                             'mbedtls/library/x509_crl.c',
	                             'mbedtls/library/x509_crt.c',
	                             'mbedtls/library/x509_csr.c',
	                             'mbedtls/library/x509write_crt.c',
	                             'mbedtls/library/x509write_csr.c',
	                             include_directories: mbedtls_includes,
	                             c_args: mbedtls_cflags)
	mbedx509 = declare_dependency(link_with: libmbedx509)

	libmbedcrypto = static_library('mbedcrypto',
	                               'mbedtls/library/aes.c',
	                               'mbedtls/library/aesni.c',
	                               'mbedtls/library/aria.c',
	                               'mbedtls/library/asn1parse.c',
	                               'mbedtls/library/asn1write.c',
	                               'mbedtls/library/base64.c',
	                               'mbedtls/library/bignum.c',
	                               'mbedtls/library/camellia.c',
	                               'mbedtls/library/ccm.c',
	                               'mbedtls/library/chacha20.c',
	                               'mbedtls/library/chachapoly.c',
	                               'mbedtls/library/cipher.c',
	                               'mbedtls/library/cipher_wrap.c',
	                               'mbedtls/library/cmac.c',
	                               'mbedtls/library/constant_time.c',
	                               'mbedtls/library/ctr_drbg.c',
	                               'mbedtls/library/des.c',
	                               'mbedtls/library/dhm.c',
	                               'mbedtls/library/ecdh.c',
	                               'mbedtls/library/ecdsa.c',
	                               'mbedtls/library/ecjpake.c',
	                               'mbedtls/library/ecp.c',
	                               'mbedtls/library/ecp_curves.c',
	                               'mbedtls/library/entropy.c',
	                               'mbedtls/library/entropy_poll.c',
	                               'mbedtls/library/error.c',
	                               'mbedtls/library/gcm.c',
	                               'mbedtls/library/hkdf.c',
	                               'mbedtls/library/hmac_drbg.c',
	                               'mbedtls/library/md.c',
	                               'mbedtls/library/md5.c',
	                               'mbedtls/library/memory_buffer_alloc.c',
	                               'mbedtls/library/nist_kw.c',
	                               'mbedtls/library/oid.c',
	                               'mbedtls/library/padlock.c',
	                               'mbedtls/library/pem.c',
	                               'mbedtls/library/pk.c',
	                               'mbedtls/library/pk_wrap.c',
	                               'mbedtls/library/pkcs12.c',
	                               'mbedtls/library/pkcs5.c',
	                               'mbedtls/library/pkparse.c',
	                               'mbedtls/library/pkwrite.c',
	                               'mbedtls/library/platform.c',
	                               'mbedtls/library/platform_util.c',
	                               'mbedtls/library/poly1305.c',
	                               'mbedtls/library/ripemd160.c',
	                               'mbedtls/library/rsa.c',
	                               'mbedtls/library/rsa_alt_helpers.c',
	                               'mbedtls/library/sha1.c',
	                               'mbedtls/library/sha256.c',
	                               'mbedtls/library/sha512.c',
	                               'mbedtls/library/threading.c',
	                               'mbedtls/library/timing.c',
	                               'mbedtls/library/version.c',
	                               'mbedtls/library/version_features.c',
	                               include_directories: mbedtls_includes,
	                               c_args: mbedtls_cflags)
	mbedcrypto = declare_dependency(link_with: libmbedcrypto, include_directories: mbedtls_includes)

	b6b_deps += [mbedtls, mbedx509, mbedcrypto]

	ca_certs = custom_target('ca_certs',
	                         input: 'gen_ca_certs.sh',
	                         output: 'b6b_ca_certs.c',
	                         command: [
	                         	find_program('sh'),
	                         	'-e',
	                         	'@INPUT@',
	                         	join_paths(meson.current_source_dir(), 'curl', 'lib', 'mk-ca-bundle.pl'),
	                         	meson.current_build_dir()
	                         ])

	libb6b_srcs += ['b6b_ssl.c', ca_certs]
endif

libb6b = library('b6b',
                 libb6b_srcs,
                 include_directories: b6b_includes,
                 dependencies: b6b_deps,
                 link_with: b6b_libs,
                 install: true)

if get_option('default_library') == 'static' and with_linenoise
	b6b = executable('b6b',
	                 'b6b.c',
	                 include_directories: b6b_includes,
	                 dependencies: b6b_deps,
	                 link_whole: [libb6b],
	                 install: true)
elif with_linenoise
	b6b = executable('b6b',
	                 'b6b.c',
	                 include_directories: b6b_includes,
	                 link_with: [libb6b],
	                 install: true)
endif
