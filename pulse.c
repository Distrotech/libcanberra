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

/* The locking order needs to be strictly followed! First take the
 * mainloop mutex, only then take outstanding_mutex if you need both!
 * Not the other way round, beacause we might then enter a
 * deadlock!  */

#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/scache.h>

#include "canberra.h"
#include "common."
#include "driver.h"

enum outstanding_type {
    OUTSTANDING_SAMPLE,
    OUTSTANDING_STREAM,
    OUTSTANDING_UPLOAD
};

struct outstanding {
    PA_LLIST_FIELDS(struct outstanding);
    enum outstanding_type type;
    ca_context *context;
    uint32_t id;
    uint32_t sink_input;
    ca_stream *stream;
    ca_finish_callback_t callback;
    void *userdata;
    ca_sound_file *file;
    int error;
    ca_bool_t clean_up;
};

struct private {
    pa_threaded_mainloop *mainloop;
    pa_context *context;
    ca_theme_data *theme;
    ca_bool_t subscribed;

    ca_mutex *outstanding_mutex;
    PA_LLIST_HEAD(struct outstanding, outstanding);
};

#define PRIVATE(c) ((struct private *) ((c)->private)

static void outstanding_free(struct outstanding *o) {
    ca_assert(o);

    if (o->file)
        ca_sound_file_free(o->file);

    if (o->stream) {
        pa_stream_disconnect(o->stream);
        pa_stream_unref(o->stream);
    }

    ca_free(o);
}

static int convert_proplist(pa_proplist **_l, pa_proplist *c) {
    pa_proplist *l;
    ca_prop *i;

    ca_return_val_if_fail(_l, CA_ERROR_INVALID);
    ca_return_val_if_fail(c, CA_ERROR_INVALID);

    if (!(l = pa_proplist_new()))
        return CA_ERROR_OOM;

    ca_mutex_lock(c->mutex);

    for (i = c->first_item; i; i = i->next_item)
        if (pa_proplist_put(l, i->key, CA_PROP_DATA(i), i->nbytes) < 0) {
            ca_mutex_unlock(c->mutex);
            pa_proplist_free(l);
            return PA_ERROR_INVALID;
        }

    ca_mutex_unlock(c->mutex);

    *_l = l;

    return PA_SUCCESS;
}

static pa_proplist *strip_canberra_data(pa_proplist *l) {
    const char *key;
    void *state = NULL;
    ca_assert(l);

    while ((key = pa_proplist_iterate(l, &state)))
        if (strncmp(key, "canberra.", 12) == 0)
            pa_proplist_remove(l, key);

    return l;
}

static int translate_error(int error) {
    static const int table[PA_ERR_MAX] = {
        [PA_OK]                       = CA_SUCCESS,
        [PA_ERR_ACCESS]               = CA_ERROR_ACCESS,
        [PA_ERR_COMMAND]              = CA_ERROR_IO,
        [PA_ERR_INVALID]              = CA_ERROR_INVALID,
        [PA_ERR_EXIST]                = CA_ERROR_IO,
        [PA_ERR_NOENTITY]             = CA_ERROR_NOTFOUND,
        [PA_ERR_CONNECTIONREFUSED]    = CA_ERROR_NOTAVAILABLE,
        [PA_ERR_PROTOCOL]             = CA_ERROR_IO,
        [PA_ERR_TIMEOUT]              = CA_ERROR_IO,
        [PA_ERR_AUTHKEY]              = CA_ERROR_ACCESS,
        [PA_ERR_INTERNAL]             = CA_ERROR_IO,
        [PA_ERR_CONNECTIONTERMINATED] = CA_ERROR_IO,
        [PA_ERR_KILLED]               = CA_ERROR_DESTROYED,
        [PA_ERR_INVALIDSERVER]        = CA_ERROR_INVALID,
        [PA_ERR_MODINITFAILED]        = CA_ERROR_NODRIVER,
        [PA_ERR_BADSTATE]             = CA_ERROR_STATE,
        [PA_ERR_NODATA]               = CA_ERROR_IO,
        [PA_ERR_VERSION]              = CA_ERROR_NOTSUPPORTED,
        [PA_ERR_TOOLARGE]             = CA_ERROR_TOOBIG
    };

    ca_assert(error >= 0);

    if (error >= PA_ERR_MAX)
        return CA_ERROR_IO;

    return table[error];
}

