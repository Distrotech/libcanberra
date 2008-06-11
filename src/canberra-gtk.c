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
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "canberra.h"
#include "canberra-gtk.h"
#include "common.h"
#include "malloc.h"

/**
 * SECTION:canberra-gtk
 * @short_description: Gtk+ libcanberra Bindings
 *
 * libcanberra-gtk provides a few functions that simplify libcanberra
 * usage from Gtk+ programs. It maintains a single application-global
 * ca_context object that is made accessible via
 * ca_gtk_context_get(). More importantly, it provides a few functions
 * to compile event sound property lists based on GtkWidget objects or
 * GdkEvent events.
 */

/**
 * ca_gtk_context_get:
 *
 * libcanberra-gtk maintains a single application-global ca_context
 * object. Use this function to access it. The
 * %CA_PROP_CANBERRA_XDG_THEME_NAME of this context property is
 * dynamically bound to the XSETTINGS setting for the XDG theme
 * name. CA_PROP_APPLICATION_NAME is bound to
 * g_get_application_name().
 *
 * Returns: a pa_context object
 */

ca_context *ca_gtk_context_get(void) {
    static GStaticPrivate context_private = G_STATIC_PRIVATE_INIT;
    ca_context *c = NULL;
    const char *name;
    GValue value;

    if ((c = g_static_private_get(&context_private)))
        return c;

    ca_assert_se(ca_context_create(&c) == CA_SUCCESS);

    if ((name = g_get_application_name()))
        ca_assert_se(ca_context_change_props(c, CA_PROP_APPLICATION_NAME, name, NULL) == 0);

    if (gdk_screen_get_setting(gdk_screen_get_default(), "xdg-sound-theme", &value)) {
        const char *t;

        /* FIXME, this needs more love, we need to subscribe to theme changes */

        if ((t = g_value_get_string(&value)))
            ca_assert_se(ca_context_change_props(c, CA_PROP_CANBERRA_XDG_THEME_NAME, t, NULL) == 0);

        g_value_unset(&value);
    }

    if (gdk_screen_get_setting(gdk_screen_get_default(), "xdg-sound-theme-output-profile", &value)) {
        const char *t;

        /* FIXME, this needs more love, we need to subscribe to theme changes */

        if ((t = g_value_get_string(&value)))
            ca_assert_se(ca_context_change_props(c, CA_PROP_CANBERRA_XDG_THEME_OUTPUT_PROFILE, t, NULL) == 0);

        g_value_unset(&value);
    }

    g_static_private_set(&context_private, c, (GDestroyNotify) ca_context_destroy);

    return c;
}

static GtkWindow* get_toplevel(GtkWidget *w) {
    if (!(w = gtk_widget_get_toplevel(w)))
        return NULL;

    if (!GTK_IS_WINDOW(w))
        return NULL;

    return GTK_WINDOW(w);
}

/**
 * ca_gtk_proplist_set_for_widget:
 * @p: The proplist to store these sound event properties in
 * @w: The Gtk widget to base these sound event properties on
 *
 * Fill in a ca_proplist object for a sound event that shall originate
 * from the specified Gtk Widget. This will fill in properties like
 * %CA_PROP_WINDOW_NAME or %CA_PROP_WINDOW_X11_DISPLAY for you.
 *
 * Returns: 0 on success, negative error code on error.
 */

