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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "canberra-gtk.h"

static guint
    signal_id_message_dialog_show,
    signal_id_dialog_response,
    signal_id_menu_show,
    signal_id_menu_hide,
    signal_id_check_menu_item_toggled,
    signal_id_menu_item_activate,
    signal_id_toggle_button_toggled,
    signal_id_button_clicked;

static const char *translate_message_tye(GtkMessageType mt) {
    static const char *const message_type_table[] = {
        [GTK_MESSAGE_INFO] = "dialog-information",
        [GTK_MESSAGE_WARNING] = "dialog-warning",
        [GTK_MESSAGE_QUESTION] = "dialog-question",
        [GTK_MESSAGE_ERROR] = "dialog-error",
        [GTK_MESSAGE_OTHER] = NULL
    };

    if (mt >= G_N_ELEMENTS(message_type_table))
        return NULL;

    return message_type_table[mt];
}

static const char *translate_response(int response) {
    static const char *const response_table[] = {
        [-GTK_RESPONSE_NONE] = NULL,
        [-GTK_RESPONSE_REJECT] = "dialog-cancel",
        [-GTK_RESPONSE_DELETE_EVENT] = "dialog-cancel",
        [-GTK_RESPONSE_ACCEPT] = "dialog-ok",
        [-GTK_RESPONSE_OK] = "dialog-ok",
        [-GTK_RESPONSE_CANCEL] = "dialog-cancel",
        [-GTK_RESPONSE_CLOSE] = "dialog-ok",
        [-GTK_RESPONSE_YES] = "dialog-ok",
        [-GTK_RESPONSE_NO] = "dialog-cancel",
        [-GTK_RESPONSE_APPLY] = "dialog-ok",
        [-GTK_RESPONSE_HELP] = NULL,
    };

    if (response >= 0)
        return NULL;

    if ((unsigned) -response >= G_N_ELEMENTS(response_table))
        return NULL;

    return response_table[-response];
}

static gboolean is_child_of_combo_box(GtkWidget *w) {

    while (w) {

        if (GTK_IS_COMBO_BOX(w))
            return TRUE;

        w = gtk_widget_get_parent(w);
    }

    return FALSE;
}

static GtkDialog* find_parent_dialog(GtkWidget *w) {

    while (w) {

        if (GTK_IS_DIALOG(w))
            return GTK_DIALOG(w);

        w = gtk_widget_get_parent(w);
    }

    return NULL;
}

