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

cc = meson.get_compiler('c')

if get_option('buildtype').startswith('debug') and cc.has_header('valgrind/valgrind.h') and cc.has_header('valgrind/memcheck.h')
	add_project_arguments('-DB6B_HAVE_VALGRIND', language: 'c')
endif

libb6b_srcs = [
	'b6b_obj.c', 'b6b_str.c', 'b6b_num.c', 'b6b_list.c', 'b6b_dict.c',
	'b6b_frame.c', 'b6b_thread.c', 'b6b_interp.c',
	'b6b_math.c', 'b6b_logic.c', 'b6b_loop.c', 'b6b_exc.c', 'b6b_proc.c', 'b6b_co.c',
	'b6b_strm.c', 'b6b_fdops.c', 'b6b_stdio.c', 'b6b_file.c', 'b6b_socket.c', 'b6b_timer.c', 'b6b_signal.c', 'b6b_sem.c', 'b6b_sh.c', 'b6b_poll.c', 'b6b_evloop.c',
	'b6b_time.c', 'b6b_path.c', 'b6b_rand.c',
	'linenoise/linenoise.c', 'b6b_linenoise.c',
	'miniz/miniz_tdef.c', 'miniz/miniz_tinfl.c', 'miniz/miniz.c', 'b6b_zlib.c'
]
b6b_deps = [cc.find_library('m', required: false)]

# b6b cannot be linked with a static libb6b due to the use of sections in
# __b6b_ext and __b6b_init, which get lost in a static build
if get_option('default_library') == 'static'
	b6b = executable('b6b',
	                 libb6b_srcs + ['b6b.c'],
	                 include_directories: b6b_includes,
	                 dependencies: b6b_deps,
	                 install: true)
else
	libb6b = library('b6b',
	                 libb6b_srcs,
	                 include_directories: b6b_includes,
	                 dependencies: b6b_deps,
	                 install: true)

	b6b = executable('b6b',
	                 'b6b.c',
	                 include_directories: b6b_includes,
	                 link_with: [libb6b],
	                 install: true)
endif