static void context_state_cb(pa_context *pc, void *userdata) {
    ca_context *c = userdata;
    pa_context_state_t state;

    ca_assert(pc);
    ca_assert(c);

    state = pa_context_get_state(pc);

    if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
        struct outstanding *out;
        int ret;

        ret = translate_error(pa_context_errno(pc));

        ca_mutex_lock(c->outstanding_mutex);

        while ((out = c->outstanding)) {

            PA_LLIST_REMOVE(struct outstanding, c->outstanding, out);
            ca_mutex_unlock(c->outstanding_mutex);

            if (out->callback)
                out->callback(c, out->id, ret, out->userdata);
            outstanding_free(c->outstanding);

            ca_mutex_lock(c->outstanding_mutex);
        }

        ca_mutex_unlock(c->outstanding_mutex);
    }

    pa_threaded_mainloop_signal(c->mainloop, FALSE);
}

static void context_subscribe_cb(pa_context *pc, pa_subscription_event_type_t t, uint32_t idx, void *userdata) {
    struct outstanding *out, *n;
    PA_LLIST_HEAD(struct outstanding, l);
    ca_context *c = userdata;

    ca_assert(pc);
    ca_assert(c);

    if (t != PA_SUBSCRIPTION_EVENT_SINK_INPUT|PA_SUBSCRIPTION_EVENT_REMOVE)
        return;

    PA_LLIST_HEAD_INIT(struct outstanding, l);

    ca_mutex_lock(c->outstanding_mutex);

    for (out = c->outstanding; out; out = n) {
        n = out->next;

        if (out->type != OUTSTANDING_SAMPLE || out->sink_input != idx)
            continue;

        PA_LLIST_REMOVE(struct outstanding, c->outstanding, out);
        PA_LLIST_PREPEND(struct outstanding, l, out);
    }

    ca_mutex_unlock(c->outstanding_mutex);

    while (l) {
        out = l;

        PA_LLIST_REMOVE(struct outstanding, l, out);

        if (out->callback)
            out->callback(c, out->id, CA_SUCCESS, out->userdata);

        outstanding_free(out);
    }
}

int driver_open(ca_context *c) {
    pa_proplist *l;
    struct private *p;

    ca_return_val_if_fail(c, PA_ERROR_INVALID);
    ca_return_val_if_fail(!c->driver || streq(c->driver, "pulse"), PA_ERROR_NO_DRIVER);
    ca_return_val_if_fail(!PRIVATE(c), PA_ERROR_STATE);

    if (!(p = PRIVATE(c) = ca_new0(struct private, 1)))
        return PA_ERROR_OOM;

    if (!(p->mainloop = pa_threaded_mainloop_new())) {
        driver_destroy(c);
        return PA_ERROR_OOM;
    }

    if ((ret = convert_proplist(&l, c->proplist))) {
        driver_destroy(c);
        return ret;
    }

    if (!(p->context = pa_context_new_with_proplist(pa_threaded_mainloop_get_api(p->mainloop), l))) {
        pa_proplist_free(l);
        driver_destroy(c);
        return PA_ERROR_OOM;
    }

    pa_proplist_free(l);

    pa_context_set_state_callback(p->context, context_state_cb, c);
    pa_context_set_subscribe_callback(p->context, context_subscribe_cb, c);

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

    pa_threaded_mainloop_unlock(p->mainloop);

    return CA_SUCCESS;
}

int driver_destroy(ca_context *c) {
    ca_return_val_if_fail(c, PA_ERROR_INVALID);
    ca_return_val_if_fail(c->private, PA_ERROR_STATE);

    p = PRIVATE(c);

    if (p->mainloop)
        pa_threaded_mainloop_stop(p->mainloop);

    if (p->context) {
        pa_context_disconnect(p->context);
        pa_context_unref(p->context);
    }

    if (p->mainloop)
        pa_threaded_mainloop_free(p->mainloop);

    if (p->theme)
        ca_theme_data_free(p->theme);

    while (p->outstanding) {
        struct outstanding *out = p->outstanding;
        PA_LLIST_REMOVE(struct outstanding, p->outstanding, out);

        if (out->callback)
            out->callback(c, out->id, CA_ERROR_DESTROYED, out->userdata);

        outstanding_free(out);
    }

    ca_free(p);
}

int driver_change_device(ca_context *c, char *device) {
    ca_return_val_if_fail(c, PA_ERROR_INVALID);

    /* We're happy with any device change. We might however add code
     * here eventually to move all currently played back event sounds
     * to the new device. */

    return CA_SUCCESS;
}