static gboolean emission_hook_cb(GSignalInvocationHint *hint, guint n_param_values, const GValue *param_values, gpointer data) {
    int ret = CA_SUCCESS;
    static GQuark disable_sound_quark = 0;
    GObject *object;

    /* This is the same quark used by libgnomeui! */
    if (!disable_sound_quark)
        disable_sound_quark = g_quark_from_static_string("gnome_disable_sound_events");

    object = g_value_get_object(&param_values[0]);

    if (g_object_get_qdata(object, disable_sound_quark))
        return TRUE;

    if (!GTK_IS_WIDGET(object))
        return TRUE;

    if (!GTK_WIDGET_DRAWABLE(object))
        return TRUE;

/*     g_message("signal %s on %s", g_signal_name(hint->signal_id), g_type_name(G_OBJECT_TYPE(object))); */

    if (GTK_IS_MESSAGE_DIALOG(object) && hint->signal_id == signal_id_message_dialog_show) {
        GtkMessageType mt;
        const char *id;

        g_object_get(object, "message_type", &mt, NULL);

        if ((id = translate_message_tye(mt)))
            ret = ca_gtk_play_for_widget(GTK_WIDGET(object), 0,
                                         CA_PROP_EVENT_ID, id,
                                         CA_PROP_EVENT_DESCRIPTION, "Message dialog shown",
                                         CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                                         NULL);

        /* Hmm, why do we get two of these? */
    }

    if (GTK_IS_DIALOG(object) && hint->signal_id == signal_id_dialog_response) {

        int response;
        const char *id;

        response = g_value_get_int(&param_values[1]);

        if ((id = translate_response(response))) {

            ret = ca_gtk_play_for_widget(GTK_WIDGET(object), 0,
                                         CA_PROP_EVENT_ID, id,
                                         CA_PROP_EVENT_DESCRIPTION, "Dialog closed",
                                         CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                                         NULL);
        }
    }

    if (GTK_IS_MENU(object)) {

        /* This doesn't work */

        if (hint->signal_id == signal_id_menu_show)
            ret = ca_gtk_play_for_widget(GTK_WIDGET(object), 0,
                                         CA_PROP_EVENT_ID, "menu-popup",
                                         CA_PROP_EVENT_DESCRIPTION, "Menu popped up",
                                         CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                                         NULL);
        else if (hint->signal_id == signal_id_menu_hide)
            ret = ca_gtk_play_for_widget(GTK_WIDGET(object), 0,
                                         CA_PROP_EVENT_ID, "menu-popdown",
                                         CA_PROP_EVENT_DESCRIPTION, "Menu popped up",
                                         CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                                         NULL);
    }

    if (GTK_IS_CHECK_MENU_ITEM(object) && hint->signal_id == signal_id_check_menu_item_toggled) {

        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(object)))
            ret = ca_gtk_play_for_widget(GTK_WIDGET(object), 0,
                                         CA_PROP_EVENT_ID, "button-toggle-on",
                                         CA_PROP_EVENT_DESCRIPTION, "Check menu item checked",
                                         CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                                         NULL);
        else
            ret = ca_gtk_play_for_widget(GTK_WIDGET(object), 0,
                                         CA_PROP_EVENT_ID, "button-toggle-off",
                                         CA_PROP_EVENT_DESCRIPTION, "Check menu item unchecked",
                                         CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                                         NULL);

    } else if (GTK_IS_MENU_ITEM(object) && hint->signal_id == signal_id_menu_item_activate) {

        if (!GTK_MENU_ITEM(object)->submenu)
            ret = ca_gtk_play_for_widget(GTK_WIDGET(object), 0,
                                         CA_PROP_EVENT_ID, "menu-click",
                                         CA_PROP_EVENT_DESCRIPTION, "Menu item clicked",
                                         CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                                         NULL);
    }

    if (GTK_IS_TOGGLE_BUTTON(object) && hint->signal_id == signal_id_toggle_button_toggled) {


        if (!is_child_of_combo_box(GTK_WIDGET(object))) {

            /* We don't want to play this sound if this is a toggle
             * button belonging to combo box. */

            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object)))
                ret = ca_gtk_play_for_widget(GTK_WIDGET(object), 0,
                                             CA_PROP_EVENT_ID, "button-toggle-on",
                                             CA_PROP_EVENT_DESCRIPTION, "Toggle button checked",
                                             CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                                             NULL);
            else
                ret = ca_gtk_play_for_widget(GTK_WIDGET(object), 0,
                                             CA_PROP_EVENT_ID, "button-toggle-off",
                                             CA_PROP_EVENT_DESCRIPTION, "Toggle button unchecked",
                                             CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                                             NULL);
        }

    } else if (GTK_IS_BUTTON(object) && !GTK_IS_TOGGLE_BUTTON(object) && hint->signal_id == signal_id_button_clicked) {
        GtkDialog *dialog;
        gboolean dont_play = FALSE;

        if ((dialog = find_parent_dialog(GTK_WIDGET(object)))) {
            int response;

            /* Don't play the click sound if this is a response widget
             * we will generate a dialog-xxx event sound anyway. */

            response = gtk_dialog_get_response_for_widget(dialog, GTK_WIDGET(object));
            dont_play = !!translate_response(response);
        }

        if (!dont_play)
            ret = ca_gtk_play_for_widget(GTK_WIDGET(object), 0,
                                         CA_PROP_EVENT_ID, "button-click",
                                         CA_PROP_EVENT_DESCRIPTION, "Button clicked",
                                         CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                                         NULL);
    }

    if (ret != CA_SUCCESS)
        g_warning("Failed to play event sound: %s", ca_strerror(ret));

    return TRUE;
}


static void install_hook(GType type, const char *signal, guint *sn) {

    /* I've no idea what this is for. libgnomeui does it, and thus we
     * do it too. Someone with more gobject-fu should say something
     * about this */
    g_type_class_unref(g_type_class_ref(type));

    *sn = g_signal_lookup(signal, type);
    g_signal_add_emission_hook(*sn, 0, emission_hook_cb, NULL, NULL);
}

void gtk_module_init(gint *argc, gchar ***argv[]);

void gtk_module_init(gint *argc, gchar ***argv[]) {
    install_hook(GTK_TYPE_MESSAGE_DIALOG, "show", &signal_id_message_dialog_show);
    install_hook(GTK_TYPE_DIALOG, "response", &signal_id_dialog_response);
    install_hook(GTK_TYPE_MENU, "show", &signal_id_menu_show);
    install_hook(GTK_TYPE_MENU, "hide", &signal_id_menu_hide);
    install_hook(GTK_TYPE_MENU_ITEM, "activate", &signal_id_menu_item_activate);
    install_hook(GTK_TYPE_CHECK_MENU_ITEM, "toggled", &signal_id_check_menu_item_toggled);
    install_hook(GTK_TYPE_TOGGLE_BUTTON, "toggled", &signal_id_toggle_button_toggled);
    install_hook(GTK_TYPE_BUTTON, "clicked", &signal_id_button_clicked);
}
