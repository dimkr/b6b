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

#define b6b_likely(cond) __builtin_expect(!!(cond), 1)
#define b6b_unlikely(cond) __builtin_expect(!!(cond), 0)

/* if malloc() can return a non-NULL pointer when out of memory and writing to
 * that address will crash the process, checking the return value of malloc() is
 * useless; if malloc() gets to return NULL, we let the process crash due to
 * dereference of NULL */
#ifdef B6B_OPTIMISTIC_ALLOC
#	define b6b_allocated(cond) 1
#else
#	define b6b_allocated(cond) b6b_likely(cond)
#endif

/**
 * @enum b6b_res
 * Procedure status codes, which control the flow of execution
 */
enum b6b_res {
	B6B_OK, /**< Success */
	B6B_ERR, /**< Error: the next statement shall not be executed */
	B6B_YIELD, /**< Switch to another thread of execution */
	B6B_BREAK, /**< Success, but the next statement shall not be executed */
	B6B_CONT, /**< Jump back to the first statement in the loop body */
	B6B_RET, /**< Stop execution of statements in the current block and
	              stop the caller with B6B_BREAK */
	B6B_EXIT /**< Exit the script */
};
