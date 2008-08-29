/***
    This file is part of libcanberra.

    Copyright 2008 Nokia Corporation and/or its subsidiary(-ies).

    Author: Marc-Andre Lureau <marc-andre.lureau@nokia.com>

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

#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gst/gst.h>

#include "canberra.h"
#include "common.h"
#include "driver.h"
#include "llist.h"
#include "read-sound-file.h"
#include "sound-theme-spec.h"
#include "malloc.h"

struct outstanding {
    CA_LLIST_FIELDS(struct outstanding);
    ca_bool_t dead;
    uint32_t id;
    ca_finish_callback_t callback;
    void *userdata;
    GstElement *pipeline;
    struct ca_context *context;
};

struct private {
    ca_theme_data *theme;
    ca_mutex *outstanding_mutex;
    ca_bool_t signal_semaphore;
    sem_t semaphore;
    ca_bool_t semaphore_allocated;
    CA_LLIST_HEAD(struct outstanding, outstanding);
};

#define PRIVATE(c) ((struct private *) ((c)->private))

static void outstanding_free(struct outstanding *o) {
    GstBus *bus;

    ca_assert(o);

    bus = gst_pipeline_get_bus(GST_PIPELINE (o->pipeline));
    gst_bus_set_sync_handler(bus, NULL, NULL);
    gst_object_unref(bus);

    if (o->pipeline)
        gst_object_unref(GST_OBJECT(o->pipeline));

    ca_free(o);
}

int driver_open(ca_context *c) {
    GError *error = NULL;
    struct private *p;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(!PRIVATE(c), CA_ERROR_INVALID);
    ca_return_val_if_fail(!c->driver || ca_streq(c->driver, "gstreamer"), CA_ERROR_NODRIVER);

    gst_init_check (NULL, NULL, &error);
    if (error != NULL) {
        g_warning("gst_init: %s ", error->message);
        g_error_free(error);
        return CA_ERROR_INVALID;
    }

    if (!(p = ca_new0(struct private, 1)))
        return CA_ERROR_OOM;

    if (!(p->outstanding_mutex = ca_mutex_new())) {
        driver_destroy(c);
        return CA_ERROR_OOM;
    }

    if (sem_init(&p->semaphore, 0, 0) < 0) {
        driver_destroy(c);
        return CA_ERROR_OOM;
    }

    p->semaphore_allocated = TRUE;

    c->private = p;

    return CA_SUCCESS;
}

int driver_destroy(ca_context *c) {
    struct private *p;
    struct outstanding *out;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(PRIVATE(c), CA_ERROR_STATE);

    p = PRIVATE(c);

    if (p->outstanding_mutex) {
        ca_mutex_lock(p->outstanding_mutex);

        /* Tell all player threads to terminate */
        out = p->outstanding;
        while (out) {
            GstElement *pipeline;

            if (out->dead) {
                out = out->next;
                continue;
            }

            pipeline = out->pipeline;
            out->dead = TRUE;

            if (out->callback)
                out->callback(c, out->id, CA_ERROR_DESTROYED, out->userdata);

            out = out->next;

            ca_mutex_unlock(p->outstanding_mutex);

            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(GST_OBJECT(pipeline));

            ca_mutex_lock(p->outstanding_mutex);
        }

        if (p->semaphore_allocated) {
            /* Now wait until all players are destroyed */
            p->signal_semaphore = TRUE;
            while (p->outstanding) {
                ca_mutex_unlock(p->outstanding_mutex);
                sem_wait(&p->semaphore);
                ca_mutex_lock(p->outstanding_mutex);
            }
        }

        ca_mutex_unlock(p->outstanding_mutex);
        ca_mutex_free(p->outstanding_mutex);
    }

    if (p->theme)
        ca_theme_data_free(p->theme);

    if (p->semaphore_allocated)
        sem_destroy(&p->semaphore);

    ca_free(p);

    /* no gst_deinit (), see doc */

    return CA_SUCCESS;
}

int driver_change_device(ca_context *c, char *device) {
    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(PRIVATE(c), CA_ERROR_STATE);

    return CA_SUCCESS;
}

int driver_change_props(ca_context *c, ca_proplist *changed, ca_proplist *merged) {
    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(changed, CA_ERROR_INVALID);
    ca_return_val_if_fail(merged, CA_ERROR_INVALID);
    ca_return_val_if_fail(PRIVATE(c), CA_ERROR_STATE);

    return CA_SUCCESS;
}

