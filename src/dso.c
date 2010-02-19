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

#include <ltdl.h>
#include <string.h>
#include <errno.h>

#include "driver.h"
#include "common.h"
#include "malloc.h"
#include "driver-order.h"

struct private_dso {
        lt_dlhandle module;
        ca_bool_t ltdl_initialized;

        int (*driver_open)(ca_context *c);
        int (*driver_destroy)(ca_context *c);
        int (*driver_change_device)(ca_context *c, const char *device);
        int (*driver_change_props)(ca_context *c, ca_proplist *changed, ca_proplist *merged);
        int (*driver_play)(ca_context *c, uint32_t id, ca_proplist *p, ca_finish_callback_t cb, void *userdata);
        int (*driver_cancel)(ca_context *c, uint32_t id);
        int (*driver_cache)(ca_context *c, ca_proplist *p);
        int (*driver_playing)(ca_context *c, uint32_t id, int *playing);
};

#define PRIVATE_DSO(c) ((struct private_dso *) ((c)->private_dso))

static int ca_error_from_lt_error(int code) {

        static const int table[] = {
                [LT_ERROR_UNKNOWN] = CA_ERROR_INTERNAL,
                [LT_ERROR_DLOPEN_NOT_SUPPORTED] = CA_ERROR_NOTSUPPORTED,
                [LT_ERROR_INVALID_LOADER] = CA_ERROR_INTERNAL,
                [LT_ERROR_INIT_LOADER] = CA_ERROR_INTERNAL,
                [LT_ERROR_REMOVE_LOADER] = CA_ERROR_INTERNAL,
                [LT_ERROR_FILE_NOT_FOUND] = CA_ERROR_NOTFOUND,
                [LT_ERROR_DEPLIB_NOT_FOUND] = CA_ERROR_NOTFOUND,
                [LT_ERROR_NO_SYMBOLS] = CA_ERROR_NOTFOUND,
                [LT_ERROR_CANNOT_OPEN] = CA_ERROR_ACCESS,
                [LT_ERROR_CANNOT_CLOSE] = CA_ERROR_INTERNAL,
                [LT_ERROR_SYMBOL_NOT_FOUND] = CA_ERROR_NOTFOUND,
                [LT_ERROR_NO_MEMORY] = CA_ERROR_OOM,
                [LT_ERROR_INVALID_HANDLE] = CA_ERROR_INVALID,
                [LT_ERROR_BUFFER_OVERFLOW] = CA_ERROR_TOOBIG,
                [LT_ERROR_INVALID_ERRORCODE] = CA_ERROR_INVALID,
                [LT_ERROR_SHUTDOWN] = CA_ERROR_INTERNAL,
                [LT_ERROR_CLOSE_RESIDENT_MODULE] = CA_ERROR_INTERNAL,
                [LT_ERROR_INVALID_MUTEX_ARGS] = CA_ERROR_INTERNAL,
                [LT_ERROR_INVALID_POSITION] = CA_ERROR_INTERNAL
#ifdef LT_ERROR_CONFLICTING_FLAGS
                , [LT_ERROR_CONFLICTING_FLAGS] = CA_ERROR_INTERNAL
#endif
        };

        if (code < 0 || code >= (int) CA_ELEMENTSOF(table))
                return CA_ERROR_INTERNAL;

        return table[code];
}

static int lt_error_from_string(const char *t) {

        struct lt_error_code {
                int code;
                const char *text;
        };

        static const struct lt_error_code lt_error_codes[] = {
                /* This is so disgustingly ugly, it makes me vomit. But that's
                 * all ltdl's fault. */
#define LT_ERROR(u, s) { .code = LT_ERROR_ ## u, .text = s },
                lt_dlerror_table
#undef LT_ERROR

                { .code = 0, .text = NULL }
        };

        const struct lt_error_code *c;

        for (c = lt_error_codes; c->text; c++)
                if (ca_streq(t, c->text))
                        return c->code;

        return -1;
}

