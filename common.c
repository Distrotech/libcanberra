/* $Id$ */

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
  License along with libcanberra. If not, If not, see
  <http://www.gnu.org/licenses/>.
***/

#include <stdarg.h>

#include "canberra.h"
#include "common.h"
#include "malloc.h"
#include "driver.h"
#include "proplist.h"

int ca_context_create(ca_context **_c) {
    ca_context *c;
    int ret;

    ca_return_val_if_fail(_c, CA_ERROR_INVALID);

    if (!(c = ca_new0(ca_context, 1)))
        return CA_ERROR_OOM;

    if (!(c->mutex = ca_mutex_new())) {
        ca_free(c);
        return CA_ERROR_OOM;
    }

    if ((ret = ca_proplist_create(&c->props)) < 0) {
        ca_mutex_free(c->mutex);
        ca_free(c);
        return ret;
    }

    *_c = c;
    return CA_SUCCESS;
}

int ca_context_destroy(ca_context *c) {
    int ret = CA_SUCCESS;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    /* There's no locking necessary here, because the application is
     * broken anyway if it destructs this object in one thread and
     * still is calling a method of it in another. */

    if (c->opened)
        ret = driver_destroy(c);

    if (c->props)
        ca_assert_se(ca_proplist_destroy(c->props) == CA_SUCCESS);

    ca_mutex_free(c->mutex);
    ca_free(c->driver);
    ca_free(c->device);
    ca_free(c);

    return ret;
}

int ca_context_set_driver(ca_context *c, char *driver) {
    char *n;
    int ret;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_mutex_lock(c->mutex);
    ca_return_val_if_fail_unlock(!c->opened, CA_ERROR_STATE, c->mutex);

    if (!(n = ca_strdup(driver))) {
        ret = CA_ERROR_OOM;
        goto fail;
    }

    ca_free(c->driver);
    c->driver = n;

    ret = CA_SUCCESS;

fail:
    ca_mutex_unlock(c->mutex);

    return ret;
}

int ca_context_change_device(ca_context *c, char *device) {
    char *n;
    int ret;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_mutex_lock(c->mutex);

    if (!(n = ca_strdup(device))) {
        ret = CA_ERROR_OOM;
        goto fail;
    }

    ret = c->opened ? driver_change_device(c, n) : CA_SUCCESS;

    if (ret == CA_SUCCESS) {
        ca_free(c->device);
        c->device = n;
    } else
        ca_free(n);

fail:
    ca_mutex_unlock(c->mutex);

    return ret;
}

static int context_open_unlocked(ca_context *c) {
    int ret;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    if (c->opened)
        return CA_SUCCESS;

    if ((ret = driver_open(c)) == CA_SUCCESS)
        c->opened = TRUE;

    return ret;
}

int ca_context_open(ca_context *c) {
    int ret;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_mutex_lock(c->mutex);
    ca_return_val_if_fail_unlock(!c->opened, CA_ERROR_STATE, c->mutex);

    ret = context_open_unlocked(c);

    ca_mutex_unlock(c->mutex);

    return ret;
}

static int ca_proplist_from_ap(ca_proplist **_p, va_list ap) {
    int ret;
    ca_proplist *p;

    ca_assert(_p);

    if ((ret = ca_proplist_create(&p)) < 0)
        return ret;

    for (;;) {
        const char *key, *value;
        int ret;

        if (!(key = va_arg(ap, const char*)))
            break;

        if (!(value = va_arg(ap, const char*))) {
            ret = CA_ERROR_INVALID;
            goto fail;
        }

        if ((ret = ca_proplist_sets(p, key, value)) < 0)
            goto fail;
    }

    *_p = p;

    return CA_SUCCESS;

fail:
    ca_assert_se(ca_proplist_destroy(p) == CA_SUCCESS);

    return ret;
}

int ca_context_change_props(ca_context *c, ...)  {
    va_list ap;
    int ret;
    ca_proplist *p = NULL;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    va_start(ap, c);
    ret = ca_proplist_from_ap(&p, ap);
    va_end(ap);

    if (ret < 0)
        return ret;

    ret = ca_context_change_props_full(c, p);

    ca_assert_se(ca_proplist_destroy(p) == 0);

    return ret;
}

