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
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "canberra.h"
#include "canberra-gtk.h"
#include "common.h"
#include "malloc.h"

ca_context *ca_gtk_context_get(void) {
    static GStaticPrivate context_private = G_STATIC_PRIVATE_INIT;
    ca_context *c = NULL;
    const char *name;

    if ((c = g_static_private_get(&context_private)))
        return c;

    ca_assert_se(ca_context_create(&c) == CA_SUCCESS);

    if ((name = g_get_application_name()))
        ca_assert_se(ca_context_change_props(c, CA_PROP_APPLICATION_NAME, name, NULL) == 0);

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

int ca_gtk_proplist_set_for_window(ca_proplist *p, GtkWidget *widget) {
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
            if ((ret = ca_gtk_proplist_set_for_window(p, w)) < 0)
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

            if ((ret = ca_proplist_setf(p, CA_PROP_EVENT_MOUSE_HPOS, "%0.0f", x/width)) < 0)
                return ret;

            if ((ret = ca_proplist_setf(p, CA_PROP_EVENT_MOUSE_VPOS, "%0.0f", y/height)) < 0)
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

int ca_gtk_play_for_window(GtkWidget *w, uint32_t id, ...) {
    va_list ap;
    int ret;
    ca_proplist *p;

    ca_return_val_if_fail(w, CA_ERROR_INVALID);

    if ((ret = ca_proplist_create(&p)) < 0)
        return ret;

    if ((ret = ca_gtk_proplist_set_for_window(p, w)) < 0)
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