static int ca_error_from_string(const char *t) {
        int err;

        if ((err = lt_error_from_string(t)) < 0)
                return CA_ERROR_INTERNAL;

        return ca_error_from_lt_error(err);
}

static int try_open(ca_context *c, const char *t) {
        char *mn;
        struct private_dso *p;

        p = PRIVATE_DSO(c);

        if (!(mn = ca_sprintf_malloc(CA_PLUGIN_PATH "/libcanberra-%s", t)))
                return CA_ERROR_OOM;

        errno = 0;
        p->module = lt_dlopenext(mn);
        ca_free(mn);

        if (!p->module) {
                int ret;

                if (errno == ENOENT)
                        ret = CA_ERROR_NOTFOUND;
                else
                        ret = ca_error_from_string(lt_dlerror());

                if (ret == CA_ERROR_NOTFOUND)
                        ret = CA_ERROR_NODRIVER;

                return ret;
        }

        return CA_SUCCESS;
}

static void* real_dlsym(lt_module m, const char *name, const char *symbol) {
        char sn[256];
        char *s;
        void *r;

        ca_return_null_if_fail(m);
        ca_return_null_if_fail(name);
        ca_return_null_if_fail(symbol);

        snprintf(sn, sizeof(sn), "%s_%s", name, symbol);
        sn[sizeof(sn)-1] = 0;

        for (s = sn; *s; s++) {
                if (*s >= 'a' && *s <= 'z')
                        continue;
                if (*s >= 'A' && *s <= 'Z')
                        continue;
                if (*s >= '0' && *s <= '9')
                        continue;

                *s = '_';
        }

        if ((r = lt_dlsym(m, sn)))
                return r;

        return lt_dlsym(m, symbol);
}

#define MAKE_FUNC_PTR(ret, args, x) ((ret (*) args ) (size_t) (x))
#define GET_FUNC_PTR(module, name, symbol, ret, args) MAKE_FUNC_PTR(ret, args, real_dlsym((module), (name), (symbol)))

int driver_open(ca_context *c) {
        int ret;
        struct private_dso *p;
        char *driver;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(!PRIVATE_DSO(c), CA_ERROR_STATE);

        if (!(c->private_dso = p = ca_new0(struct private_dso, 1)))
                return CA_ERROR_OOM;

        if (lt_dlinit() != 0) {
                ret = ca_error_from_string(lt_dlerror());
                driver_destroy(c);
                return ret;
        }

        p->ltdl_initialized = TRUE;

        if (c->driver) {
                char *e;
                size_t n;

                if (!(e = ca_strdup(c->driver))) {
                        driver_destroy(c);
                        return CA_ERROR_OOM;
                }

                n = strcspn(e, ",:");
                e[n] = 0;

                if (n == 0) {
                        driver_destroy(c);
                        ca_free(e);
                        return CA_ERROR_INVALID;
                }

                if ((ret = try_open(c, e)) < 0) {
                        driver_destroy(c);
                        ca_free(e);
                        return ret;
                }

                driver = e;

        } else {
                const char *const * e;

                for (e = ca_driver_order; *e; e++) {

                        if ((ret = try_open(c, *e)) == CA_SUCCESS)
                                break;

                        if (ret != CA_ERROR_NODRIVER &&
                            ret != CA_ERROR_NOTAVAILABLE &&
                            ret != CA_ERROR_NOTFOUND) {

                                driver_destroy(c);
                                return ret;
                        }
                }

                if (!*e) {
                        driver_destroy(c);
                        return CA_ERROR_NODRIVER;
                }

                if (!(driver = ca_strdup(*e))) {
                        driver_destroy(c);
                        return CA_ERROR_OOM;
                }
        }

        ca_assert(p->module);

        if (!(p->driver_open = GET_FUNC_PTR(p->module, driver, "driver_open", int, (ca_context*))) ||
            !(p->driver_destroy = GET_FUNC_PTR(p->module, driver, "driver_destroy", int, (ca_context*))) ||
            !(p->driver_change_device = GET_FUNC_PTR(p->module, driver, "driver_change_device", int, (ca_context*, const char *))) ||
            !(p->driver_change_props = GET_FUNC_PTR(p->module, driver, "driver_change_props", int, (ca_context *, ca_proplist *, ca_proplist *))) ||
            !(p->driver_play = GET_FUNC_PTR(p->module, driver, "driver_play", int, (ca_context*, uint32_t, ca_proplist *, ca_finish_callback_t, void *))) ||
            !(p->driver_cancel = GET_FUNC_PTR(p->module, driver, "driver_cancel", int, (ca_context*, uint32_t))) ||
            !(p->driver_cache = GET_FUNC_PTR(p->module, driver, "driver_cache", int, (ca_context*, ca_proplist *))) ||
            !(p->driver_playing = GET_FUNC_PTR(p->module, driver, "driver_playing", int, (ca_context*, uint32_t, int*)))) {

                ca_free(driver);
                driver_destroy(c);
                return CA_ERROR_CORRUPT;
        }

        ca_free(driver);

        if ((ret = p->driver_open(c)) < 0) {
                p->driver_destroy = NULL;
                driver_destroy(c);
                return ret;
        }

        return CA_SUCCESS;
}

