/*-*- Mode: C; c-basic-offset: 8 -*-*/

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
#include <unistd.h>

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
        int err;
        ca_finish_callback_t callback;
        void *userdata;
        GstElement *pipeline;
        struct ca_context *context;
};

struct private {
        ca_theme_data *theme;
        ca_bool_t signal_semaphore;
        sem_t semaphore;

        GstBus *mgr_bus;

        /* Everything below protected by the outstanding_mutex */
        ca_mutex *outstanding_mutex;
        ca_bool_t mgr_thread_running;
        ca_bool_t semaphore_allocated;
        CA_LLIST_HEAD(struct outstanding, outstanding);
};

#define PRIVATE(c) ((struct private *) ((c)->private))

static void* thread_func(void *userdata);
static void send_eos_msg(struct outstanding *out, int err);
static void send_mgr_exit_msg (struct private *p);

static void outstanding_free(struct outstanding *o) {
        GstBus *bus;

        ca_assert(o);

        if (o->pipeline) {
                bus = gst_pipeline_get_bus(GST_PIPELINE (o->pipeline));
                if (bus != NULL) {
                        gst_bus_set_sync_handler(bus, NULL, NULL, NULL);
                        gst_object_unref(bus);
                }

                gst_object_unref(GST_OBJECT(o->pipeline));
        }

        ca_free(o);
}

int driver_open(ca_context *c) {
        GError *error = NULL;
        struct private *p;
        pthread_t thread;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(!PRIVATE(c), CA_ERROR_INVALID);
        ca_return_val_if_fail(!c->driver || ca_streq(c->driver, "gstreamer"), CA_ERROR_NODRIVER);

        gst_init_check(NULL, NULL, &error);
        if (error != NULL) {
                g_warning("gst_init: %s ", error->message);
                g_error_free(error);
                return CA_ERROR_INVALID;
        }

        if (!(p = ca_new0(struct private, 1)))
                return CA_ERROR_OOM;
        c->private = p;

        if (!(p->outstanding_mutex = ca_mutex_new())) {
                driver_destroy(c);
                return CA_ERROR_OOM;
        }

        if (sem_init(&p->semaphore, 0, 0) < 0) {
                driver_destroy(c);
                return CA_ERROR_OOM;
        }
        p->semaphore_allocated = TRUE;

        p->mgr_bus = gst_bus_new();
        if (p->mgr_bus == NULL) {
                driver_destroy(c);
                return CA_ERROR_OOM;
        }
        gst_bus_set_flushing(p->mgr_bus, FALSE);

        /* Give a reference to the bus to the mgr thread */
        if (pthread_create(&thread, NULL, thread_func, p) < 0) {
                driver_destroy(c);
                return CA_ERROR_OOM;
        }
        p->mgr_thread_running = TRUE;

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
                        if (!out->dead)
                                send_eos_msg(out, CA_ERROR_DESTROYED);
                        out = out->next;
                }

                /* Now that we've sent EOS for all pending players, append a
                 * message to wait for the mgr thread to exit */
                if (p->mgr_thread_running && p->semaphore_allocated) {
                        send_mgr_exit_msg(p);

                        p->signal_semaphore = TRUE;
                        while (p->mgr_thread_running) {
                                ca_mutex_unlock(p->outstanding_mutex);
                                sem_wait(&p->semaphore);
                                ca_mutex_lock(p->outstanding_mutex);
                        }
                }

                ca_mutex_unlock(p->outstanding_mutex);
                ca_mutex_free(p->outstanding_mutex);
        }

        if (p->mgr_bus)
                g_object_unref(p->mgr_bus);

        if (p->theme)
                ca_theme_data_free(p->theme);

        if (p->semaphore_allocated)
                sem_destroy(&p->semaphore);

        ca_free(p);

        /* no gst_deinit(), see doc */

        return CA_SUCCESS;
}