int ca_gtk_proplist_set_for_widget(ca_proplist *p, GtkWidget *widget) {
    GtkWindow *w;
    int ret;
    const char *t, *role;
    GdkWindow *dw;
    GdkScreen *screen;

    ca_return_val_if_fail(p, CA_ERROR_INVALID);
    ca_return_val_if_fail(widget, CA_ERROR_INVALID);

    if (!(w = get_toplevel(widget)))
        return CA_ERROR_INVALID;

    if ((t = gtk_window_get_title(w)))
        if ((ret = ca_proplist_sets(p, CA_PROP_WINDOW_NAME, t)) < 0)
            return ret;

    if ((role = gtk_window_get_role(w))) {
        if (role && t) {
            char *id = ca_sprintf_malloc("%s#%s", t, role);

            if ((ret = ca_proplist_sets(p, CA_PROP_WINDOW_ID, id)) < 0) {
                ca_free(id);
                return ret;
            }

            ca_free(id);
        }
    } else if (t)
        if ((ret = ca_proplist_sets(p, CA_PROP_WINDOW_ID, t)) < 0)
            return ret;

    if ((t = gtk_window_get_icon_name(w)))
        if ((ret = ca_proplist_sets(p, CA_PROP_WINDOW_ICON_NAME, t)) < 0)
            return ret;

    if ((dw = GTK_WIDGET(w)->window))
        if ((ret = ca_proplist_setf(p, CA_PROP_WINDOW_X11_XID, "%lu", (unsigned long) GDK_WINDOW_XID(dw))) < 0)
            return ret;

    if ((screen = gtk_widget_get_screen(widget))) {

        if ((t = gdk_display_get_name(gdk_screen_get_display(screen))))
            if ((ret = ca_proplist_sets(p, CA_PROP_WINDOW_X11_DISPLAY, t)) < 0)
                return ret;

        if ((ret = ca_proplist_setf(p, CA_PROP_WINDOW_X11_SCREEN, "%i", gdk_screen_get_number(screen))) < 0)
            return ret;

        if (dw)
            if ((ret = ca_proplist_setf(p, CA_PROP_WINDOW_X11_MONITOR, "%i", gdk_screen_get_monitor_at_window(screen, dw))) < 0)
                return ret;
    }

    return CA_SUCCESS;
}

/**
 * ca_gtk_proplist_set_for_event:
 * @p: The proplist to store these sound event properties in
 * @e: The Gdk event to base these sound event properties on
 *
 * Fill in a ca_proplist object for a sound event that is being
 * triggered by the specified Gdk Event. This will fill in properties
 * like %CA_PROP_EVENT_MOUSE_X or %CA_PROP_EVENT_MOUSE_BUTTON for
 * you. This will internally also cal ca_gtk_proplist_set_for_widget()
 * on the widget this event belongs to.
 *
 * Returns: 0 on success, negative error code on error.
 */

int ca_gtk_proplist_set_for_event(ca_proplist *p, GdkEvent *e) {
    gdouble x, y;
    GdkWindow *gw;
    GtkWidget *w = NULL;
    int ret;

    ca_return_val_if_fail(p, CA_ERROR_INVALID);
    ca_return_val_if_fail(e, CA_ERROR_INVALID);

    if ((gw = e->any.window)) {
        gdk_window_get_user_data(gw, (gpointer*) &w);

        if (w)
            if ((ret = ca_gtk_proplist_set_for_widget(p, w)) < 0)
                return ret;
    }

    if (gdk_event_get_root_coords(e, &x, &y)) {

        if ((ret = ca_proplist_setf(p, CA_PROP_EVENT_MOUSE_X, "%0.0f", x)) < 0)
            return ret;

        if ((ret = ca_proplist_setf(p, CA_PROP_EVENT_MOUSE_Y, "%0.0f", y)) < 0)
            return ret;

        if (w)  {
            int width, height;

            width = gdk_screen_get_width(gtk_widget_get_screen(w));
            height = gdk_screen_get_height(gtk_widget_get_screen(w));

            /* We use these strange format strings here to avoid that
             * libc applies locale information on the formatting of
             * floating numbers. */

            if ((ret = ca_proplist_setf(p, CA_PROP_EVENT_MOUSE_HPOS, "%i.%03i", (int) (x/width), (int) (1000.0*x/width) % 1000)) < 0)
                return ret;

            if ((ret = ca_proplist_setf(p, CA_PROP_EVENT_MOUSE_VPOS, "%i.%03i", (int) (y/height), (int) (1000.0*y/height) % 1000)) < 0)
                return ret;
        }
    }

    if (e->type == GDK_BUTTON_PRESS ||
        e->type == GDK_2BUTTON_PRESS ||
        e->type == GDK_3BUTTON_PRESS ||
        e->type == GDK_BUTTON_RELEASE) {

        if ((ret = ca_proplist_setf(p, CA_PROP_EVENT_MOUSE_BUTTON, "%u", e->button.button)) < 0)
            return ret;
    }

    return CA_SUCCESS;
}

