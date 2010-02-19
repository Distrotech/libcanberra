/*-*- Mode: C; c-basic-offset: 8 -*-*/

#ifndef foocanberrareadwavhfoo
#define foocanberrareadwavhfoo

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

#include <stdio.h>

#include "read-sound-file.h"

typedef struct ca_wav ca_wav;

int ca_wav_open(ca_wav **v, FILE *f);
void ca_wav_close(ca_wav *f);

unsigned ca_wav_get_nchannels(ca_wav *f);
unsigned ca_wav_get_rate(ca_wav *f);
ca_sample_type_t ca_wav_get_sample_type(ca_wav *f);
const ca_channel_position_t* ca_wav_get_channel_map(ca_wav *f);

int ca_wav_read_u8(ca_wav *f, uint8_t *d, size_t *n);
int ca_wav_read_s16le(ca_wav *f, int16_t *d, size_t *n);

off_t ca_wav_get_size(ca_wav *f);

#endif