int driver_change_device(ca_context *c, const char *device) {
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

static void
send_eos_msg(struct outstanding *out, int err) {
        struct private *p;
        GstMessage *m;
        GstStructure *s;

        out->dead = TRUE;
        out->err = err;

        p = PRIVATE(out->context);
        s = gst_structure_new("application/eos", "info", G_TYPE_POINTER, out, NULL);
        m = gst_message_new_application (GST_OBJECT (out->pipeline), s);

        gst_bus_post (p->mgr_bus, m);
}

static GstBusSyncReply
bus_cb(GstBus *bus, GstMessage *message, gpointer data) {
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
        case GST_MESSAGE_ERROR:
                err = CA_ERROR_SYSTEM;
                break;
        case GST_MESSAGE_EOS:
                /* only respect EOS from the toplevel pipeline */
                if (GST_OBJECT(out->pipeline) != GST_MESSAGE_SRC(message))
                        return GST_BUS_PASS;

                err = CA_SUCCESS;
                break;
        default:
                return GST_BUS_PASS;
        }

        /* Bin finished playback: ask the manager thread to shut it
         * down, since we can't from the sync message handler */
        ca_mutex_lock(p->outstanding_mutex);
        if (!out->dead)
                send_eos_msg(out, err);
        ca_mutex_unlock(p->outstanding_mutex);

        return GST_BUS_PASS;
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

        if (!(f = ca_new0(ca_sound_file, 1))) {
                close(fd);
                return CA_ERROR_OOM;
        }

        if (!(f->fdsrc = gst_element_factory_make("fdsrc", NULL))) {
                close(fd);
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

        sinkelement = GST_ELEMENT(data);

        caps = gst_pad_query_caps(pad, NULL);
        if (gst_caps_is_empty(caps) || gst_caps_is_any(caps)) {
                gst_caps_unref(caps);
                return;
        }

        structure = gst_caps_get_structure(caps, 0);
        type = gst_structure_get_name(structure);
        if (g_str_has_prefix(type, "audio/x-raw") == TRUE) {
                vpad = gst_element_get_static_pad(sinkelement, "sink");
                gst_pad_link(pad, vpad);
                gst_object_unref(vpad);
        }
        gst_caps_unref(caps);
}

static void
send_mgr_exit_msg (struct private *p) {
        GstMessage *m;
        GstStructure *s;

        s = gst_structure_new("application/mgr-exit", NULL);
        m = gst_message_new_application (NULL, s);

        gst_bus_post (p->mgr_bus, m);
}

/* Global manager thread that shuts down GStreamer pipelines when ordered */
static void* thread_func(void *userdata) {
        struct private *p = userdata;
        GstBus *bus = g_object_ref(p->mgr_bus);

        pthread_detach(pthread_self());

        /* Pop messages from the manager bus until we see an exit command */
        do {
                GstMessage *m = gst_bus_timed_pop(bus, GST_CLOCK_TIME_NONE);
                const GstStructure *s;
                const GValue *v;
                struct outstanding *out;

                if (m == NULL)
                        break;
                if (GST_MESSAGE_TYPE(m) != GST_MESSAGE_APPLICATION) {
                        gst_message_unref (m);
                        break;
                }

                s = gst_message_get_structure(m);
                if (gst_structure_has_name(s, "application/mgr-exit")) {
                        gst_message_unref (m);
                        break;
                }

                /* Otherwise, this must be an EOS message for an outstanding pipe */
                ca_assert(gst_structure_has_name(s, "application/eos"));
                v  = gst_structure_get_value(s, "info");
                ca_assert(v);
                out = g_value_get_pointer(v);
                ca_assert(out);

                /* Set pipeline back to NULL to close things. By the time this
                 * completes, we can be sure bus_cb won't be called */
                if (gst_element_set_state(out->pipeline, GST_STATE_NULL) ==
                    GST_STATE_CHANGE_FAILURE) {
                        gst_message_unref (m);
                        break;
                }
                if (out->callback)
                        out->callback(out->context, out->id, out->err, out->userdata);

                ca_mutex_lock(p->outstanding_mutex);
                CA_LLIST_REMOVE(struct outstanding, p->outstanding, out);
                outstanding_free(out);
                ca_mutex_unlock(p->outstanding_mutex);

                gst_message_unref(m);
        } while (TRUE);

        /* Signal the semaphore and exit */
        ca_mutex_lock(p->outstanding_mutex);
        if (p->signal_semaphore)
                sem_post(&p->semaphore);
        p->mgr_thread_running = FALSE;
        ca_mutex_unlock(p->outstanding_mutex);

        gst_bus_set_flushing(bus, TRUE);
        g_object_unref (bus);
        return NULL;
}


int driver_play(ca_context *c, uint32_t id, ca_proplist *proplist, ca_finish_callback_t cb, void *userdata) {
        struct private *p;
        struct outstanding *out;
        ca_sound_file *f;
        GstElement *decodebin, *sink, *audioconvert, *audioresample, *abin;
        GstBus *bus;
        GstPad *audiopad;
        int ret;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(proplist, CA_ERROR_INVALID);
        ca_return_val_if_fail(!userdata || cb, CA_ERROR_INVALID);

        out = NULL;
        f = NULL;
        sink = NULL;
        decodebin = NULL;
        audioconvert = NULL;
        audioresample = NULL;
        abin = NULL;
        p = PRIVATE(c);

        if ((ret = ca_lookup_sound_with_callback(&f, ca_gst_sound_file_open, NULL, &p->theme, c->props, proplist)) < 0)
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
            || !(sink = gst_element_factory_make("autoaudiosink", NULL))
            || !(abin = gst_bin_new ("audiobin"))) {

                /* At this point, if there is a failure, free each plugin separately. */
                if (out->pipeline != NULL)
                        g_object_unref (out->pipeline);
                if (decodebin != NULL)
                        g_object_unref(decodebin);
                if (audioconvert != NULL)
                        g_object_unref(audioconvert);
                if (audioresample != NULL)
                        g_object_unref(audioresample);
                if (sink != NULL)
                        g_object_unref(sink);
                if (abin != NULL)
                        g_object_unref(abin);

                ca_free(out);

                ret = CA_ERROR_OOM;
                goto fail;
        }

        bus = gst_pipeline_get_bus(GST_PIPELINE (out->pipeline));
        gst_bus_set_sync_handler(bus, bus_cb, out, NULL);
        gst_object_unref(bus);

        g_signal_connect(decodebin, "new-decoded-pad",
                         G_CALLBACK (on_pad_added), abin);
        gst_bin_add_many(GST_BIN (abin), audioconvert, audioresample, sink, NULL);
        gst_element_link_many(audioconvert, audioresample, sink, NULL);

        audiopad = gst_element_get_static_pad(audioconvert, "sink");
        gst_element_add_pad(abin, gst_ghost_pad_new("sink", audiopad));
        gst_object_unref(audiopad);

        gst_bin_add_many(GST_BIN (out->pipeline),
                         f->fdsrc, decodebin, abin, NULL);
        if (!gst_element_link(f->fdsrc, decodebin)) {
                /* Bin now owns the fdsrc... */
                f->fdsrc = NULL;

                outstanding_free(out);
                ret = CA_ERROR_OOM;
                goto fail;
        }
        /* Bin now owns the fdsrc... */
        f->fdsrc = NULL;

        ca_free(f);
        f = NULL;

        ca_mutex_lock(p->outstanding_mutex);
        CA_LLIST_PREPEND(struct outstanding, p->outstanding, out);
        ca_mutex_unlock(p->outstanding_mutex);

        if (gst_element_set_state(out->pipeline,
                                  GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
                ret = CA_ERROR_NOTAVAILABLE;
                goto fail;
        }

        return CA_SUCCESS;

fail:
        if (f && f->fdsrc)
                gst_object_unref(f->fdsrc);

        if (f)
                ca_free(f);

        return ret;
}

int driver_cancel(ca_context *c, uint32_t id) {
        struct private *p;
        struct outstanding *out = NULL;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(PRIVATE(c), CA_ERROR_STATE);

        p = PRIVATE(c);

        ca_mutex_lock(p->outstanding_mutex);

        for (out = p->outstanding; out;/* out = out->next*/) {
                struct outstanding *next;

                if (out->id != id || out->pipeline == NULL || out->dead == TRUE) {
                        out = out->next;
                        continue;
                }

                if (gst_element_set_state(out->pipeline, GST_STATE_NULL) ==
                    GST_STATE_CHANGE_FAILURE)
                        goto error;

                if (out->callback)
                        out->callback(c, out->id, CA_ERROR_CANCELED, out->userdata);
                next = out->next;
                CA_LLIST_REMOVE(struct outstanding, p->outstanding, out);
                outstanding_free(out);
                out = next;
        }

        ca_mutex_unlock(p->outstanding_mutex);

        return CA_SUCCESS;

error:
        ca_mutex_unlock(p->outstanding_mutex);
        return CA_ERROR_SYSTEM;
}

int driver_cache(ca_context *c, ca_proplist *proplist) {
        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(proplist, CA_ERROR_INVALID);
        ca_return_val_if_fail(PRIVATE(c), CA_ERROR_STATE);

        return CA_ERROR_NOTSUPPORTED;
}

int driver_playing(ca_context *c, uint32_t id, int *playing) {
        struct private *p;
        struct outstanding *out;

        ca_return_val_if_fail(c, CA_ERROR_INVALID);
        ca_return_val_if_fail(c->private, CA_ERROR_STATE);
        ca_return_val_if_fail(playing, CA_ERROR_INVALID);

        p = PRIVATE(c);

        *playing = 0;

        ca_mutex_lock(p->outstanding_mutex);

        for (out = p->outstanding; out; out = out->next) {

                if (out->id != id || out->pipeline == NULL || out->dead == TRUE)
                        continue;

                *playing = 1;
                break;
        }

        ca_mutex_unlock(p->outstanding_mutex);

        return CA_SUCCESS;
}
