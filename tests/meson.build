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

components = ['cast', 'list_parse', 'if', 'try', 'map', 'dict', 'str', 'file', 'signal', 'poll', 'proc', 'sh', 'math']

if get_option('with_threads')
	components += ['spawn', 'co', 'sem']

	if with_offload
		components += ['offload']
	endif
endif

if get_option('with_miniz')
	components += ['zlib']
endif

if libffi.found()
	components += ['ffi']
endif

if get_option('with_threads')
	test_deps = [dependency('threads')]
else
	test_deps = []
endif

if get_option('with_valgrind')
	timeout = 60
else
	timeout = 10
endif

if get_option('default_library') != 'static'
	foreach component: components
		test_name = 'b6b_test_@0@'.format(component)
		test_exe = executable(test_name,
		                      '@0@.c'.format(test_name),
		                      include_directories: b6b_includes,
		                      dependencies: test_deps,
		                      link_with: [libb6b])
		test(test_name, test_exe, timeout: timeout)
	endforeach
endif
