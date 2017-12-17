# This file is part of b6b.
#
# Copyright 2017 Dima Krasner
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

core_tests = [
	['hash', 'quick', 5],
	['cast', 'quick', 5],
	['istrue', 'quick', 5],
	['list_parse', 'quick', 5],

	['nop', 'quick', 5],
	['echo', 'quick', 5],
	['return', 'quick', 5],
	['exit', 'quick', 5],
	['throw', 'quick', 5],
	['break', 'quick', 5],
	['continue', 'quick', 5],

	['eval', 'quick', 5],
	['call', 'quick', 5],
	['source', 'quick', 5],

	['global', 'quick', 5],
	['local', 'quick', 5],
	['export', 'quick', 5],

	['repr', 'quick', 5],
	['b6b', 'quick', 5],

	['math', 'quick', 5],
	['dict_get', 'quick', 5],
	['dict_set', 'quick', 5],
	['dict_unset', 'quick', 5],

	['str_len', 'quick', 5],
	['str_index', 'quick', 5],
	['str_range', 'quick', 5],
	['str_join', 'quick', 5],
	['str_split', 'quick', 5],
	['str_ord', 'quick', 5],
	['str_chr', 'quick', 5],
	['str_expand', 'quick', 5],
	['str_in', 'quick', 5],
	['str_ltrim', 'quick', 5],
	['str_rtrim', 'quick', 5],

	['list_new', 'quick', 5],
	['list_len', 'quick', 5],
	['list_in', 'quick', 5],
	['list_append', 'quick', 5],
	['list_extend', 'quick', 5],
	['list_index', 'quick', 5],
	['list_range', 'quick', 5],
	['list_pop', 'quick', 5],

	['obj_decode', 'quick', 5],

	['if', 'quick', 5],
	['try', 'quick', 5],
	['range', 'quick', 5],
	['map', 'quick', 5],
	['proc', 'quick', 5],

	['socket_endian', 'quick', 5],
	['randint', 'quick', 5],
	['choice', 'quick', 5],

	['sleep', 'slow', 7]
]

stream_tests = [
	['file', 'quick', 5],
	['signal', 'quick', 5],
	['un_pair', 'quick', 5],
	['timer', 'quick', 10],
	['poll', 'quick', 5],
	['sh', 'slow', 10]
]

binding_tests = []

if get_option('with_threads')
	core_tests += [
		['spawn', 'quick', 5],
		['co', 'quick', 5],
	]

	if with_offload
		core_tests += [['offload', 'slow', 15]]
	endif

	stream_tests += [['sem', 'quick', 5]]
endif

if get_option('with_miniz')
	binding_tests += [
		['deflate', 'quick', 5],
		['inflate', 'quick', 5],
		['compress', 'quick', 5],
		['decompress', 'quick', 5]
	]
endif

if libffi.found()
	binding_tests += [
		['ffi_memcpy', 'quick', 5],
		['ffi_buf', 'quick', 5],
		['ffi_pack', 'quick', 5],
		['ffi_unpack', 'quick', 5],
		['ffi_dlopen', 'quick', 5],
		['ffi_call', 'quick', 5],
	]
endif

if get_option('with_threads')
	test_deps = [dependency('threads')]
else
	test_deps = []
endif

here = meson.current_source_dir()
if get_option('default_library') != 'static'
	foreach test: core_tests + stream_tests + binding_tests
		test_name = 'b6b_test_@0@'.format(test[0])
		test_exe = executable(test_name,
		                      '@0@.c'.format(test_name),
		                      include_directories: b6b_includes,
		                      dependencies: test_deps,
		                      link_with: [libb6b])
		test(test[0], test_exe, timeout: test[2], suite: test[1], workdir: here)
	endforeach
endif
