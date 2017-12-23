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

FROM ubuntu:artful

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8

RUN sed -i s/^deb.*multiverse.*/\#\&/g /etc/apt/sources.list
RUN apt-get -qq update
RUN apt-get -y --no-install-recommends install gcc clang ninja-build locales python3-pip python3-setuptools python3-wheel libffi-dev valgrind curl gcovr
RUN locale-gen --lang en_US.UTF-8
RUN pip3 install meson

ADD . /root
