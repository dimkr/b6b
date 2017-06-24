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

install_man('b6b.1')

asciidoc = find_program('asciidoc', required: false)
if asciidoc.found()
	custom_target('manual',
	              input: 'b6b.txt',
	              output: 'b6b.html',
	              command: [asciidoc.path(), '-a', 'toc', '-a', 'toclevels=4', '-o', '@OUTPUT@', '@INPUT@'],
	              install: true,
	              install_dir: docdir)
endif
