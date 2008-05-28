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

static GtkWidget* get_toplevel(GtkWidget *w) {
    while (w) {
        if (GTK_WIDGET_TOPLEVEL(w))
            return w;

        w = gtk_widget_get_parent(w);
    }

    return NULL;
};

int ca_gtk_proplist_set_for_window(ca_proplist *p, GtkWidget *w) {
    int ret;
    const char *t;

    ca_return_val_if_fail(p, CA_ERROR_INVALID);
    ca_return_val_if_fail(w, CA_ERROR_INVALID);

    if (!(w = get_toplevel(w)))
        return CA_ERROR_INVALID;

    if ((t = gtk_window_get_title(w)))
        if ((ret = ca_proplist_sets(p, CA_PROP_WINDOW_NAME, t)) < 0)
            return ret;

    if ((t = gtk_window_get_icon_name(w)))
        if ((ret = ca_proplist_sets(p, CA_PROP_WINDOW_NAME, t)) < 0)
            return ret;



}




const char *ca_gtk_window_display(GtkWidget *w) {
}

const char *ca_gtk_window_xid(GtkWidget *w) {
}
