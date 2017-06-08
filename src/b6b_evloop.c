/*
 * This file is part of b6b.
 *
 * Copyright 2017 Dima Krasner
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <b6b.h>

#define B6B_EVLOOP_REMOVE \
	"{$try {" \
		"{$local fd [$2 fd]}\n" \
		"{[$list.index $. 0] remove $fd}\n" \
		"{$map d [$list.range $. 1 5] {" \
			"{$dict.unset $d $fd}" \
		"}}" \
	"} {} {" \
		"{$2 close}" \
	"}}"

#define B6B_EVLOOP_ADD \
	"{$local fd [$2 fd]}\n" \
	"{$dict.set [$list.index $. 1] $fd $2}\n" \
	"{$dict.set [$list.index $. 2] $fd $3}\n" \
	"{$dict.set [$list.index $. 3] $fd $4}\n" \
	"{$dict.set [$list.index $. 4] $fd $5}\n" \
	"{$if [$&& $3 $4] {" \
		"{[$list.index $. 0] add $fd $POLLINOUT}" \
	"} {" \
		"{$if $3 {" \
			"{[$list.index $. 0] add $fd $POLLIN}" \
		"} {" \
			"{[$list.index $. 0] add $fd $POLLOUT}" \
		"}}" \
	"}}"

#define B6B_EVLOOP_UPDATE \
	"{[$list.index $. 0] remove [$2 fd]}\n" \
	"{$0 add $2 $3 $4 $5}"

#define B6B_EVLOOP_AFTER \
	"{$0 add [$timer $2] [$proc _ {" \
		"{$try {" \
			"{$call [$list.index $. 0]}" \
		"} {" \
			"{$throw}" \
		"} {" \
			"{$try {{[$list.index $. 1] remove $1}}}" \
		"}}" \
	"} [$list.new [$list.new [$list.new $3]] $0]] {} {}}"

#define B6B_EVLOOP_EVERY \
	"{$0 add [$timer $2] [$proc _ {" \
		"{$1 read}\n" \
		"{$call $.}" \
	"} [$list.new [$list.new $3]]] {} {}}"

#define B6B_EVLOOP_WAIT \
	"{$while 1 {" \
		"{$map {p strms inps outps errps} $. {" \
			"{$local n [$list.len $strms]}\n" \
			"{$if [$== $n 0] {" \
				"{$return}" \
			"}}\n" \
			"{$map {rfds wfds efds} [$p wait [$* $n 1.5] $2] {" \
				"{$map fd $efds {" \
					"{$local strm [$dict.get $strms $fd {}]}\n" \
					"{$if $strm {" \
						"{$try {" \
							"{[$dict.get $errps $fd] $strm}" \
						"} {} {" \
							"{$0 remove $strm}" \
						"}}" \
					"}}" \
				"}}\n" \
				"{$map fd $wfds {" \
					"{$local strm [$dict.get $strms $fd {}]}\n" \
					"{$if $strm {" \
						"{$try {" \
							"{[$dict.get $outps $fd] $strm}" \
						"} {" \
							"{$0 remove $strm}" \
						"}}" \
					"}}" \
				"}}\n" \
				"{$map fd $rfds {" \
					"{$local strm [$dict.get $strms $fd {}]}\n" \
					"{$if $strm {" \
						"{$try {" \
							"{[$dict.get $inps $fd] $strm}" \
						"} {" \
							"{$0 remove $strm}" \
						"}}" \
					"}}" \
				"}}" \
			"}}" \
		"}}" \
	"}}"

#define B6B_EVLOOP_BODY \
	"{$proc evloop {" \
		"{$proc _ {" \
			"{$if [$== $1 remove] {" \
				B6B_EVLOOP_REMOVE \
			"} {" \
				"{$if [$== $1 add] {" \
					B6B_EVLOOP_ADD \
				"} {" \
					"{$if [$== $1 update] {" \
						B6B_EVLOOP_UPDATE \
					"} {" \
						"{$if [$== $1 after] {" \
							B6B_EVLOOP_AFTER \
						"} {" \
							"{$if [$== $1 every] {" \
								B6B_EVLOOP_EVERY \
							"} {" \
								"{$if [$== $1 wait] {" \
									B6B_EVLOOP_WAIT \
								"} {" \
									"{$throw {bad evloop op}}" \
								"}}" \
							"}}" \
						"}}" \
					"}}" \
				"}}" \
			"}}" \
		"} [$list.new [$poll] {} {} {} {}]}" \
	"}}"

static int b6b_evloop_init(struct b6b_interp *interp)
{
	return b6b_call_copy(interp,
	                     B6B_EVLOOP_BODY,
	                     sizeof(B6B_EVLOOP_BODY) - 1) == B6B_OK;
}
__b6b_init(b6b_evloop_init);
