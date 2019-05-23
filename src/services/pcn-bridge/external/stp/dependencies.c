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

#include "dependencies.h"
#include <stdarg.h>
#include <stdio.h>

/********** util.c **********/
void out_of_memory() {
  abort();
}

void *xcalloc(size_t count, size_t size) {
  void *p = count && size ? calloc(count, size) : malloc(1);
  // COVERAGE_INC(util_xalloc);
  if (p == NULL) {
    out_of_memory();
  }
  return p;
}

void *xmalloc(size_t size) {
  void *p = malloc(size ? size : 1);
  // COVERAGE_INC(util_xalloc);
  if (p == NULL) {
    out_of_memory();
  }
  return p;
}

void *xzalloc(size_t size) {
  return xcalloc(1, size);
}

char *xmemdup0(const char *p_, size_t length) {
  char *p = xmalloc(length + 1);
  memcpy(p, p_, length);
  p[length] = '\0';
  return p;
}

char *xstrdup(const char *s) {
  return xmemdup0(s, strlen(s));
}

/********** ofpbuf.c **********/

struct ofpbuf {
  void *base_;    /* First byte of allocated space. */
  uint32_t size_; /* Number of allocated bytes. */
};

struct ofpbuf *ofpbuf_new(size_t size) {
  struct ofpbuf *p = xmalloc(sizeof(*p));
  p->base_ = xmalloc(size);
  p->size_ = size;
  return p;
}

void ofpbuf_free(struct ofpbuf *p) {
  free(p->base_);
  free(p);
}

void *ofpbuf_base(struct ofpbuf *p) {
  return p->base_;
}

ssize_t ofpbuf_size(struct ofpbuf *p) {
  return p->size_;
}

/********** ovs-thread.c **********/
void ovs_mutex_init(pthread_mutex_t *m) {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(m, &attr);
}

void ovs_mutex_lock(pthread_mutex_t *m) {
  pthread_mutex_lock(m);
}

void ovs_mutex_unlock(pthread_mutex_t *m) {
  pthread_mutex_unlock(m);
}
