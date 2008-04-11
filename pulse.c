#include <pulse/thread-mainloop.h>

#include "canberra.h"
#include "common."
#include "driver.h"

struct private {
    pa_threaded_mainloop *mainloop;
    pa_context *context;
};

#define PRIVATE(c) ((struct private *) ((c)->private)

int driver_open(ca_context *c) {
    pa_proplist *l;
    struct private *p;
    ca_prop *i;
    ca_return_val_if_fail(c, PA_ERROR_INVALID);

    if (!(p = PRIVATE(c) = ca_new0(struct private, 1)))
        return PA_ERROR_OOM;

    if (!(p->mainloop = pa_threaded_mainloop_new())) {
        driver_destroy(c);
        return PA_ERROR_OOM;
    }

    l = pa_proplist_new();

    for (i = c->first_item; i; i = i->next_item)
        if (pa_proplist_put(l, i->key, CA_PROP_DATA(i), i->nbytes) < 0) {
            driver_destroy(c);
            pa_proplist_free(l);
            return PA_ERROR_INVALID;
        }

    if (!(p->context = pa_context_new_with_proplist(pa_threaded_mainloop_get_api(p->mainloop), l))) {
        pa_proplist_free(l);
        driver_destroy(c);
        return PA_ERROR_OOM;
    }

    pa_proplist_free(l);

    pa_context_set_state_callback(p->context, context_state_cb, c);

    if (pa_context_connect(p->context, NULL, 0, NULL) < 0) {
        int ret = translate_error(pa_context_errno(p->context));
        driver_destroy(c);
        return ret;
    }

    pa_threaded_mainloop_lock(p->mainloop);

    if (pa_threaded_mainloop_start(p->mainloop) < 0) {
        pa_threaded_mainloop_unlock(p->mainloop);
        driver_destroy(c);
        return PA_ERROR_INTERNAL;
    }

    pa_threaded_mainloop_wait(p->mainloop);

    if (pa_context_get_state(p->context) != PA_CONTEXT_READY) {
        int ret = translate_error(pa_context_errno(p->context));
        pa_threaded_mainloop_unlock(p->mainloop);
        driver_destroy(c);
        return ret;
    }

    return PA_SUCCESS;
}

int driver_destroy(ca_context *c) {
    ca_return_val_if_fail(c, PA_ERROR_INVALID);

    p = PRIVATE(c);

    if (p->mainloop)
        pa_threaded_mainloop_stop(p->mainloop);

    if (p->context) {
        pa_context_disconnect(p->context);
        pa_context_unref(p->context);
    }

    if (p->mainloop)
        pa_threaded_mainloop_free(p->mainloop);

    ca_free(p);
}

int driver_prop_put(ca_context *c, const char *key, const void* data, size_t nbytes) {
    struct private *p;
    pa_operation *o;
    pa_proplist *l;
    int ret = CA_SUCCESS;

    ca_return_val_if_fail(c, PA_ERROR_INVALID);
    ca_return_val_if_fail(key, PA_ERROR_INVALID);

    p = PRIVATE(c);

    ca_return_val_if_fail(p->mainloop, PA_ERROR_STATE);
    ca_return_val_if_fail(p->context, PA_ERROR_STATE);

    l = pa_proplist_new();

    if (pa_proplist_put(l, key, data, nbytes) < 0) {
        pa_proplist_free(l);
        return PA_ERROR_INVALID;
    }

    pa_threaded_mainloop_lock(p->mainloop);

    /* We start these asynchronously and don't care about the return
     * value */

    if (!(o = pa_context_proplist_update(p->context, PA_UPDATE_REPLACE, l, NULL, NULL)))
        ret = translate_error(pa_context_errno(p->context));
    else
        pa_operation_unref(o);

    pa_threaded_mainloop_unlock(p->mainloop);

    return ret;
}

int driver_prop_unset(ca_context *c, const char *key) {
    struct private *p;
    pa_operation *o;
    const char *a[2];
    int ret = CA_SUCCESS;

    ca_return_val_if_fail(c, PA_ERROR_INVALID);
    ca_return_val_if_fail(key, PA_ERROR_INVALID);

    p = PRIVATE(c);

    ca_return_val_if_fail(p->mainloop, PA_ERROR_STATE);
    ca_return_val_if_fail(p->context, PA_ERROR_STATE);

    a[0] = key;
    a[1] = NULL;

    pa_threaded_mainloop_lock(p->mainloop);

    /* We start these asynchronously and don't care about the return
     * value */

    if (!(o = pa_context_proplist_remove(p->context, a, NULL, NULL)))
        ret = translate_error(pa_context_errno(p->context));
    else
        pa_operation_unref(o);

    pa_threaded_mainloop_unlock(p->mainloop);

    return ret;
}

static pa_proplist* proplist_unroll(va_list ap) {
    pa_proplist *l;

    l = pa_proplist_new();

    for (;;) {
        const char *key, *value;

        if (!(key = va_arg(ap, const char*)))
            break;

        if (!(value = va_arg(ap, const char *))) {
            pa_proplist_free(l);
            return NULL;
        }

        if (pa_proplist_puts(l, key, value) < 0) {
            pa_proplist_free(l);
            return NULL;
        }
    }

    return l;
}