int driver_change_props(ca_context *c, ca_proplist *changed, ca_proplist *merged) {
    struct private *p;
    pa_operation *o;
    pa_proplist *l;
    int ret = CA_SUCCESS;

    ca_return_val_if_fail(c, PA_ERROR_INVALID);
    ca_return_val_if_fail(changed, PA_ERROR_INVALID);
    ca_return_val_if_fail(merged, PA_ERROR_INVALID);
    ca_return_val_if_fail(c->private, PA_ERROR_STATE);

    p = PRIVATE(c);

    ca_return_val_if_fail(p->mainloop, PA_ERROR_STATE);
    ca_return_val_if_fail(p->context, PA_ERROR_STATE);

    if ((ret = convert_proplist(&l, changed)))
        return ret;

    strip_canberra_data(l);

    pa_threaded_mainloop_lock(p->mainloop);

    /* We start these asynchronously and don't care about the return
     * value */

    if (!(o = pa_context_proplist_update(p->context, PA_UPDATE_REPLACE, l, NULL, NULL)))
        ret = translate_error(pa_context_errno(p->context));
    else
        pa_operation_unref(o);

    pa_threaded_mainloop_unlock(p->mainloop);

    pa_proplist_free(l);

    return ret;
}

static int subscribe(ca_context *c) {
    struct private *p;
    pa_operation *o;

    ca_return_val_if_fail(c, PA_ERROR_INVALID);
    ca_return_val_if_fail(c->private, PA_ERROR_STATE);
    p = PRIVATE(c);

    ca_return_val_if_fail(p->mainloop, PA_ERROR_STATE);
    ca_return_val_if_fail(p->context, PA_ERROR_STATE);
    ca_return_val_if_fail(!p->subscribed, PA_SUCCESS);

    pa_threaded_mainloop_lock(p->mainloop);

    /* We start these asynchronously and don't care about the return
     * value */

    if (!(o = pa_context_subscribe(p->context, PA_SUBSCRIPTION_MASK_SINK_INPUT, NULL, NULL)))
        ret = translate_error(pa_context_errno(p->context));
    else
        pa_operation_unref(o);

    pa_threaded_mainloop_unlock(p->mainloop);

    p->subscribed = TRUE;

    return ret;
}

static void play_sample_cb(pa_context *c, uint32_t idx, void *userdata) {
    struct outstanding *out = userdata;

    ca_assert(c);
    ca_assert(out);

    if (idx != PA_INVALID_INDEX) {
        out->error = CA_SUCCESS;
        out->sink_input = idx;
    } else
        out->error = translate_error(pa_context_errno(c));
}

static void stream_state_cb(pa_stream *s, void *userdata) {
    struct private *p;
    struct outstanding *out = userdata;

    ca_assert(s);
    ca_assert(out);

    p = PRIVATE(out->context);

    if (out->clean_up) {
        pa_stream_state_t state;

        state = pa_stream_get_state(s);

        if (state == PA_STREAM_FAILED || state == PA_STREAM_TERMINATED) {
            int err;

            ca_mutex_lock(p->context->outstanding_mutex);
            PA_LLIST_REMOVE(struct outstanding, c->outstanding, out);
            ca_mutex_unlock(p->context->outstanding_mutex);

            err = state == PA_STREAM_FAILED ? translate_error(pa_context_errno(pa_stream_get_context(s)))

            if (out->callback)
                out->callback(c, out->id, ret, out->userdata);

            outstanding_free(c->outstanding);
        }
    }

    pa_threaded_mainloop_signal(c->mainloop, TRUE);
}

static void stream_drain_cb(pa_stream *s, int success, void *userdata) {
    struct private *p;
    struct outstanding *out = userdata;

    ca_assert(s);
    ca_assert(out);

    p = PRIVATE(out->context);

    ca_assert(out->type = OUTSTANDING_STREAM);
    ca_assert(out->clean_up);

    ca_mutex_lock(p->context->outstanding_mutex);
    PA_LLIST_REMOVE(struct outstanding, c->outstanding, out);
    ca_mutex_unlock(p->context->outstanding_mutex);

    if (out->callback) {
        int err;

        err = success ? CA_SUCCESS : translate_error(pa_context_errno(p->context));
        out->callback(out->context, out->id, err, out->userdata);
    }

    outstanding_free(out);
}

