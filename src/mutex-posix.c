/*-*- Mode: C; c-basic-offset: 8 -*-*/

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>
#include <errno.h>

#include "mutex.h"
#include "malloc.h"

struct ca_mutex {
        pthread_mutex_t mutex;
};

ca_mutex* ca_mutex_new(void) {
        ca_mutex *m;

        if (!(m = ca_new(ca_mutex, 1)))
                return NULL;

        if (pthread_mutex_init(&m->mutex, NULL) < 0) {
                ca_free(m);
                return NULL;
        }

        return m;
}

void ca_mutex_free(ca_mutex *m) {
        ca_assert(m);

        ca_assert_se(pthread_mutex_destroy(&m->mutex) == 0);
        ca_free(m);
}

void ca_mutex_lock(ca_mutex *m) {
        ca_assert(m);

        ca_assert_se(pthread_mutex_lock(&m->mutex) == 0);
}

ca_bool_t ca_mutex_try_lock(ca_mutex *m) {
        int r;
        ca_assert(m);

        if ((r = pthread_mutex_trylock(&m->mutex)) != 0) {
                ca_assert(r == EBUSY);
                return FALSE;
        }

        return TRUE;
}

void ca_mutex_unlock(ca_mutex *m) {
        ca_assert(m);

        ca_assert_se(pthread_mutex_unlock(&m->mutex) == 0);
}