int driver_play(ca_context *c, uint32_t id, va_list ap) {
    struct private *p;
    pa_proplist *l;
    const char *name;
    ca_bool_t played = FALSE;

    ca_return_val_if_fail(c, PA_ERROR_INVALID);

    p = PRIVATE(c);

    ca_return_val_if_fail(p->mainloop, PA_ERROR_STATE);
    ca_return_val_if_fail(p->context, PA_ERROR_STATE);

    if (!(l = proplist_unroll(ap)))
        return PA_ERROR_INVALID;

    if (!(name = pa_proplist_gets(l, CA_PROP_EVENT_ID)))
        if (!(name = ca_context_gets(l, CA_PROP_EVENT_ID))) {
            pa_proplist_free(l);
            return PA_ERROR_INVALID;
        }

    pa_threaded_mainloop_lock(p->mainloop);

    if ((o = pa_context_play_sample_with_proplist(p->context, name, NULL, PA_VOLUME_NORM, l, id, play_success_cb, c))) {

        while (pa_operation_get_state(o) != OPERATION_DONE)
            pa_threaded_mainloop_wait(m);

        pa_operation_unref(o);
    }

    pa_threaded_mainloop_unlock(p->mainloop);

    if (played)
        return CA_SUCCESS;

    va_copy(aq, ap);
    ret = file_open(&f, c, aq);
    va_end(aq);

    if (ret < 0) {
        pa_proplist_free(l);
        return ret;
    }

    pa_threaded_mainloop_lock(p->mainloop);

    s = pa_stream_new_with_proplist(p->context, name, &ss, NULL, l);
    pa_proplist_free(l);

    if (!s) {
        ret = translate_error(pa_context_errno(p->context));
        file_close(f);
        pa_threaded_mainloop_unlock(p->mainloop);
        return ret;
    }

    pa_stream_set_state_callback(p->stream, stream_state_cb, c);
    pa_stream_set_write_callback(p->stream, stream_request_cb, c);

    if (pa_stream_connect_playback(s, NULL, NULL, 0, NULL, NULL) < 0) {
        ret = translate_error(pa_context_errno(p->context));
        file_close(f);
        pa_stream_disconnect(s);
        pa_stream_unref(s);
        pa_threaded_mainloop_unlock(p->mainloop);
        return ret;
    }

    while (!done) {

        if (pa_stream_get_state(s) != PA_STREAM_READY ||
            pa_context_get_state(c) != PA_CONTEXT_READY) {

            ret = translate_error(pa_context_errno(p->context));
            file_close(f);
            pa_stream_disconnect(s);
            pa_stream_unref(s);
            pa_threaded_mainloop_unlock(p->mainloop);
            return ret;
        }

        pa_threaded_mainloop_wait(p->mainloop);
    }

    pa_stream_disconnect(s);
    pa_stream_unref(s);

    return CA_SUCCESS;

}

int driver_cancel(ca_context *c, uint32_t id) {
    ca_return_val_if_fail(c, PA_ERROR_INVALID);

}

int driver_cache(ca_context *c, va_list ap) {
    struct private *p;
    pa_proplist *l;
    ca_file *f;
    int ret;
    va_list aq;
    pa_stream *s;
    const char *name;

    ca_return_val_if_fail(c, PA_ERROR_INVALID);

    p = PRIVATE(c);

    ca_return_val_if_fail(p->mainloop, PA_ERROR_STATE);
    ca_return_val_if_fail(p->context, PA_ERROR_STATE);

    if (!(l = proplist_unroll(ap)))
        return PA_ERROR_INVALID;

    if (!(name = pa_proplist_gets(l, CA_PROP_EVENT_ID)))
        if (!(name = ca_context_gets(l, CA_PROP_EVENT_ID)))  {
            pa_proplist_free(l);
            return PA_ERROR_INVALID;
        }

    va_copy(aq, ap);
    ret = file_open(&f, c, aq);
    va_end(aq);

    if (ret < 0) {
        pa_proplist_free(l);
        return ret;
    }

    pa_threaded_mainloop_lock(p->mainloop);

    s = pa_stream_new_with_proplist(p->context, name, &ss, NULL, l);
    pa_proplist_free(l);

    if (!s) {
        ret = translate_error(pa_context_errno(p->context));
        file_close(f);
        pa_threaded_mainloop_unlock(p->mainloop);
        return ret;
    }

    pa_stream_set_state_callback(p->stream, stream_state_cb, c);
    pa_stream_set_write_callback(p->stream, stream_request_cb, c);

    if (pa_stream_connect_upload(s, f->nbytes) < 0) {
        ret = translate_error(pa_context_errno(p->context));
        file_close(f);
        pa_stream_disconnect(s);
        pa_stream_unref(s);
        pa_threaded_mainloop_unlock(p->mainloop);
        return ret;
    }

    while (!done) {

        if (pa_stream_get_state(s) != PA_STREAM_READY ||
            pa_context_get_state(c) != PA_CONTEXT_READY) {

            ret = translate_error(pa_context_errno(p->context));
            file_close(f);
            pa_stream_disconnect(s);
            pa_stream_unref(s);
            pa_threaded_mainloop_unlock(p->mainloop);
            return ret;
        }

        pa_threaded_mainloop_wait(p->mainloop);
    }

    pa_stream_disconnect(s);
    pa_stream_unref(s);

    return CA_SUCCESS;
}
