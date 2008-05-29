#ifndef foocanberragtkhfoo
#define foocanberragtkhfoo

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

#include <canberra.h>
#include <gtk/gtk.h>

ca_context *ca_gtk_context_get(void);

int ca_gtk_play_for_window(GtkWidget *w, uint32_t id, ...) CA_GCC_SENTINEL;

int ca_gtk_play_for_event(GdkEvent *e, uint32_t id, ...) CA_GCC_SENTINEL;

int ca_gtk_proplist_set_for_window(ca_proplist *p, GtkWidget *w);

int ca_gtk_proplist_set_for_event(ca_proplist *p, GdkEvent *e);

#endif
