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

#define B6B_EVLOOP_BODY \
	"{$global evloop._fdp [$poll]}\n" \
	"{$global evloop._fdstrm {}}\n" \
	"{$global evloop._fdintp {}}\n" \
	"{$global evloop._fdoutp {}}\n" \
	"{$global evloop._fderrp {}}\n" \
	"{$proc evloop.remove {" \
		"{$try {" \
			"{$local fd [$1 fd]}\n" \
			"{$evloop._fdp remove $fd}\n" \
			"{$dict.unset $evloop._fdstrm $fd}\n" \
			"{$dict.unset $evloop._fderrp $fd}\n" \
			"{$dict.unset $evloop._fdoutp $fd}\n" \
			"{$dict.unset $evloop._fdintp $fd}" \
		"} {} {" \
			"{$1 close}" \
		"}}" \
	"}}\n" \
	"{$proc evloop.add {" \
		"{$local fd [$1 fd]}\n" \
		"{$dict.set $evloop._fdstrm $fd $1}\n" \
		"{$dict.set $evloop._fdintp $fd $2}\n" \
		"{$dict.set $evloop._fdoutp $fd $3}\n" \
		"{$dict.set $evloop._fderrp $fd $4}\n" \
		"{$if [$&& $2 $3] {" \
			"{$evloop._fdp add $fd $POLLINOUT}" \
		"} {" \
			"{$if $1 {" \
				"{$evloop._fdp add $fd $POLLIN}" \
			"} {" \
				"{$evloop._fdp add $fd $POLLOUT}" \
			"}}" \
		"}}" \
	"}}\n" \
	"{$proc evloop.update {" \
		"{$evloop._fdp remove [$1 fd]}\n" \
		"{$evloop.add $1 $2 $3 $4}" \
	"}}\n" \
	"{$proc evloop.after {" \
		"{$local t [$timer $1]}\n" \
		"{$local fd [$t fd]}\n" \
		"{$dict.set $evloop._fdstrm $fd $t}\n" \
		"{$dict.set $evloop._fdintp $fd [$proc {} {" \
			"{$try {" \
				"{$call [$list.new [$list.new $.]]}" \
			"} {" \
				"{$throw}" \
			"} {" \
				"{$try {{$evloop.remove $1}}}\n" \
			"}}" \
		"} $2]}\n" \
		"{$dict.set $evloop._fdoutp $fd {{$nop}}}\n" \
		"{$dict.set $evloop._fderrp $fd {{$nop}}}\n" \
		"{$evloop._fdp add $fd $POLLIN}" \
	"}}\n" \
	"{$proc evloop.wait {" \
		"{$while 1 {" \
			"{$local n [$list.len $evloop._fdstrm]}\n" \
			"{$if [$== $n 0] {" \
				"{$break}" \
			"}}\n" \
			"{$local evs [$evloop._fdp wait [$* $n 1.5] $1]}\n" \
			"{$map fd [$list.index $evs 2] {" \
				"{$local strm [$dict.get $evloop._fdstrm $fd {}]}\n" \
				"{$if $strm {" \
					"{$try {" \
						"{[$dict.get $evloop._fderrp $fd] $strm}" \
					"} {} {" \
						"{$evloop.remove $strm}" \
					"}}" \
				"}}" \
			"}}\n" \
			"{$map fd [$list.index $evs 1] {" \
				"{$local strm [$dict.get $evloop._fdstrm $fd {}]}\n" \
				"{$if $strm {" \
					"{$try {" \
						"{[$dict.get $evloop._fdoutp $fd] $strm}" \
					"} {" \
						"{$evloop.remove $strm}" \
					"}}" \
				"}}" \
			"}}\n" \
			"{$map fd [$list.index $evs 0] {" \
				"{$local strm [$dict.get $evloop._fdstrm $fd {}]}\n" \
				"{$if $strm {" \
					"{$try {" \
						"{[$dict.get $evloop._fdintp $fd] $strm}" \
					"} {" \
						"{$evloop.remove $strm}" \
					"}}" \
				"}}" \
			"}}" \
		"}}" \
	"}}"

static int b6b_evloop_init(struct b6b_interp *interp)
{
	return b6b_call_copy(interp,
	                     B6B_EVLOOP_BODY,
	                     sizeof(B6B_EVLOOP_BODY) - 1) == B6B_OK;
}
__b6b_init(b6b_evloop_init);
