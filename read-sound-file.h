#ifndef foocareadsoundfilehfoo
#define foocareadsoundfilehfoo

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

typedef enum ca_sample_type {
    CA_SAMPLE_S16NE,
    CA_SAMPLE_S16RE,
    CA_SAMPLE_U8
} ca_sample_type_t;

typedef struct ca_sound_file wa_sound_file;

int ca_sound_file_open(ca_sound_file *f, const char *fn);
void ca_sound_file_close(ca_sound_file *f);

unsigned ca_sound_file_get_nchannels(ca_sound_file *f);
unsigned ca_sound_file_get_rate(ca_sound_file *f);
ca_sample_type_t ca_sound_file_get_sample_type(ca_sound_file *f);

int ca_sound_file_read_int16(ca_sound_file *f, int16_t *d, unsigned *n);
int ca_sound_file_read_uint8(ca_sound_file *f, uint8_t *d, unsigned *n);

#endif