/**
 * ca_gtk_play_for_widget:
 * @w: The Gtk widget to base these sound event properties on
 * @id: The event id that can later be used to cancel this event sound
 * using ca_context_cancel(). This can be any integer and shall be
 * chosen be the client program. It is a good idea to pass 0 here if
 * cancelling the sound later is not needed. If the same id is passed
 * to multiple sounds they can be canceled with a single
 * ca_context_cancel() call.
 * @...: additional event properties as pairs of strings, terminated by NULL.
 *
 * Play a sound event for the specified widget. This will internally
 * call ca_gtk_proplist_set_for_widget() and then merge them with the
 * properties passed in via the NULL terminated argument
 * list. Finally, it will call ca_context_play_full() to actually play
 * the event sound.
 *
 * Returns: 0 on success, negative error code on error.
 */

int ca_gtk_play_for_widget(GtkWidget *w, uint32_t id, ...) {
    va_list ap;
    int ret;
    ca_proplist *p;

    ca_return_val_if_fail(w, CA_ERROR_INVALID);

    if ((ret = ca_proplist_create(&p)) < 0)
        return ret;

    if ((ret = ca_gtk_proplist_set_for_widget(p, w)) < 0)
        goto fail;

    va_start(ap, id);
    ret = ca_proplist_merge_ap(p, ap);
    va_end(ap);

    if (ret < 0)
        goto fail;

    ret = ca_context_play_full(ca_gtk_context_get(), id, p, NULL, NULL);

fail:

    ca_assert_se(ca_proplist_destroy(p) == 0);

    return ret;
}

/**
 * ca_gtk_play_for_event:
 * @e: The Gdk event to base these sound event properties on
 * @id: The event id that can later be used to cancel this event sound
 * using ca_context_cancel(). This can be any integer and shall be
 * chosen be the client program. It is a good idea to pass 0 here if
 * cancelling the sound later is not needed. If the same id is passed
 * to multiple sounds they can be canceled with a single
 * ca_context_cancel() call.
 * @...: additional event properties as pairs of strings, terminated by NULL.
 *
 * Play a sound event for the specified event. This will internally
 * call ca_gtk_proplist_set_for_event() and then merge them with the
 * properties passed in via the NULL terminated argument
 * list. Finally, it will call ca_context_play_full() to actually play
 * the event sound.
 *
 * Returns: 0 on success, negative error code on error.
 */

int ca_gtk_play_for_event(GdkEvent *e, uint32_t id, ...) {
    va_list ap;
    int ret;
    ca_proplist *p;

    ca_return_val_if_fail(e, CA_ERROR_INVALID);

    if ((ret = ca_proplist_create(&p)) < 0)
        return ret;

    if ((ret = ca_gtk_proplist_set_for_event(p, e)) < 0)
        goto fail;

    va_start(ap, id);
    ret = ca_proplist_merge_ap(p, ap);
    va_end(ap);

    if (ret < 0)
        goto fail;

    ret = ca_context_play_full(ca_gtk_context_get(), id, p, NULL, NULL);

fail:

    ca_assert_se(ca_proplist_destroy(p) == 0);

    return ret;
}

/**
 * ca_gtk_widget_disable_sounds:
 * @w: The Gtk widget to disable automatic event sounds for.
 * @enable: Boolean specifying whether sound events shall be enabled or disabled for this widget.
 *
 * By default sound events are automatically generated for all kinds
 * of input events. Use this function to disable this. This is
 * intended to be used for widgets which directly generate sound
 * events.
 */

void ca_gtk_widget_disable_sounds(GtkWidget *w, gboolean enable) {
    static GQuark disable_sound_quark = 0;

    /* This is the same quark used by libgnomeui! */
    if (!disable_sound_quark)
        disable_sound_quark = g_quark_from_static_string("gnome_disable_sound_events");

    g_object_set_qdata(G_OBJECT(w), disable_sound_quark, GINT_TO_POINTER(!!enable));
}
