/*-*- Mode: C; c-basic-offset: 8 -*-*/

#ifndef foocanberradriverhfoo
#define foocanberradriverhfoo

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

int driver_open(ca_context *c);
int driver_destroy(ca_context *c);

int driver_change_device(ca_context *c, const char *device);
int driver_change_props(ca_context *c, ca_proplist *changed, ca_proplist *merged);

int driver_play(ca_context *c, uint32_t id, ca_proplist *p, ca_finish_callback_t cb, void *userdata);
int driver_cancel(ca_context *c, uint32_t id);
int driver_cache(ca_context *c, ca_proplist *p);

int driver_playing(ca_context *c, uint32_t id, int *playing);

#endif
