#ifndef foocanberrareadvorbishfoo
#define foocanberrareadvorbishfoo

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

#include <stdio.h>
#include <inttypes.h>

typedef struct ca_vorbis ca_vorbis;

int ca_vorbis_open(ca_vorbis **v, FILE *f);
void ca_vorbis_close(ca_vorbis *v);

unsigned ca_vorbis_get_nchannels(ca_vorbis *v);
unsigned ca_vorbis_get_rate(ca_vorbis *v);

int ca_vorbis_read_s16ne(ca_vorbis *v, int16_t *d, unsigned *n);

size_t ca_vorbis_get_size(ca_vorbis *f);

#endif