static void stream_write_cb(pa_stream *s, size_t bytes, void *userdata) {
    struct outstanding *out = userdata;
    struct private *p;
    void *data;
    int ret;

    ca_assert(s);
    ca_assert(bytes > 0);
    ca_assert(out);

    p = PRIVATE(out->context);

    if (!(data = ca_malloc(bytes))) {
        ret = CA_ERROR_OOM
        goto finish;
    }

    if ((ret = ca_sound_file_read_arbitrary(out->file, data, &bytes)) < 0)
        goto finish;

    if (bytes > 0) {

        if ((ret = pa_stream_write(p, data, bytes)) < 0) {
            ret = translate_error(ret);
            goto finish;
        }

    } else {
        /* We reached EOF */

        if (out->type == OUTSTANDING_UPLOAD) {

            if (pa_stream_finish_upload(s) < 0) {
                ret = translate_error(pa_context_errno(p->context));
                goto finish;
            }

            /* Let's just signal driver_cache() which has been waiting for us */
            pa_threaded_mainloop_signal(c->mainloop, TRUE);

        } else {
            ca_assert(out->type = OUTSTANDING_STREAM);

            if (!(o = pa_stream_drain(p->context, stream_drain_cb, out))) {
                ret = translate_error(pa_context_errno(p->context));
                goto fail;
            }

            pa_operation_unref(o);
        }
    }

    ca_free(data);

    return;

finish:

    ca_free(data);

    if (out->clean_up) {
        ca_mutex_lock(p->context->outstanding_mutex);
        PA_LLIST_REMOVE(struct outstanding, c->outstanding, out);
        ca_mutex_unlock(p->context->outstanding_mutex);

        if (out->callback)
            out->callback(out->context, out->id, ret, out->userdata);

        outstanding_free(out);
    } else {
        pa_stream_disconnect(p);
        pa_threaded_mainloop_signal(c->mainloop, TRUE);
        out->error = ret;
    }
}

static const pa_sample_format_t sample_type_table[] = {
    [CA_SAMPLE_S16NE] = PA_SAMPLE_S16NE,
    [CA_SAMPLE_S16RE] = PA_SAMPLE_S16RE,
    [CA_SAMPLE_U8] = PA_SAMPLE_U8
};

