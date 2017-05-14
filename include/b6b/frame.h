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

#define B6B_MAX_NESTING 32

struct b6b_frame {
	struct b6b_obj *locals;
	struct b6b_obj *args;
	struct b6b_frame *prev;
};

struct b6b_frame *b6b_frame_new(struct b6b_frame *prev);
int b6b_frame_set_args(struct b6b_interp *interp,
                       struct b6b_frame *f,
                       struct b6b_obj *args);
void b6b_frame_destroy(struct b6b_frame *f);

struct b6b_frame *b6b_frame_push(struct b6b_interp *interp);
void b6b_frame_pop(struct b6b_interp *interp);