static GstBusSyncReply bus_cb(GstBus *bus, GstMessage *message, gpointer data) {
    int err;
    struct outstanding *out;
    struct private *p;

    ca_return_val_if_fail(bus, GST_BUS_DROP);
    ca_return_val_if_fail(message, GST_BUS_DROP);
    ca_return_val_if_fail(data, GST_BUS_DROP);

    out = data;
    p = PRIVATE(out->context);

    switch (GST_MESSAGE_TYPE(message)) {
        /* for all elements */
        case GST_MESSAGE_UNKNOWN:
            return GST_BUS_DROP;
        case GST_MESSAGE_ERROR:
            err = CA_ERROR_SYSTEM;
            break;
            /* only from bin */
        case GST_MESSAGE_EOS:
            if (GST_OBJECT(out->pipeline) != GST_MESSAGE_SRC(message))
                return GST_BUS_DROP;

            err = CA_SUCCESS;
            break;
        case GST_MESSAGE_STATE_CHANGED: {
            GstState pending;

            if (GST_OBJECT(out->pipeline) != GST_MESSAGE_SRC(message))
                return GST_BUS_DROP;

            gst_message_parse_state_changed(message, NULL, NULL, &pending);
            /* g_debug (gst_element_state_get_name (pending)); */

            if (pending == GST_STATE_NULL || pending == GST_STATE_VOID_PENDING)
                err = CA_SUCCESS;
            else
                return GST_BUS_DROP;
            break;
        }
        default:
            return GST_BUS_DROP;
    }

    if (!out->dead && out->callback)
        out->callback(out->context, out->id, err, out->userdata);

    ca_mutex_lock(p->outstanding_mutex);

    CA_LLIST_REMOVE(struct outstanding, p->outstanding, out);

    if (!p->outstanding && p->signal_semaphore)
        sem_post(&p->semaphore);

    outstanding_free(out);

    ca_mutex_unlock(p->outstanding_mutex);

    return GST_BUS_DROP;
}

struct ca_sound_file {
    GstElement *fdsrc;
};

static int ca_gst_sound_file_open(ca_sound_file **_f, const char *fn) {
    int fd;
    ca_sound_file *f;

    ca_return_val_if_fail(_f, CA_ERROR_INVALID);
    ca_return_val_if_fail(fn, CA_ERROR_INVALID);

    if ((fd = open(fn, O_RDONLY)) == -1)
        return errno == ENOENT ? CA_ERROR_NOTFOUND : CA_ERROR_SYSTEM;

    if (!(f = ca_new0(ca_sound_file, 1)))
        return CA_ERROR_OOM;

    if (!(f->fdsrc = gst_element_factory_make("fdsrc", NULL))) {
        ca_free(f);
        return CA_ERROR_OOM;
    }

    g_object_set(GST_OBJECT(f->fdsrc), "fd", fd, NULL);
    *_f = f;

    return CA_SUCCESS;
}

static void on_pad_added(GstElement *element, GstPad *pad, gboolean arg1, gpointer data)
{
    GstStructure *structure;
    GstElement *sinkelement;
    GstCaps *caps;
    GstPad *vpad;
    const char *type;

    sinkelement = (GstElement *)data;

    caps = gst_pad_get_caps (pad);
    if (gst_caps_is_empty (caps) || gst_caps_is_any (caps)) {
        gst_caps_unref (caps);
        return;
    }

    structure = gst_caps_get_structure (caps, 0);
    type = gst_structure_get_name (structure);
    if (g_str_has_prefix (type, "audio/x-raw") == TRUE) {
        vpad = gst_element_get_pad (sinkelement, "sink");
        gst_pad_link (pad, vpad);
        gst_object_unref (vpad);
    }
    gst_caps_unref (caps);
}

