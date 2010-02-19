/*-*- Mode: C; c-basic-offset: 8 -*-*/

#ifndef foocanberracommonh
#define foocanberracommonh

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

#include "canberra.h"
#include "macro.h"
#include "mutex.h"

struct ca_context {
        ca_bool_t opened;
        ca_mutex *mutex;

        ca_proplist *props;

        char *driver;
        char *device;

        void *private;
#ifdef HAVE_DSO
        void *private_dso;
#endif
};

typedef enum ca_cache_control {
        CA_CACHE_CONTROL_NEVER,
        CA_CACHE_CONTROL_PERMANENT,
        CA_CACHE_CONTROL_VOLATILE
} ca_cache_control_t;

int ca_parse_cache_control(ca_cache_control_t *control, const char *c);

#endif
