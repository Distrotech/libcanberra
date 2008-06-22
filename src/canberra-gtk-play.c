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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <canberra-gtk.h>
#include <locale.h>

static gboolean idle_quit(gpointer userdata) {
    gtk_main_quit ();
    return FALSE;
}

static void callback(ca_context *c, uint32_t id, int error, void *userdata) {
    int *ret = userdata;

    if (error < 0) {
        g_printerr("Failed to play sound: %s\n", ca_strerror(error));
        *ret = 1;
    }

    /* So, why don't we call gtk_main_quit() here directly? -- Because
     * otherwise we might end up with a small race condition: this
     * callback might get called before the main loop actually started
     * running */
    g_idle_add(idle_quit, NULL);
}

int main (int argc, char *argv[]) {
    GOptionContext *oc;
    ca_proplist *p;
    static gchar *event_id = NULL, *event_description = NULL;
    int ret = 0, r;

    static const GOptionEntry options[] = {
        { "id",          0, 0, G_OPTION_ARG_STRING, &event_id,          "Event sound identifier",  "STRING" },
        { "description", 0, 0, G_OPTION_ARG_STRING, &event_description, "Event sound description", "STRING" },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    setlocale(LC_ALL, "");

    g_type_init();
    g_thread_init(NULL);

    oc = g_option_context_new("- libcanberra-gtk-play");
    g_option_context_add_main_entries(oc, options, NULL);
    g_option_context_set_help_enabled(oc, TRUE);
    g_option_context_parse(oc, &argc, &argv, NULL);
    g_option_context_free(oc);

    gtk_init(&argc, &argv);

    if (!event_id) {
        g_printerr("No event id specified.\n");
        return 1;
    }

    ca_context_change_props(ca_gtk_context_get(),
                            CA_PROP_APPLICATION_NAME, "canberra-gtk-play",
                            CA_PROP_APPLICATION_ID, "org.freedesktop.libcanberra.GtkPlay",
                            NULL);

    ca_proplist_create(&p);
    ca_proplist_sets(p, CA_PROP_EVENT_ID, event_id);

    if (event_description)
        ca_proplist_sets(p, CA_PROP_EVENT_DESCRIPTION, event_description);

    r = ca_context_play_full(ca_gtk_context_get(), 1, p, callback, &ret);
    ca_proplist_destroy(p);

    if (r < 0) {
        g_printerr("Failed to play sound: %s\n", ca_strerror(r));
        return 1;
    }

    gtk_main();

    return ret;
}
