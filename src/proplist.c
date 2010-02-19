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

#include <stdarg.h>

#include "canberra.h"
#include "proplist.h"
#include "macro.h"
#include "malloc.h"

static unsigned calc_hash(const char *c) {
        unsigned hash = 0;

        for (; *c; c++)
                hash = 31 * hash + (unsigned) *c;

        return hash;
}

/**
 * ca_proplist_create:
 * @p: A pointer where to fill in a pointer for the new property list.
 *
 * Allocate a new empty property list.
 *
 * Returns: 0 on success, negative error code on error.
 */
int ca_proplist_create(ca_proplist **_p) {
        ca_proplist *p;
        ca_return_val_if_fail(_p, CA_ERROR_INVALID);

        if (!(p = ca_new0(ca_proplist, 1)))
                return CA_ERROR_OOM;

        if (!(p->mutex = ca_mutex_new())) {
                ca_free(p);
                return CA_ERROR_OOM;
        }

        *_p = p;

        return CA_SUCCESS;
}

static int _unset(ca_proplist *p, const char *key) {
        ca_prop *prop, *nprop;
        unsigned i;

        ca_return_val_if_fail(p, CA_ERROR_INVALID);
        ca_return_val_if_fail(key, CA_ERROR_INVALID);

        i = calc_hash(key) % N_HASHTABLE;

        nprop = NULL;
        for (prop = p->prop_hashtable[i]; prop; nprop = prop, prop = prop->next_in_slot)
                if (strcmp(prop->key, key) == 0)
                        break;

        if (prop) {
                if (nprop)
                        nprop->next_in_slot = prop->next_in_slot;
                else
                        p->prop_hashtable[i] = prop->next_in_slot;

                if (prop->prev_item)
                        prop->prev_item->next_item = prop->next_item;
                else
                        p->first_item = prop->next_item;

                if (prop->next_item)
                        prop->next_item->prev_item = prop->prev_item;

                ca_free(prop->key);
                ca_free(prop);
        }

        return CA_SUCCESS;
}

/**
 * ca_proplist_sets:
 * @p: The property list to add this key/value pair to
 * @key: The key for this key/value pair
 * @value: The value for this key/value pair
 *
 * Add a new string key/value pair to the property list.
 *
 * Returns: 0 on success, negative error code on error.
 */

int ca_proplist_sets(ca_proplist *p, const char *key, const char *value) {
        ca_return_val_if_fail(p, CA_ERROR_INVALID);
        ca_return_val_if_fail(key, CA_ERROR_INVALID);
        ca_return_val_if_fail(value, CA_ERROR_INVALID);

        return ca_proplist_set(p, key, value, strlen(value)+1);
}

/**
 * ca_proplist_setf:
 * @p: The property list to add this key/value pair to
 * @key: The key for this key/value pair
 * @format: The format string for the value for this key/value pair
 * @...: The parameters for the format string
 *
 * Much like ca_proplist_sets(): add a new string key/value pair to
 * the property list. Takes a standard C format string plus arguments
 * and formats a string of it.
 *
 * Returns: 0 on success, negative error code on error.
 */

int ca_proplist_setf(ca_proplist *p, const char *key, const char *format, ...) {
        int ret;
        char *k;
        ca_prop *prop;
        size_t size = 100;
        unsigned h;

        ca_return_val_if_fail(p, CA_ERROR_INVALID);
        ca_return_val_if_fail(key, CA_ERROR_INVALID);
        ca_return_val_if_fail(format, CA_ERROR_INVALID);

        if (!(k = ca_strdup(key)))
                return CA_ERROR_OOM;

        for (;;) {
                va_list ap;
                int r;

                if (!(prop = ca_malloc(CA_ALIGN(sizeof(ca_prop)) + size))) {
                        ca_free(k);
                        return CA_ERROR_OOM;
                }


                va_start(ap, format);
                r = vsnprintf(CA_PROP_DATA(prop), size, format, ap);
                va_end(ap);

                ((char*) CA_PROP_DATA(prop))[size-1] = 0;

                if (r > -1 && (size_t) r < size) {
                        prop->nbytes = (size_t) r+1;
                        break;
                }

                if (r > -1)    /* glibc 2.1 */
                        size = (size_t) r+1;
                else           /* glibc 2.0 */
                        size *= 2;

                ca_free(prop);
        }

        prop->key = k;

        ca_mutex_lock(p->mutex);

        if ((ret = _unset(p, key)) < 0) {
                ca_free(prop);
                ca_free(k);
                goto finish;
        }

        h = calc_hash(key) % N_HASHTABLE;

        prop->next_in_slot = p->prop_hashtable[h];
        p->prop_hashtable[h] = prop;

        prop->prev_item = NULL;
        if ((prop->next_item = p->first_item))
                prop->next_item->prev_item = prop;
        p->first_item = prop;

finish:

        ca_mutex_unlock(p->mutex);

        return ret;
}

/**
 * ca_proplist_set:
 * @p: The property list to add this key/value pair to
 * @key: The key for this key/value pair
 * @data: The binary value for this key value pair
 * @nbytes: The size of thebinary value for this key value pair.
 *
 * Add a new binary key/value pair to the property list.
 *
 * Returns: 0 on success, negative error code on error.
 */

