#include "canberra.h"
#include "common.h"

int ca_context_create(ca_context_t **c) {
    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    if (!(*c = ca_new0(ca_context_t, 1)))
        return CA_ERROR_OOM;

    return CA_SUCCESS;
}

int ca_context_destroy(ca_context_t *c) {
    int ret;
    unsigned i;
    ca_prop *p, *n;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    ret = driver_destroy(c);

    for (p = c->first_item; p; p = n) {
        n = p->next_item;
        ca_free(p);
    }

    ca_free(c);

    return ret;
}

int ca_context_open(ca_context_t *c) {
    int ret;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(!c->opened, CA_ERROR_STATE);

    if ((ret = driver_open(c)) == 0)
        c->opened = TRUE;

    return ret;
}

int ca_context_sets(ca_context_t *c, ...)  {
    va_list ap;
    int ret = CA_SUCCESS;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    va_start(ap, c);

    for (;;) {
        const char *key, *value;
        int ret;

        if (!(key = v_arg(ap, const char*)))
            break;

        if (!(value = v_arg(ap, const char *))) {
            ret = CA_ERROR_INVALID;
            break;
        }

        if ((ret = ca_context_set(c, key, value, strlen(value)+1)) < 0)
            break;
    }

    va_end(ap);

    return ret;
}

static int _unset(ca_context_t *c, const char *key) {
    ca_prop *p, *np;
    unsigned i;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(key, CA_ERROR_INVALID);

    i = calc_hash(key) % N_HASHTABLE;

    np = NULL;
    for (p = c->props[i]; p; np = p, p = p->next_in_slot)
        if (strcmp(p->key, key) == 0)
            break;

    if (p) {
        if (np)
            np->next_in_slot = p->next_in_slot;
        else
            c->props[i] = p->next_in_slot;

        if (p->prev_item)
            p->prev_item->next_item = p->next_item;
        else
            c->first_item = p->next_item;

        if (p->next_item)
            p->next_item->prev_item = p->prev_item;

        ca_free(p);
    }
}

int ca_context_set(ca_context_t *c, const char *key, const void *data, size_t nbytes) {
    int ret;
    ca_prop *p;
    char *k;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(key, CA_ERROR_INVALID);
    ca_return_val_if_fail(!nbytes || data, CA_ERROR_INVALID);

    if (!(k = ca_strdup(key)))
        return CA_ERROR_OOM;

    if (!(p = ca_malloc(CA_ALIGN(sizeof(ca_prop)) + nbytes))) {
        ca_free(k);
        return CA_ERROR_OOM;
    }

    if ((ret = _unset(c, key)) < 0) {
        ca_free(p);
        ca_free(k);
        return ret;
    }

    p->key = k;
    p->nbytes = nbytes;
    memcpy(CA_PROP_DATA(p), data, nbytes);

    i = calc_hash(key) % N_HASHTABLE;

    p->next_in_slot = c->props[i];
    c->props[i] = p;

    p->prev_item = NULL;
    p->next_item = c->first_item;
    c->first_item = p;

    if (c->opened)
        if ((ret = driver_set(c, key, data, nbytes)) < 0)
            return ret;

    return CA_SUCCESS;
}

int ca_context_unset(ca_context *c, ...) {
    int ret = CA_SUCCESS;
    va_list ap;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(key, CA_ERROR_INVALID);

    va_start(ap, c);

    for (;;) {
        const char *key;

        if (!(key = v_arg(ap, const char*)))
            break;

        if (c->opened) {
            if ((ret = driver_unset(c, key)) < 0)
                break;
        }

        if ((ret = _unset(c, key)) < 0)
            break;
    }

    va_end(ap);

    return ret;
}

/* Not exported */
ca_prop* ca_context_get(ca_context *c, const char *key) {
    ca_prop *p;
    unsigned i;

    ca_return_val_if_fail(c, NULL);
    ca_return_val_if_fail(key, NULL);

    i = calc_hash(key) % N_HASHTABLE;

    for (p = c->props[i]; p; p = p->next_in_slot)
        if (strcmp(p->key, key) == 0)
            return p;

    return NULL;
}

/* Not exported */
const char* ca_context_gets(ca_context *c, const char *key) {
    ca_prop *p;

    ca_return_val_if_fail(c, NULL);
    ca_return_val_if_fail(key, NULL);

    if (!(p = ca_context_get(c, key)))
        return NULL;

    if (memchr(CA_PROP_DATA(p), 0, p->nbytes))
        return CA_PROP_DATA(p);

    return NULL;
}

int ca_context_play(ca_context_t *c, uint32_t id, ...) {
    int ret;
    va_list ap;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(key, CA_ERROR_INVALID);

    if (!c->opened)
        if ((ret = ca_context_open(c)) < 0)
            return ret;

    ca_assert(c->opened);

    /* make sure event.id is set */

    va_start(ap, c);
    for (;;) {
        const char *key, *value;

        if (!(key = va_arg(ap, const char *)))
            break;

        if (!(value = va_arg(ap, const char *))) {
            va_end(ap);
            return CA_ERROR_INVALID;
        }

        found = found || strcmp(key, CA_PROP_EVENT_ID) == 0;
    }
    va_end(ap);

    found = found || ca_context_gets(c, CA_PROP_EVENT_ID);

    if (!found)
        return CA_ERROR_INVALID;

    va_start(ap, id);
    ret = driver_play(c, id, ap);
    va_end(ap);

    return ret;
}

int ca_context_cancel(ca_context_t *c, uint32_ id)  {

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(c->opened, CA_ERROR_STATE);

    return driver_cancel(c, id);
}

int ca_context_cache(ca_context_t *c, ...) {
    int ret;
    va_list ap;
    ca_bool_t found = FALSE;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    if (!c->opened)
        if ((ret = ca_context_open(c)) < 0)
            return ret;

    ca_assert(c->opened);

    /* make sure event.id is set */

    va_start(ap, c);
    for (;;) {
        const char *key, *value;

        if (!(key = va_arg(ap, const char *)))
            break;

        if (!(value = va_arg(ap, const char *))) {
            va_end(ap);
            return CA_ERROR_INVALID;
        }

        found = found || strcmp(key, CA_PROP_EVENT_ID) == 0;
    }
    va_end(ap);

    found = found || ca_context_gets(c, CA_PROP_EVENT_ID);

    if (!found)
        return CA_ERROR_INVALID;

    va_start(ap, c);
    ret = driver_cache(c, ap);
    va_end(ap);

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
    ca_return_val_if_fail(code > _SA_ERROR_MAX, NULL);

    return error_table[-code];
}

#endif
