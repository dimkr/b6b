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

libb6b = library('b6b',
                 'b6b_obj.c', 'b6b_str.c', 'b6b_num.c', 'b6b_list.c', 'b6b_dict.c',
                 'b6b_frame.c', 'b6b_thread.c', 'b6b_interp.c',
                 'b6b_math.c', 'b6b_logic.c', 'b6b_loop.c', 'b6b_exc.c', 'b6b_proc.c', 'b6b_co.c',
                 'b6b_strm.c', 'b6b_file.c',
                 'b6b_time.c',
                 'linenoise/linenoise.c', 'b6b_linenoise.c',
                 include_directories: b6b_includes,
                 dependencies: [cc.find_library('m', required: false)])

b6b = executable('b6b',
                 'b6b.c',
                 include_directories: b6b_includes,
                 link_with: [libb6b])
