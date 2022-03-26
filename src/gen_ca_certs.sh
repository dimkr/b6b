#!/bin/sh -e

# This file is part of b6b.
#
# Copyright 2022 Dima Krasner
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

cd $2

perl $1 -q cert.pem > /dev/null
rm -f certdata.txt

cat << EOF > b6b_ca_certs.c
#include <sys/types.h>

static const unsigned char arr[] = \\
EOF

grep -v -e ^# -e '^$' cert.pem |
while read x
do
    echo "    \"$x\\n\" \\"
done >> b6b_ca_certs.c

cat << EOF >> b6b_ca_certs.c
;

const unsigned char *b6b_ca_certs = arr;
const size_t b6b_ca_certs_len = sizeof(arr);
EOF

rm -f cert.pem
