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

#define B6B_CO_BODY \
	"{$global _fco 1}\n" \
	"{$global _cos {}}\n" \
	"{$proc co {" \
		"{$list.append $_cos $1}\n" \
		"{$if $_fco {" \
			"{$spawn {" \
				"{$loop {" \
					"{$if $_cos {" \
						"{$try {" \
							"{$call [$list.pop $_cos 0]}" \
						"}}" \
					"} {" \
						"{$global _fco 1}\n" \
						"{$return}" \
					"}}" \
				"}}" \
			"}}\n" \
			"{$global _fco 0}" \
		"}}" \
	"}}" \

static int b6b_co_init(struct b6b_interp *interp)
{
	return b6b_call_copy(interp,
	                     B6B_CO_BODY,
	                     sizeof(B6B_CO_BODY) - 1) == B6B_OK;
}
__b6b_init(b6b_co_init);
