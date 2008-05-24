#ifndef foocasoundthemespechfoo
#define foocasoundthemespechfoo

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

#include "read-sound-file.h"
#include "proplist.h"

typedef struct ca_theme_data ca_theme_data;

int ca_lookup_sound(ca_sound_file **f, ca_theme_data **t, ca_proplist *p);
void ca_theme_data_free(ca_theme_data *t);

#endif
