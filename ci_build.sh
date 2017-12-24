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
meson -Dwith_valgrind=true build-gcc
CC=clang meson -Dwith_valgrind=true build-clang
meson -Dwith_threads=false -Dwith_valgrind=true build-no-threads
meson -Dwith_threads=false -Dwith_miniz=false -Dwith_linenoise=false build-small
meson -Db_coverage=true -Doptimistic_alloc=false build-coverage

# build with GCC, clang, with GCC while thread support is disabled and a small build with all optional features off
for i in build-gcc build-clang build-no-threads
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

for i in build-gcc build-clang
do
	# run all single-threaded tests
	meson test -C $i --no-rebuild --print-errorlogs --no-suite b6b:threaded

	# run multi-threaded tests on a single CPU and 5 additional times, to increase the chance of triggering race conditions
	meson test -C $i --no-rebuild --print-errorlogs --suite b6b:threaded --wrapper "taskset -c 0"
	meson test -C $i --no-rebuild --print-errorlogs --suite b6b:threaded --repeat 5
done

# this is required to work around missing suppressions in glibc's symbol lookup
export LD_BIND_NOW=1

for i in build-gcc build-clang
do
	# run all tests except extremely slow ones with Valgrind
	meson test -C $i --no-rebuild --print-errorlogs --no-suite b6b:slow --num-processes 1 -t 3 --wrapper "valgrind --leak-check=full --error-exitcode=1 --malloc-fill=1 --free-fill=1 --track-fds=yes"

	# run multi-threaded tests with Helgrind
	meson test -C $i --no-rebuild --print-errorlogs --suite b6b:threaded --num-processes 1 -t 3 --wrapper "valgrind --tool=helgrind --error-exitcode=1"
	meson test -C $i --no-rebuild --print-errorlogs --suite b6b:threaded --num-processes 1 -t 3 --wrapper "valgrind --tool=helgrind --error-exitcode=1 --fair-sched=yes"
done

meson test -C build-coverage
cd build-coverage
curl -s https://codecov.io/bash | bash