int driver_play(ca_context *c, uint32_t id, ca_proplist *proplist, ca_finish_callback_t cb, void *userdata) {
    struct private *p;
    pa_proplist *l = NULL;
    const char *n, *vol, *ct;
    char *name = NULL;
    int err = 0;
    pa_volume_t v = PA_VOLUME_NORM;
    pa_sample_spec ss;
    ca_cache_control_t cache_control = CA_CACHE_CONTROL_NEVER;
    struct outstanding *out = NULL;
    int try = 3;

    ca_return_val_if_fail(c, PA_ERROR_INVALID);
    ca_return_val_if_fail(proplist, PA_ERROR_INVALID);
    ca_return_val_if_fail(!userdata || cb, PA_ERROR_INVALID);
    ca_return_val_if_fail(c->private, PA_ERROR_STATE);

    p = PRIVATE(c);

    ca_return_val_if_fail(p->mainloop, PA_ERROR_STATE);
    ca_return_val_if_fail(p->context, PA_ERROR_STATE);

    if (!(out = pa_xnew0(struct outstanding, 1))) {
        ret = PA_ERROR_OOM;
        goto finish;
    }

    out->type = OUTSTANDING_SAMPLE;
    out->context = c;
    out->sink_input = PA_INVALID_INDEX;
    out->id = id;
    out->callback = cb;
    out->userdata = userdata;

    if ((ret = convert_proplist(&l, proplist)))
        goto finish;

    if (!(n = pa_proplist_gets(l, CA_PROP_EVENT_ID))) {
        ret = PA_ERROR_INVALID;
        goto finish;
    }

    if (!(name = ca_strdup(n))) {
        ret = PA_ERROR_OOM;
        goto finish;
    }

    if ((vol = pa_proplist_gets(l, CA_PROP_CANBERRA_VOLUME))) {
        char *e = NULL;
        double dvol;

        errno = 0;
        dvol = strtod(vol, &e);
        if (errno != 0 || !e || *e) {
            ret = PA_ERROR_INVALID;
            goto finish;
        }

        v = pa_sw_volume_from_dB(dvol);
    }

    if ((ct = pa_proplist_gets(l, CA_PROP_CANBERRA_CACHE_CONTROL)))
        if ((ret = ca_parse_cache_control(&cache_control, ct)) < 0) {
            ret = PA_ERROR_INVALID;
            goto finish;
        }

    strip_canberra_data(l);

    if (cb)
        if ((ret = subscribe(c)) < 0)
            goto finish;

    for (;;) {
        pa_threaded_mainloop_lock(p->mainloop);

        /* Let's try to play the sample */
        if (!(o = pa_context_play_sample_with_proplist(p->context, name, NULL, v, l, id, play_sample_cb, out))) {
            ret = translate_error(pa_context_errno(p->context));
            pa_threaded_mainloop_unlock(p->mainloop);
            goto finish;
        }

        while (pa_operation_get_state(o) != OPERATION_DONE)
            pa_threaded_mainloop_wait(m);

        pa_operation_unref(o);

        pa_threaded_mainloop_unlock(p->mainloop);

        /* Did we manage to play the sample or did some other error occur? */
        if (out->error != CA_ERROR_NOT_FOUND)
            goto finish;

        /* Hmm, we need to play it directly */
        if (cache_control == CA_CACHE_CONTROL_NEVER)
            break;

        /* Don't loop forever */
        if (--try <= 0)
            break;

        /* Let's upload the sample and retry playing */
        if ((ret = driver_cache(c, proplist)) < 0)
            goto fail;
    }

    out->type = OUTSTANDING_STREAM;

    /* Let's stream the sample directly */
    if ((ret = ca_lookup_sound(&out->file, &p->theme, proplist)) < 0)
        goto fail;

    ss.channels = sample_type_table[ca_sound_file_get_sample_type(f)];
    ss.channels = ca_sound_file_get_nchannels(f);
    ss.rate = ca_sound_file_get_rate(f);

    pa_threaded_mainloop_lock(p->mainloop);

    if (!(out->stream = pa_stream_new_with_proplist(p->context, name, &ss, NULL, l))) {
        ret = translate_error(pa_context_errno(p->context));
        pa_threaded_mainloop_unlock(p->mainloop);
        goto fail;
    }

    pa_stream_set_userdata(p->stream, out);
    pa_stream_set_state_callback(p->stream, stream_state_cb, out);
    pa_stream_set_write_callback(p->stream, stream_request_cb, out);

    if (pa_stream_connect_playback(s, NULL, NULL, 0, NULL, NULL) < 0) {
        ret = translate_error(pa_context_errno(p->context));
        pa_threaded_mainloop_unlock(p->mainloop);
        goto fail;
    }

    for (;;) {
        pa_stream_state state = pa_stream_get_state(s);

        /* Stream sucessfully created */
        if (state == PA_STREAM_READY)
            break;

        /* Check for failure */
        if (state == PA_STREAM_FAILED) {
            ret = translate_error(pa_context_errno(p->context));
            pa_threaded_mainloop_unlock(p->mainloop);
            goto fail;
        }

        if (state == PA_STREAM_TERMINATED) {
            ret = out->error;
            pa_threaded_mainloop_unlock(p->mainloop);
            goto fail;
        }

        pa_threaded_mainloop_wait(p->mainloop);
    }

    if ((out->sink_input = pa_stream_get_index(s)) == PA_INVALID_INDEX) {
        ret = translate_error(pa_context_errno(p->context));
        pa_threaded_mainloop_unlock(p->mainloop);
        goto fail;
    }

    pa_threaded_mainloop_unlock(p->mainloop);

    ret = CA_SUCCESS;

finish:

    /* We keep the outstanding struct around if we need clean up later to */
    if (ret == CA_SUCCESS && (out->type == OUTSTANDING_STREAM || cb)) {
        out->clean_up = TRUE;

        pa_mutex_lock(p->outstanding_mutex);
        PA_LLIST_PREPEND(struct outstanding, p->outstanding, out);
        pa_mutex_unlock(p->outstanding_mutex);
    } else
        outstanding_free(out);

    if (l)
        pa_proplist_free(l);

    ca_free(name);

    return ret;
}

