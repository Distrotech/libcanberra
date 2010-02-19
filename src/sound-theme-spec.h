/*-*- Mode: C; c-basic-offset: 8 -*-*/

#ifndef foocanberrasoundthemespechfoo
#define foocanberrasoundthemespechfoo

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

#include "read-sound-file.h"
#include "proplist.h"

typedef struct ca_theme_data ca_theme_data;

typedef int (*ca_sound_file_open_callback_t)(ca_sound_file **f, const char *fn);

int ca_lookup_sound(ca_sound_file **f, char **sound_path, ca_theme_data **t, ca_proplist *cp, ca_proplist *sp);
int ca_lookup_sound_with_callback(ca_sound_file **f, ca_sound_file_open_callback_t sfopen, char **sound_path, ca_theme_data **t, ca_proplist *cp, ca_proplist *sp);
void ca_theme_data_free(ca_theme_data *t);

int ca_get_data_home(char **e);
const char *ca_get_data_dirs(void);

#endif
