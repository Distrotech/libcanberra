/*-*- Mode: C; c-basic-offset: 8 -*-*/

#ifndef foocanberramutexhfoo
#define foocanberramutexhfoo

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

#include "macro.h"

typedef struct ca_mutex ca_mutex;

ca_mutex* ca_mutex_new(void);
void ca_mutex_free(ca_mutex *m);

void ca_mutex_lock(ca_mutex *m);
ca_bool_t ca_mutex_try_lock(ca_mutex *m);
void ca_mutex_unlock(ca_mutex *m);

#endif
