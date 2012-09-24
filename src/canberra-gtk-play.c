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

#include <string.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <canberra-gtk.h>

static int ret = 0;
static ca_proplist *proplist = NULL;
static int n_loops = 1;

static void callback(ca_context *c, uint32_t id, int error, void *userdata);

static gboolean idle_quit(gpointer userdata) {
        gtk_main_quit();
        return FALSE;
}

static gboolean idle_play(gpointer userdata) {
        int r;

        g_assert(n_loops > 1);

        n_loops--;

        r = ca_context_play_full(ca_gtk_context_get(), 1, proplist, callback, NULL);

        if (r < 0) {
                g_printerr("Failed to play sound: %s\n", ca_strerror(r));
                ret = 1;
                gtk_main_quit();
        }

        return FALSE;
}

static void callback(ca_context *c, uint32_t id, int error, void *userdata) {

        if (error < 0) {
                g_printerr("Failed to play sound (callback): %s\n", ca_strerror(error));
                ret = 1;

        } else if (n_loops > 1) {
                /* So, why don't we call ca_context_play_full() here directly?
                   -- Because the context this callback is called from is
                   explicitly documented as undefined and no libcanberra function
                   may be called from it. */

                g_idle_add(idle_play, NULL);
                return;
        }

        /* So, why don't we call gtk_main_quit() here directly? -- Because
         * otherwise we might end up with a small race condition: this
         * callback might get called before the main loop actually started
         * running */
        g_idle_add(idle_quit, NULL);
}

static GQuark error_domain(void) {
        return g_quark_from_static_string("canberra-error-quark");
}

static gboolean property_callback(
                const gchar *option_name,
                const gchar *value,
                gpointer data,
                GError **error) {

        const char *equal;
        char *t;

        if (!(equal = strchr(value, '='))) {
                g_set_error(error, error_domain(), 0, "Property lacks '='.");
                return FALSE;
        }

        t = g_strndup(value, equal - value);

        if (ca_proplist_sets(proplist, t, equal + 1) < 0) {
                g_set_error(error, error_domain(), 0, "Invalid property.");
                g_free(t);
                return FALSE;
        }

        g_free(t);
        return TRUE;
}

int main (int argc, char *argv[]) {
        GOptionContext *oc;
        static gchar *event_id = NULL, *filename = NULL, *event_description = NULL, *cache_control = NULL, *volume = NULL;
        int r;
        static gboolean version = FALSE;
        GError *error = NULL;

        static const GOptionEntry options[] = {
                { "version",       'v', 0, G_OPTION_ARG_NONE,     &version,                  "Display version number and quit", NULL },
                { "id",            'i', 0, G_OPTION_ARG_STRING,   &event_id,                 "Event sound identifier",  "STRING" },
                { "file",          'f', 0, G_OPTION_ARG_STRING,   &filename,                 "Play file",  "PATH" },
                { "description",   'd', 0, G_OPTION_ARG_STRING,   &event_description,        "Event sound description", "STRING" },
                { "cache-control", 'c', 0, G_OPTION_ARG_STRING,   &cache_control,            "Cache control (permanent, volatile, never)", "STRING" },
                { "loop",          'l', 0, G_OPTION_ARG_INT,      &n_loops,                  "Loop how many times (detault: 1)", "INTEGER" },
                { "volume",        'V', 0, G_OPTION_ARG_STRING,   &volume,                   "A floating point dB value for the sample volume (ex: 0.0)", "STRING" },
                { "property",      0,   0, G_OPTION_ARG_CALLBACK, (void*) property_callback, "An arbitrary property", "STRING" },
                { NULL, 0, 0, 0, NULL, NULL, NULL }
        };

        setlocale(LC_ALL, "");

        g_type_init();

        ca_proplist_create(&proplist);

        oc = g_option_context_new("- canberra-gtk-play");
        g_option_context_add_main_entries(oc, options, NULL);
        g_option_context_add_group(oc, gtk_get_option_group(TRUE));
        g_option_context_set_help_enabled(oc, TRUE);

        if (!(g_option_context_parse(oc, &argc, &argv, &error))) {
                g_print("Option parsing failed: %s\n", error->message);
                return 1;
        }
        g_option_context_free(oc);

        if (version) {
                g_print("canberra-gtk-play from %s\n", PACKAGE_STRING);
                return 0;
        }

        if (!event_id && !filename) {
                g_printerr("No event id or file specified.\n");
                return 1;
        }

        ca_context_change_props(ca_gtk_context_get(),
                                CA_PROP_APPLICATION_NAME, "canberra-gtk-play",
                                CA_PROP_APPLICATION_VERSION, PACKAGE_VERSION,
                                CA_PROP_APPLICATION_ID, "org.freedesktop.libcanberra.gtk-play",
                                NULL);

        if (event_id)
                ca_proplist_sets(proplist, CA_PROP_EVENT_ID, event_id);

        if (filename)
                ca_proplist_sets(proplist, CA_PROP_MEDIA_FILENAME, filename);

        if (cache_control)
                ca_proplist_sets(proplist, CA_PROP_CANBERRA_CACHE_CONTROL, cache_control);

        if (event_description)
                ca_proplist_sets(proplist, CA_PROP_EVENT_DESCRIPTION, event_description);

        if (volume)
                ca_proplist_sets(proplist, CA_PROP_CANBERRA_VOLUME, volume);

        r = ca_context_play_full(ca_gtk_context_get(), 1, proplist, callback, NULL);

        if (r < 0) {
                g_printerr("Failed to play sound: %s\n", ca_strerror(r));
                ret = 1;
                goto finish;
        }

        gtk_main();

finish:

        ca_proplist_destroy(proplist);

        return ret;
}
