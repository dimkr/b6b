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

export CFLAGS=-g
meson -Dwith_valgrind=true /builds/gcc
CC=clang meson -Dwith_valgrind=true /builds/clang
meson -Dwith_threads=false -Dwith_valgrind=true /builds/no-threads
meson -Dwith_threads=false -Dwith_miniz=false -Dwith_linenoise=false /builds/small
meson -Db_coverage=true -Doptimistic_alloc=false /builds/coverage

# build with GCC, clang, with GCC while thread support is disabled and a small build with all optional features off
for i in /builds/gcc /builds/clang /builds/no-threads
do
	ninja -C $i
	meson configure -Dbuildtype=release $i
	DESTDIR=dest ninja -C $i install
done

DESTDIR=dest ninja -C /builds/small install
meson test -C /builds/small --no-rebuild --print-errorlogs

# rebuild the small build with a static library
meson configure -Ddefault_library=static /builds/small
DESTDIR=dest-static ninja -C /builds/small install

# run all tests
for i in /builds/gcc /builds/clang
do
	meson test -C $i --no-rebuild --print-errorlogs --repeat 5
	meson test -C $i --no-rebuild --print-errorlogs --wrapper "taskset -c 0"
done

# this is required to work around missing suppressions in glibc's symbol lookup
export LD_BIND_NOW=1

# run all tests, with Valgrind
for i in /builds/gcc /builds/clang
do
	meson test -C $i --no-rebuild --print-errorlogs --no-suite=b6b:slow --num-processes 1 -t 2 --wrapper "valgrind --leak-check=full --error-exitcode=1 --malloc-fill=1 --free-fill=1 --track-fds=yes"
	meson test -C $i --no-rebuild --print-errorlogs --no-suite=b6b:slow --num-processes 1 -t 2 --wrapper "valgrind --tool=helgrind --error-exitcode=1"
	meson test -C $i --no-rebuild --print-errorlogs --no-suite=b6b:slow --num-processes 1 -t 2 --wrapper "valgrind --tool=helgrind --error-exitcode=1 --fair-sched=yes"
done

ninja test -C /builds/coverage
