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

project('b6b', 'c', version: '0.0.1', license: 'Apache-2.0')

add_global_arguments('-D_FILE_OFFSET_BITS=64', language: 'c')
add_project_arguments('-Wall', '-Wno-overlength-strings', language: 'c')

subdir('include')
subdir('src')
subdir('doc')

install_data('README', 'AUTHORS', 'COPYING',
             install_dir: join_paths(get_option('datadir'), 'doc', 'b6b'))
