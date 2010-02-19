/*-*- Mode: C; c-basic-offset: 8 -*-*/

#ifndef foocanberramallochfoo
#define foocanberramallochfoo

/***
  This file is part of libcanberra.

  Copyright 2008 Lennart Poettering

  libcanberra is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 2.1 of the
  License, or (at your option) any later version.

  libcanberra is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with libcanberra. If not, see
  <http://www.gnu.org/licenses/>.
***/

#include <stdlib.h>
#include <string.h>

#include "canberra.h"
#include "macro.h"

#ifndef PACKAGE
#error "Please include config.h before including this file!"
#endif

#define ca_malloc malloc
#define ca_free free
#define ca_malloc0(size) calloc(1, (size))
#define ca_strdup strdup
#ifdef HAVE_STRNDUP
#define ca_strndup strndup
#else
char *ca_strndup(const char *s, size_t n);
#endif

void* ca_memdup(const void* p, size_t size);

#define ca_new(t, n) ((t*) ca_malloc(sizeof(t)*(n)))
#define ca_new0(t, n) ((t*) ca_malloc0(sizeof(t)*(n)))
#define ca_newdup(t, p, n) ((t*) ca_memdup(p, sizeof(t)*(n)))

char *ca_sprintf_malloc(const char *format, ...) __attribute__((format(printf, 1, 2)));

#endif