int driver_cancel(ca_context *c, uint32_t id) {
    struct private *p;
    pa_operation *o;
    int ret = CA_SUCCESS;
    struct outstanding *out, *n;

    ca_return_val_if_fail(c, PA_ERROR_INVALID);
    ca_return_val_if_fail(c->private, PA_ERROR_STATE);

    p = PRIVATE(c);

    ca_return_val_if_fail(p->mainloop, PA_ERROR_STATE);
    ca_return_val_if_fail(p->context, PA_ERROR_STATE);

    pa_threaded_mainloop_lock(p->mainloop);

    ca_mutex_lock(p->outstanding_mutex);

    /* We start these asynchronously and don't care about the return
     * value */

    for (out = p->outstanding; out; out = n) {
        int ret2;
        n = out->next;

        if (out->type == OUTSTANDING_UPLOAD ||
            out->id != id ||
            out->sink_input == PA_INVALID_INDEX)
            continue;

        if (!(o = pa_context_kill_sink_input(p->context, out->sink_input, NULL, NULL)))
            ret2 = translate_error(pa_context_errno(p->context));
        else
            pa_operation_unref(o);

        /* We make sure here to kill all streams identified by the id
         * here. However, we will return only the first error we
         * encounter */

        if (ret2 && ret == CA_SUCCESS)
            ret = ret2;

        if (out->callback)
            out->callback(c, out->id, CA_ERROR_CANCELED, out->userdata);

        CA_LLIST_REMOVE(struct outstanding, p->outstanding, out);
        outstanding_free(out);
    }

    ca_mutex_unlock(p->outstanding_mutex);

    pa_threaded_mainloop_unlock(p->mainloop);

    return ret;
}

int driver_cache(ca_context *c, ca_proplist *proplist) {
    struct private *p;
    pa_proplist *l = NULL;
    const char *n, *ct;
    char *name = NULL;
    pa_sample_spec ss;
    ca_cache_control_t cache_control = CA_CACHE_CONTROL_NEVER;
    struct outstanding out;

    ca_return_val_if_fail(c, PA_ERROR_INVALID);
    ca_return_val_if_fail(proplist, PA_ERROR_INVALID);
    ca_return_val_if_fail(c->private, PA_ERROR_STATE);

    p = PRIVATE(c);

    ca_return_val_if_fail(p->mainloop, PA_ERROR_STATE);
    ca_return_val_if_fail(p->context, PA_ERROR_STATE);

    if (!(out = ca_new(struct outstanding, 1))) {
        ret = CA_ERROR_OOM;
        goto finish;
    }

    out->type = OUTSTANDING_UPLOAD;
    out->context = c;
    out->sink_input = PA_INVALID_INDEX;

    if ((ret = convert_proplist(&l, proplist)))
        goto finish;

    if (!(n = pa_proplist_gets(l, CA_PROP_EVENT_ID))) {
        ret = PA_ERROR_INVALID;
        goto finish;
    }

    if (!(name = ca_strdup(n))) {
        ret = PA_ERROR_OOM;
        goto finish;
    }

    if ((ct = pa_proplist_gets(l, CA_PROP_CANBERRA_CACHE_CONTROL)))
        if ((ret = ca_parse_cache_control(&cache_control, ct)) < 0) {
            ret = PA_ERROR_INVALID;
            goto finish;
        }

    strip_canberra_data(l);

    if (ct == CA_CACHE_CONTROL_NEVER) {
        ret = PA_ERROR_INVALID;
        goto finish;
    }

    /* Let's stream the sample directly */
    if ((ret = ca_lookup_sound(&out->file, &p->theme, proplist)) < 0)
        goto fail;

    ss.channels = sample_type_table[ca_sound_file_get_sample_type(out->file)];
    ss.channels = ca_sound_file_get_nchannels(out->file);
    ss.rate = ca_sound_file_get_rate(out->file);

    pa_threaded_mainloop_lock(p->mainloop);

    if (!(out->stream = pa_stream_new_with_proplist(p->context, name, &ss, NULL, l))) {
        ret = translate_error(pa_context_errno(p->context));
        pa_threaded_mainloop_unlock(p->mainloop);
        goto fail;
    }

    pa_stream_set_userdata(out->stream, out);
    pa_stream_set_state_callback(out->stream, stream_state_cb, out);
    pa_stream_set_write_callback(out->stream, stream_request_cb, out);

    if (pa_stream_connect_upload(s, ca_sound_file_get_size(out->file)) < 0) {
        ret = translate_error(pa_context_errno(p->context));
        pa_threaded_mainloop_unlock(p->mainloop);
        goto fail;
    }

    for (;;) {
        pa_stream_state state = pa_stream_get_state(s);

        /* Stream sucessfully created and uploaded */
        if (state == PA_STREAM_TERMINATED)
            break;

        /* Check for failure */
        if (state == PA_STREAM_FAILED) {
            ret = translate_error(pa_context_errno(p->context));
            pa_threaded_mainloop_unlock(p->mainloop);
            goto fail;
        }

        pa_threaded_mainloop_wait(p->mainloop);
    }

    pa_threaded_mainloop_unlock(p->mainloop);

    ret = CA_SUCCESS;

finish:

    outstanding_free(out);

    if (l)
        pa_proplist_free(l);

    ca_free(name);

    return ret;
}