int ca_context_change_props_full(ca_context *c, ca_proplist *p) {
    int ret;
    ca_proplist *merged;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(p, CA_ERROR_INVALID);

    ca_mutex_lock(c->mutex);

    if ((ret = ca_proplist_merge(&merged, c->props, p)) < 0)
        goto finish;

    ret = c->opened ? driver_change_props(c, p, merged) : CA_SUCCESS;

    if (ret == CA_SUCCESS) {
        ca_assert_se(ca_proplist_destroy(c->props) == CA_SUCCESS);
        c->props = merged;
    } else
        ca_assert_se(ca_proplist_destroy(merged) == CA_SUCCESS);

finish:

    ca_mutex_unlock(c->mutex);

    return ret;
}

int ca_context_play(ca_context *c, uint32_t id, ...) {
    int ret;
    va_list ap;
    ca_proplist *p = NULL;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    va_start(ap, id);
    ret = ca_proplist_from_ap(&p, ap);
    va_end(ap);

    if (ret < 0)
        return ret;

    ret = ca_context_play_full(c, id, p, NULL, NULL);

    ca_assert_se(ca_proplist_destroy(p) == 0);

    return ret;
}

int ca_context_play_full(ca_context *c, uint32_t id, ca_proplist *p, ca_finish_callback_t cb, void *userdata) {
    int ret;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(p, CA_ERROR_INVALID);
    ca_return_val_if_fail(!userdata || cb, CA_ERROR_INVALID);

    ca_mutex_lock(c->mutex);

    ca_return_val_if_fail_unlock(ca_proplist_contains(p, CA_PROP_EVENT_ID) ||
                                 ca_proplist_contains(c->props, CA_PROP_EVENT_ID), CA_ERROR_INVALID, c->mutex);

    if ((ret = context_open_unlocked(c)) < 0)
        goto finish;

    ca_assert(c->opened);

    ret = driver_play(c, id, p, cb, userdata);

finish:

    ca_mutex_unlock(c->mutex);

    return ret;
}

int ca_context_cancel(ca_context *c, uint32_t id)  {
    int ret;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_mutex_lock(c->mutex);
    ca_return_val_if_fail_unlock(c->opened, CA_ERROR_STATE, c->mutex);

    ret = driver_cancel(c, id);

    ca_mutex_unlock(c->mutex);

    return ret;
}

int ca_context_cache(ca_context *c, ...) {
    int ret;
    va_list ap;
    ca_proplist *p = NULL;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    va_start(ap, c);
    ret = ca_proplist_from_ap(&p, ap);
    va_end(ap);

    if (ret < 0)
        return ret;

    ret = ca_context_cache_full(c, p);

    ca_assert_se(ca_proplist_destroy(p) == 0);

    return ret;
}

int ca_context_cache_full(ca_context *c, ca_proplist *p) {
    int ret;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(p, CA_ERROR_INVALID);

    ca_mutex_lock(c->mutex);

    ca_return_val_if_fail_unlock(ca_proplist_contains(p, CA_PROP_EVENT_ID) ||
                                 ca_proplist_contains(c->props, CA_PROP_EVENT_ID), CA_ERROR_INVALID, c->mutex);

    if ((ret = context_open_unlocked(c)) < 0)
        goto finish;

    ca_assert(c->opened);

    ret = driver_cache(c, p);

finish:

    ca_mutex_unlock(c->mutex);

    return ret;
}

/** Return a human readable error */
const char *ca_strerror(int code) {

    const char * const error_table[-_CA_ERROR_MAX] = {
        [-CA_SUCCESS] = "Success",
        [-CA_ERROR_NOT_SUPPORTED] = "Operation not supported",
        [-CA_ERROR_INVALID] = "Invalid argument",
        [-CA_ERROR_STATE] = "Invalid state",
        [-CA_ERROR_OOM] = "Out of memory",
        [-CA_ERROR_NO_DRIVER] = "No such driver",
        [-CA_ERROR_SYSTEM] = "System error"
    };

    ca_return_val_if_fail(code <= 0, NULL);
    ca_return_val_if_fail(code > _CA_ERROR_MAX, NULL);

    return error_table[-code];
}

/* Not exported */
int ca_parse_cache_control(ca_cache_control_t *control, const char *c) {
    ca_return_val_if_fail(control, CA_ERROR_INVALID);
    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    if (streq(control, "never"))
        *control = CA_CACHE_CONTROL_NEVER;
    else if (streq(control, "permanent"))
        *control = CA_CACHE_CONTROL_PERMANENT;
    else if (streq(control, "volatile"))
        *control = CA_CACHE_CONTROL_VOLATILE;
    else
        return CA_ERROR_INVALID;

    return CA_SUCCESS;
}
