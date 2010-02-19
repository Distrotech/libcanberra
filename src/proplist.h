/*-*- Mode: C; c-basic-offset: 8 -*-*/

#ifndef foocanberraproplisthfoo
#define foocanberraproplisthfoo

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

#include <stdarg.h>

#include "canberra.h"
#include "mutex.h"

#define N_HASHTABLE 31

typedef struct ca_prop {
        char *key;
        size_t nbytes;
        struct ca_prop *next_in_slot, *next_item, *prev_item;
} ca_prop;

#define CA_PROP_DATA(p) ((void*) ((char*) (p) + CA_ALIGN(sizeof(ca_prop))))

struct ca_proplist {
        ca_mutex *mutex;

        ca_prop *prop_hashtable[N_HASHTABLE];
        ca_prop *first_item;
};

int ca_proplist_merge(ca_proplist **_a, ca_proplist *b, ca_proplist *c);
ca_bool_t ca_proplist_contains(ca_proplist *p, const char *key);

/* Both of the following two functions are not locked! Need manual locking! */
ca_prop* ca_proplist_get_unlocked(ca_proplist *p, const char *key);
const char* ca_proplist_gets_unlocked(ca_proplist *p, const char *key);

int ca_proplist_merge_ap(ca_proplist *p, va_list ap);
int ca_proplist_from_ap(ca_proplist **_p, va_list ap);

#endif