int ca_proplist_set(ca_proplist *p, const char *key, const void *data, size_t nbytes) {
        int ret;
        char *k;
        ca_prop *prop;
        unsigned h;

        ca_return_val_if_fail(p, CA_ERROR_INVALID);
        ca_return_val_if_fail(key, CA_ERROR_INVALID);
        ca_return_val_if_fail(!nbytes || data, CA_ERROR_INVALID);

        if (!(k = ca_strdup(key)))
                return CA_ERROR_OOM;

        if (!(prop = ca_malloc(CA_ALIGN(sizeof(ca_prop)) + nbytes))) {
                ca_free(k);
                return CA_ERROR_OOM;
        }

        prop->key = k;
        prop->nbytes = nbytes;
        memcpy(CA_PROP_DATA(prop), data, nbytes);

        ca_mutex_lock(p->mutex);

        if ((ret = _unset(p, key)) < 0) {
                ca_free(prop);
                ca_free(k);
                goto finish;
        }

        h = calc_hash(key) % N_HASHTABLE;

        prop->next_in_slot = p->prop_hashtable[h];
        p->prop_hashtable[h] = prop;

        prop->prev_item = NULL;
        if ((prop->next_item = p->first_item))
                prop->next_item->prev_item = prop;
        p->first_item = prop;

finish:

        ca_mutex_unlock(p->mutex);

        return ret;
}

/* Not exported, not self-locking */
ca_prop* ca_proplist_get_unlocked(ca_proplist *p, const char *key) {
        ca_prop *prop;
        unsigned i;

        ca_return_val_if_fail(p, NULL);
        ca_return_val_if_fail(key, NULL);

        i = calc_hash(key) % N_HASHTABLE;

        for (prop = p->prop_hashtable[i]; prop; prop = prop->next_in_slot)
                if (strcmp(prop->key, key) == 0)
                        return prop;

        return NULL;
}

/* Not exported, not self-locking */
const char* ca_proplist_gets_unlocked(ca_proplist *p, const char *key) {
        ca_prop *prop;

        ca_return_val_if_fail(p, NULL);
        ca_return_val_if_fail(key, NULL);

        if (!(prop = ca_proplist_get_unlocked(p, key)))
                return NULL;

        if (!memchr(CA_PROP_DATA(prop), 0, prop->nbytes))
                return NULL;

        return CA_PROP_DATA(prop);
}

/**
 * ca_proplist_destroy:
 * @p: The property list to destroy
 *
 * Destroys a property list that was created with ca_proplist_create() earlier.
 *
 * Returns: 0 on success, negative error code on error.
 */

int ca_proplist_destroy(ca_proplist *p) {
        ca_prop *prop, *nprop;

        ca_return_val_if_fail(p, CA_ERROR_INVALID);

        for (prop = p->first_item; prop; prop = nprop) {
                nprop = prop->next_item;
                ca_free(prop->key);
                ca_free(prop);
        }

        ca_mutex_free(p->mutex);

        ca_free(p);

        return CA_SUCCESS;
}

static int merge_into(ca_proplist *a, ca_proplist *b) {
        int ret = CA_SUCCESS;
        ca_prop *prop;

        ca_return_val_if_fail(a, CA_ERROR_INVALID);
        ca_return_val_if_fail(b, CA_ERROR_INVALID);

        ca_mutex_lock(b->mutex);

        for (prop = b->first_item; prop; prop = prop->next_item)
                if ((ret = ca_proplist_set(a, prop->key, CA_PROP_DATA(prop), prop->nbytes)) < 0)
                        break;

        ca_mutex_unlock(b->mutex);

        return ret;
}

int ca_proplist_merge(ca_proplist **_a, ca_proplist *b, ca_proplist *c) {
        ca_proplist *a;
        int ret;

        ca_return_val_if_fail(_a, CA_ERROR_INVALID);
        ca_return_val_if_fail(b, CA_ERROR_INVALID);
        ca_return_val_if_fail(c, CA_ERROR_INVALID);

        if ((ret = ca_proplist_create(&a)) < 0)
                return ret;

        if ((ret = merge_into(a, b)) < 0 ||
            (ret = merge_into(a, c)) < 0) {
                ca_proplist_destroy(a);
                return ret;
        }

        *_a = a;
        return CA_SUCCESS;
}

ca_bool_t ca_proplist_contains(ca_proplist *p, const char *key) {
        ca_bool_t b;

        ca_return_val_if_fail(p, FALSE);
        ca_return_val_if_fail(key, FALSE);

        ca_mutex_lock(p->mutex);
        b = !!ca_proplist_get_unlocked(p, key);
        ca_mutex_unlock(p->mutex);

        return b;
}

int ca_proplist_merge_ap(ca_proplist *p, va_list ap) {
        int ret;

        ca_return_val_if_fail(p, CA_ERROR_INVALID);

        for (;;) {
                const char *key, *value;

                if (!(key = va_arg(ap, const char*)))
                        break;

                if (!(value = va_arg(ap, const char*)))
                        return CA_ERROR_INVALID;

                if ((ret = ca_proplist_sets(p, key, value)) < 0)
                        return ret;
        }

        return CA_SUCCESS;
}

int ca_proplist_from_ap(ca_proplist **_p, va_list ap) {
        int ret;
        ca_proplist *p;

        ca_return_val_if_fail(_p, CA_ERROR_INVALID);

        if ((ret = ca_proplist_create(&p)) < 0)
                return ret;

        if ((ret = ca_proplist_merge_ap(p, ap)) < 0)
                goto fail;

        *_p = p;

        return CA_SUCCESS;

fail:
        ca_assert_se(ca_proplist_destroy(p) == CA_SUCCESS);

        return ret;
}