int driver_play(ca_context *c, uint32_t id, ca_proplist *proplist, ca_finish_callback_t cb, void *userdata) {
    struct private *p;
    struct outstanding *out = NULL;
    ca_sound_file *f;
    GstElement *decodebin, *sink, *audioconvert, *audioresample, *bin;
    GstBus *bus;
    GstPad *audiopad;
    GstPad *pad;
    int ret;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(proplist, CA_ERROR_INVALID);
    ca_return_val_if_fail(!userdata || cb, CA_ERROR_INVALID);

    f = NULL;
    sink = NULL;
    decodebin = NULL;

    p = PRIVATE(c);

    if ((ret = ca_lookup_sound_with_callback(&f, ca_gst_sound_file_open, &p->theme, c->props, proplist)) < 0)
        goto fail;

    if (!(out = ca_new0(struct outstanding, 1)))
        return CA_ERROR_OOM;

    out->id = id;
    out->callback = cb;
    out->userdata = userdata;
    out->context = c;

    if (!(out->pipeline = gst_pipeline_new(NULL))
        || !(decodebin = gst_element_factory_make("decodebin2", NULL))
        || !(audioconvert = gst_element_factory_make("audioconvert", NULL))
        || !(audioresample = gst_element_factory_make("audioresample", NULL))
        || !(sink = gst_element_factory_make("autoaudiosink", NULL))) {
        ret = CA_ERROR_OOM;
        goto fail;
    }

    bin = gst_bin_new ("audiobin");

    g_signal_connect (decodebin, "new-decoded-pad", G_CALLBACK (on_pad_added), bin);

    bus = gst_pipeline_get_bus(GST_PIPELINE (out->pipeline));
    gst_bus_set_sync_handler(bus, bus_cb, out);
    gst_object_unref(bus);

    gst_bin_add_many(GST_BIN (out->pipeline),
                     f->fdsrc, decodebin, NULL);

    if (!gst_element_link(f->fdsrc, decodebin)) {
        f->fdsrc = NULL;
        decodebin = NULL;
        audioconvert = NULL;
        audioresample = NULL;
        sink = NULL;
        goto fail;
    }

    gst_bin_add_many (GST_BIN (bin), audioconvert, audioresample, sink, NULL);
    gst_element_link_many (audioconvert, audioresample, sink, NULL);

    audiopad = gst_element_get_pad (audioconvert, "sink");
    gst_element_add_pad (bin, gst_ghost_pad_new ("sink", audiopad));

    gst_object_unref (audiopad);

    gst_bin_add (GST_BIN (out->pipeline), bin);

    decodebin = NULL;
    sink = NULL;
    ca_free(f);
    f = NULL;

    ca_mutex_lock(p->outstanding_mutex);
    CA_LLIST_PREPEND(struct outstanding, p->outstanding, out);
    ca_mutex_unlock(p->outstanding_mutex);

    if (gst_element_set_state(out->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        ca_mutex_lock(p->outstanding_mutex);
        CA_LLIST_REMOVE(struct outstanding, p->outstanding, out);
        ca_mutex_unlock(p->outstanding_mutex);

        ret = CA_ERROR_NOTAVAILABLE;
        goto fail;
    }

    return CA_SUCCESS;

fail:
    if (f && f->fdsrc)
        gst_object_unref(f->fdsrc);

    if (f)
        ca_free(f);

    if (sink)
        gst_object_unref(sink);

    if (decodebin)
        gst_object_unref(decodebin);

    if (out && out->pipeline)
        gst_object_unref(out->pipeline);

    ca_free(out);

    return ret;
}

int driver_cancel(ca_context *c, uint32_t id) {
    struct private *p;
    struct outstanding *out = NULL;

    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(PRIVATE(c), CA_ERROR_STATE);

    p = PRIVATE(c);

    ca_mutex_lock(p->outstanding_mutex);

    for (out = p->outstanding; out; out = out->next) {
        GstElement *pipeline;

        if (out->id != id)
            continue;

        if (out->pipeline == NULL)
            break;

        if (out->callback)
            out->callback(c, out->id, CA_ERROR_CANCELED, out->userdata);

        pipeline = out->pipeline;
        out->dead = TRUE;

        ca_mutex_unlock(p->outstanding_mutex);
        gst_element_set_state(out->pipeline, GST_STATE_NULL);
        ca_mutex_lock(p->outstanding_mutex);

        gst_object_unref(pipeline);
    }

    ca_mutex_unlock(p->outstanding_mutex);

    return CA_SUCCESS;
}

int driver_cache(ca_context *c, ca_proplist *proplist) {
    ca_return_val_if_fail(c, CA_ERROR_INVALID);
    ca_return_val_if_fail(proplist, CA_ERROR_INVALID);
    ca_return_val_if_fail(PRIVATE(c), CA_ERROR_STATE);

    return CA_ERROR_NOTSUPPORTED;
}
