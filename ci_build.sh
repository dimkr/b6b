#!/bin/sh -xe

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

export LC_ALL=en_US.UTF-8

meson -Db_coverage=true -Dwith_doc=false build-debug
meson --buildtype release build-release
CC=clang meson -Dwith_doc=false build-clang-debug
CC=clang meson --buildtype release -Dwith_doc=false build-clang-release

for i in "" -clang
do
	for j in -release -debug
	do
		ninja -C build$i$j
		mesontest -C build$i$j --print-errorlogs
	done

	DESTDIR=dest ninja -C build$i-release install
done

mesontest -C build-release --print-errorlogs --num-processes 1 --wrapper "valgrind --leak-check=full --malloc-fill=1 --free-fill=1 --track-fds=yes"
