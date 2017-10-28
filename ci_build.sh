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

rm -rf build build-clang build-no-threads build-small

export CFLAGS=-g
meson -Dwith_valgrind=true build
CC=clang meson -Dwith_doc=false -Dwith_valgrind=true build-clang
meson -Dwith_doc=false -Dwith_threads=false -Dwith_valgrind=true build-no-threads
meson -Dwith_doc=false -Dwith_threads=false -Dwith_miniz=false -Dwith_linenoise=false build-small

# build with GCC, clang, with GCC while thread support is disabled and a small build with all optional features off
for i in build build-clang build-no-threads
do
	ninja -C $i
	meson configure -Dbuildtype=release $i
	DESTDIR=dest ninja -C $i install
done

DESTDIR=dest ninja -C build-small install
meson test -C build-small --no-rebuild --print-errorlogs

# rebuild the small build with a static library
meson configure -Ddefault_library=static build-small
DESTDIR=dest-static ninja -C build-small install

for i in build build-clang
do
	meson test -C $i --no-rebuild --print-errorlogs --repeat 5
	meson test -C $i --no-rebuild --print-errorlogs --repeat 5 --wrapper "taskset -c 0"
	meson test -C $i --no-rebuild --print-errorlogs --no-suite=b6b:slow --num-processes 1 -t 2 --wrapper "valgrind --leak-check=full --malloc-fill=1 --free-fill=1 --track-fds=yes"
	meson test -C $i --no-rebuild --print-errorlogs --no-suite=b6b:slow --num-processes 1 -t 2 --wrapper "valgrind --tool=helgrind"
	meson test -C $i --no-rebuild --print-errorlogs --no-suite=b6b:slow --num-processes 1 -t 2 --wrapper "valgrind --tool=helgrind --fair-sched=yes"
done
