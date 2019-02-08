/*
 * Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2014 Nicira, Inc.
 * Copyright 2017 The Polycube Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * this file contains the minimum set of dependencies that are requires by the
 * stp module
 */

#ifndef _DEPENDENCIES_H
#define _DEPENDENCIES_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "byte-order.h"
#include "compiler.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/********** util.h **********/
#define OVS_NOT_REACHED() abort()

#define ovs_assert(CONDITION)   \
  if (!OVS_LIKELY(CONDITION)) { \
    abort();                    \
  }

#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif

/* Use "%"PRIuSIZE to format size_t with printf(). */
#ifdef _WIN32
#define PRIdSIZE "Id"
#define PRIiSIZE "Ii"
#define PRIoSIZE "Io"
#define PRIuSIZE "Iu"
#define PRIxSIZE "Ix"
#define PRIXSIZE "IX"
#else
#define PRIdSIZE "zd"
#define PRIiSIZE "zi"
#define PRIoSIZE "zo"
#define PRIuSIZE "zu"
#define PRIxSIZE "zx"
#define PRIXSIZE "zX"
#endif

/* Casts 'pointer' to 'type' and issues a compiler warning if the cast changes
 * anything other than an outermost "const" or "volatile" qualifier.
 *
 * The cast to int is present only to suppress an "expression using sizeof
 * bool" warning from "sparse" (see
 * http://permalink.gmane.org/gmane.comp.parsers.sparse/2967). */
#define CONST_CAST(TYPE, POINTER) \
  ((void)sizeof((int)((POINTER) == (TYPE)(POINTER))), (TYPE)(POINTER))

#define __ARRAY_SIZE_NOCHECK(ARRAY) (sizeof(ARRAY) / sizeof((ARRAY)[0]))
#ifdef __GNUC__
/* return 0 for array types, 1 otherwise */
#define __ARRAY_CHECK(ARRAY) \
  !__builtin_types_compatible_p(typeof(ARRAY), typeof(&ARRAY[0]))

/* compile-time fail if not array */
#define __ARRAY_FAIL(ARRAY) (sizeof(char[-2 * !__ARRAY_CHECK(ARRAY)]))
#define __ARRAY_SIZE(ARRAY)                                                \
  __builtin_choose_expr(__ARRAY_CHECK(ARRAY), __ARRAY_SIZE_NOCHECK(ARRAY), \
                        __ARRAY_FAIL(ARRAY))
#else
#define __ARRAY_SIZE(ARRAY) __ARRAY_SIZE_NOCHECK(ARRAY)
#endif

#ifdef __CHECKER__
#define BUILD_ASSERT(EXPR) ((void)0)
#define BUILD_ASSERT_DECL(EXPR) extern int(*build_assert(void))[1]
#elif !defined(__cplusplus)
/* Build-time assertion building block. */
#define BUILD_ASSERT__(EXPR) \
  sizeof(struct { unsigned int build_assert_failed : (EXPR) ? 1 : -1; })

/* Build-time assertion for use in a statement context. */
#define BUILD_ASSERT(EXPR) (void)BUILD_ASSERT__(EXPR)

/* Build-time assertion for use in a declaration context. */
#define BUILD_ASSERT_DECL(EXPR) \
  extern int(*build_assert(void))[BUILD_ASSERT__(EXPR)]
#else /* __cplusplus */
#define BUILD_ASSERT static_assert
#define BUILD_ASSERT_DECL static_assert
#endif /* __cplusplus */

/* Returns the number of elements in ARRAY. */
#define ARRAY_SIZE(ARRAY) __ARRAY_SIZE(ARRAY)

/* Expands to a string that looks like "<file>:<line>", e.g. "tmp.c:10".
 *
 * See http://c-faq.com/ansi/stringize.html for an explanation of STRINGIZE and
 * STRINGIZE2. */
#define SOURCE_LOCATOR __FILE__ ":" STRINGIZE(__LINE__)
#define STRINGIZE(ARG) STRINGIZE2(ARG)
#define STRINGIZE2(ARG) #ARG

void *xzalloc(size_t) MALLOC_LIKE;
char *xstrdup(const char *) MALLOC_LIKE;
void out_of_memory();
void *xcalloc(size_t count, size_t size);
void *xmalloc(size_t size);
void *xzalloc(size_t size);
char *xmemdup0(const char *p_, size_t length);
char *xstrdup(const char *s);

/********** ofpbuf.h **********/
struct ofpbuf;

struct ofpbuf *ofpbuf_new(size_t size);
void ofpbuf_free(struct ofpbuf *p);
void *ofpbuf_base(struct ofpbuf *p);
ssize_t ofpbuf_size(struct ofpbuf *p);

/********** log.h **********/
// void VLOG_WARN(const char *format, ...);
// void VLOG_DBG(const char *format, ...);
// void VLOG_INFO(const char *format, ...);

/********** ovs-thread.h **********/
void ovs_mutex_init(pthread_mutex_t *m);
void ovs_mutex_lock(pthread_mutex_t *m);
void ovs_mutex_unlock(pthread_mutex_t *m);

#ifdef __cplusplus
}
#endif
#endif
