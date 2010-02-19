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

#include "driver.h"
#include "llist.h"
#include "malloc.h"
#include "common.h"
#include "driver-order.h"

struct backend {
        CA_LLIST_FIELDS(struct backend);
        ca_context *context;
};

struct private {
        ca_context *context;
        CA_LLIST_HEAD(struct backend, backends);
};

#define PRIVATE(c) ((struct private *) ((c)->private))

static int add_backend(struct private *p, const char *name) {
        struct backend *b, *last;
        int ret;

        ca_assert(p);
        ca_assert(name);

        if (ca_streq(name, "multi"))
                return CA_ERROR_NOTAVAILABLE;

        for (b = p->backends; b; b = b->next)
                if (ca_streq(b->context->driver, name))
                        return CA_ERROR_NOTAVAILABLE;

        if (!(b = ca_new0(struct backend, 1)))
                return CA_ERROR_OOM;

        if ((ret = ca_context_create(&b->context)) < 0)
                goto fail;

        if ((ret = ca_context_change_props_full(b->context, p->context->props)) < 0)
                goto fail;

        if ((ret = ca_context_set_driver(b->context, name)) < 0)
                goto fail;

        if ((ret = ca_context_open(b->context)) < 0)
                goto fail;

        for (last = p->backends; last; last = last->next)
                if (!last->next)
                        break;

        CA_LLIST_INSERT_AFTER(struct backend, p->backends, last, b);

        return CA_SUCCESS;

fail:

        if (b->context)
                ca_context_destroy(b->context);

        ca_free(b);

        return ret;
}

static int remove_backend(struct private *p, struct backend *b) {
        int ret;

        ca_assert(p);
        ca_assert(b);

        ret = ca_context_destroy(b->context);
        CA_LLIST_REMOVE(struct backend, p->backends, b);
        ca_free(b);

        return ret;
}

int driver_open(ca_context *c) {
        struct private *p;
        int ret = CA_SUCCESS;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->driver, CA_ERROR_NODRIVER);
        ca_return_val_if_fail(!strncmp(c->driver, "multi", 5), CA_ERROR_NODRIVER);
        ca_return_val_if_fail(!PRIVATE(c), CA_ERROR_STATE);

        if (!(c->private = p = ca_new0(struct private, 1)))
                return CA_ERROR_OOM;

        p->context = c;

        if (c->driver) {
                char *e, *k;

                if (!(e = ca_strdup(c->driver))) {
                        driver_destroy(c);
                        return CA_ERROR_OOM;
                }

                k = e;
                for (;;)  {
                        size_t n;
                        ca_bool_t last;

                        n = strcspn(k, ",:");
                        last = k[n] == 0;
                        k[n] = 0;

                        if (n > 0) {
                                int r;

                                r = add_backend(p, k);

                                if (ret == CA_SUCCESS)
                                        ret = r;
                        }

                        if (last)
                                break;

                        k += n+1 ;
                }

                ca_free(e);

        } else {

                const char *const *e;

                for (e = ca_driver_order; *e; e++) {
                        int r;

                        r = add_backend(p, *e);

                        /* We return the error code of the first module that fails only */
                        if (ret == CA_SUCCESS)
                                ret = r;
                }
        }

        if (!p->backends) {
                driver_destroy(c);
                return ret == CA_SUCCESS ? CA_ERROR_NODRIVER : ret;
        }

        return CA_SUCCESS;
}


int driver_destroy(ca_context *c) {
        int ret = CA_SUCCESS;
        struct private *p;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private, CA_ERROR_STATE);

        p = PRIVATE(c);

        while (p->backends) {
                int r;

                r = remove_backend(p, p->backends);

                if (ret == CA_SUCCESS)
                        ret = r;
        }

        ca_free(p);

        c->private = NULL;

        return ret;
}

int driver_change_device(ca_context *c, const char *device) {
        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private, CA_ERROR_STATE);

        return CA_ERROR_NOTSUPPORTED;
}

int driver_change_props(ca_context *c, ca_proplist *changed, ca_proplist *merged) {
        int ret = CA_SUCCESS;
        struct private *p;
        struct backend *b;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(changed, CA_ERROR_INVALID);
        ca_return_val_if_fail(merged, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private, CA_ERROR_STATE);

        p = PRIVATE(c);

        for (b = p->backends; b; b = b->next) {
                int r;

                r = ca_context_change_props_full(b->context, changed);

                /* We only return the first failure */
                if (ret == CA_SUCCESS)
                        ret = r;
        }

        return ret;
}

struct closure {
        ca_context *context;
        ca_finish_callback_t callback;
        void *userdata;
};

static void call_closure(ca_context *c, uint32_t id, int error_code, void *userdata) {
        struct closure *closure = userdata;

        closure->callback(closure->context, id, error_code, closure->userdata);
        ca_free(closure);
}

int driver_play(ca_context *c, uint32_t id, ca_proplist *proplist, ca_finish_callback_t cb, void *userdata) {
        int ret = CA_SUCCESS;
        struct private *p;
        struct backend *b;
        struct closure *closure;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(proplist, CA_ERROR_INVALID);
        ca_return_val_if_fail(!userdata || cb, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private, CA_ERROR_STATE);

        p = PRIVATE(c);

        if (cb) {
                if (!(closure = ca_new(struct closure, 1)))
                        return CA_ERROR_OOM;

                closure->context = c;
                closure->callback = cb;
                closure->userdata = userdata;
        } else
                closure = NULL;

        /* The first backend that can play this, takes it */
        for (b = p->backends; b; b = b->next) {
                int r;

                if ((r = ca_context_play_full(b->context, id, proplist, closure ? call_closure : NULL, closure)) == CA_SUCCESS)
                        return r;

                /* We only return the first failure */
                if (ret == CA_SUCCESS)
                        ret = r;
        }

        ca_free(closure);

        return ret;
}

int driver_cancel(ca_context *c, uint32_t id) {
        int ret = CA_SUCCESS;
        struct private *p;
        struct backend *b;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private, CA_ERROR_STATE);

        p = PRIVATE(c);

        for (b = p->backends; b; b = b->next) {
                int r;

                r = ca_context_cancel(b->context, id);

                /* We only return the first failure */
                if (ret == CA_SUCCESS)
                        ret = r;
        }

        return ret;
}

int driver_cache(ca_context *c, ca_proplist *proplist) {
        int ret = CA_SUCCESS;
        struct private *p;
        struct backend *b;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(proplist, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private, CA_ERROR_STATE);

        p = PRIVATE(c);

        /* The first backend that can cache this, takes it */
        for (b = p->backends; b; b = b->next) {
                int r;

                if ((r = ca_context_cache_full(b->context,  proplist)) == CA_SUCCESS)
                        return r;

                /* We only return the first failure */
                if (ret == CA_SUCCESS)
                        ret = r;
        }

        return ret;
}

int driver_playing(ca_context *c, uint32_t id, int *playing) {
        int ret = CA_SUCCESS;
        struct private *p;
        struct backend *b;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(playing, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private, CA_ERROR_STATE);

        p = PRIVATE(c);

        *playing = 0;

        for (b = p->backends; b; b = b->next) {
                int r, _playing = 0;

                r = ca_context_playing(b->context, id, &_playing);

                /* We only return the first failure */
                if (ret == CA_SUCCESS)
                        ret = r;

                if (_playing)
                        *playing = 1;
        }

        return ret;
}