int driver_destroy(ca_context *c) {
        struct private_dso *p;
        int ret = CA_SUCCESS;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private_dso, CA_ERROR_STATE);

        p = PRIVATE_DSO(c);

        if (p->driver_destroy)
                ret = p->driver_destroy(c);

        if (p->module)
                lt_dlclose(p->module);

        if (p->ltdl_initialized) {
                lt_dlexit();
                p->ltdl_initialized = FALSE;
        }

        ca_free(p);

        c->private_dso = NULL;

        return ret;
}

int driver_change_device(ca_context *c, const char *device) {
        struct private_dso *p;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private_dso, CA_ERROR_STATE);

        p = PRIVATE_DSO(c);
        ca_return_val_if_fail(p->driver_change_device, CA_ERROR_STATE);

        return p->driver_change_device(c, device);
}

int driver_change_props(ca_context *c, ca_proplist *changed, ca_proplist *merged) {
        struct private_dso *p;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private_dso, CA_ERROR_STATE);

        p = PRIVATE_DSO(c);
        ca_return_val_if_fail(p->driver_change_props, CA_ERROR_STATE);

        return p->driver_change_props(c, changed, merged);
}

int driver_play(ca_context *c, uint32_t id, ca_proplist *pl, ca_finish_callback_t cb, void *userdata) {
        struct private_dso *p;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private_dso, CA_ERROR_STATE);

        p = PRIVATE_DSO(c);
        ca_return_val_if_fail(p->driver_play, CA_ERROR_STATE);

        return p->driver_play(c, id, pl, cb, userdata);
}

int driver_cancel(ca_context *c, uint32_t id) {
        struct private_dso *p;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private_dso, CA_ERROR_STATE);

        p = PRIVATE_DSO(c);
        ca_return_val_if_fail(p->driver_cancel, CA_ERROR_STATE);

        return p->driver_cancel(c, id);
}

int driver_cache(ca_context *c, ca_proplist *pl) {
        struct private_dso *p;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private_dso, CA_ERROR_STATE);

        p = PRIVATE_DSO(c);
        ca_return_val_if_fail(p->driver_cache, CA_ERROR_STATE);

        return p->driver_cache(c, pl);
}

int driver_playing(ca_context *c, uint32_t id, int *playing) {
        struct private_dso *p;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private_dso, CA_ERROR_STATE);
        ca_return_val_if_fail(playing, CA_ERROR_INVALID);

        p = PRIVATE_DSO(c);
        ca_return_val_if_fail(p->driver_playing, CA_ERROR_STATE);

        return p->driver_playing(c, id, playing);
}
