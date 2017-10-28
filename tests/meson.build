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

tests = [
	['cast', 'quick', 5],
	['list_parse', 'quick', 5],
	['if', 'quick', 5],
	['try', 'quick', 5],
	['map', 'quick', 5],
	['dict', 'quick', 5],
	['str', 'quick', 5],
	['file', 'quick', 5],
	['signal', 'quick', 5],
	['poll', 'quick', 5],
	['proc', 'quick', 5],
	['sh', 'slow', 10],
	['math', 'quick', 5]
]

if get_option('with_threads')
	tests += [
		['spawn', 'quick', 5],
		['co', 'quick', 5],
		['sem', 'quick', 5]
	]

	if with_offload
		tests += [['offload', 'slow', 15]]
	endif
endif

if get_option('with_miniz')
	tests += [['zlib', 'quick', 5]]
endif

if libffi.found()
	tests += [['ffi', 'quick', 5]]
endif

if get_option('with_threads')
	test_deps = [dependency('threads')]
else
	test_deps = []
endif

if get_option('default_library') != 'static'
	foreach test: tests
		test_name = 'b6b_test_@0@'.format(test[0])
		test_exe = executable(test_name,
		                      '@0@.c'.format(test_name),
		                      include_directories: b6b_includes,
		                      dependencies: test_deps,
		                      link_with: [libb6b])
		test(test[0], test_exe, timeout: test[2], suite: test[1])
	endforeach
endif
