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

	libucontext = subproject('libucontext')
	b6b_deps += libucontext.get_variable('libucontext_dep')
	b6b_includes += libucontext.get_variable('headers')

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
