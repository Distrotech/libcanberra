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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "canberra.h"
#include "read-wav.h"
#include "macro.h"
#include "malloc.h"

#define FILE_SIZE_MAX (64U*1024U*1024U)

struct ca_wav {
    FILE *file;

    off_t data_size;
    unsigned nchannels;
    unsigned rate;
    unsigned depth;
};

static int skip_to_chunk(ca_wav *w, uint32_t id, uint32_t *size) {

    ca_return_val_if_fail(w, CA_ERROR_INVALID);
    ca_return_val_if_fail(size, CA_ERROR_INVALID);

    for (;;) {
        uint32_t chunk[2];
        uint32_t s;

        if (fread(chunk, sizeof(uint32_t), CA_ELEMENTSOF(chunk), w->file) != CA_ELEMENTSOF(chunk))
            goto fail_io;

        s = PA_UINT32_FROM_LE(chunk[1]);

        if (s <= 0 || s >= FILE_SIZE_MAX)
            return CA_ERROR_TOOBIG;

        if (PA_UINT32_FROM_LE(chunk[0]) == id) {
            *size = s;
            break;
        }

        if (fseek(w->file, (long) s, SEEK_CUR) < 0)
            return CA_ERROR_SYSTEM;
    }

    return CA_SUCCESS;

fail_io:

    if (feof(w->file))
        return CA_ERROR_CORRUPT;
    else if (ferror(w->file))
        return CA_ERROR_SYSTEM;

    ca_assert_not_reached();
}

int ca_wav_open(ca_wav **_w, FILE *f)  {
    uint32_t header[3], fmt_chunk[4];
    int ret;
    ca_wav *w;
    uint32_t file_size, fmt_size, data_size;

    ca_return_val_if_fail(_w, CA_ERROR_INVALID);
    ca_return_val_if_fail(f, CA_ERROR_INVALID);

    if (!(w = ca_new(ca_wav, 1)))
        return CA_ERROR_OOM;

    w->file = f;

    if (fread(header, sizeof(uint32_t), CA_ELEMENTSOF(header), f) != CA_ELEMENTSOF(header))
        goto fail_io;

    if (PA_UINT32_FROM_LE(header[0]) != 0x46464952U ||
        PA_UINT32_FROM_LE(header[2]) != 0x45564157U) {
        ret = CA_ERROR_CORRUPT;
        goto fail;
    }

    file_size = PA_UINT32_FROM_LE(header[1]);

    if (file_size <= 0 || file_size >= FILE_SIZE_MAX) {
        ret = CA_ERROR_TOOBIG;
        goto fail;
    }

    /* Skip to the fmt chunk */
    if ((ret = skip_to_chunk(w, 0x20746d66U, &fmt_size)) < 0)
        goto fail;

    if (fmt_size != 16) {
        ret = CA_ERROR_NOTSUPPORTED;
        goto fail;
    }

    if (fread(fmt_chunk, sizeof(uint32_t), CA_ELEMENTSOF(fmt_chunk), f) != CA_ELEMENTSOF(fmt_chunk))
        goto fail_io;

    if ((PA_UINT32_FROM_LE(fmt_chunk[0]) & 0xFFFF) != 1) {
        ret = CA_ERROR_NOTSUPPORTED;
        goto fail;
    }

    w->nchannels = PA_UINT32_FROM_LE(fmt_chunk[0]) >> 16;
    w->rate = PA_UINT32_FROM_LE(fmt_chunk[1]);
    w->depth = PA_UINT32_FROM_LE(fmt_chunk[3]) >> 16;

    if (w->nchannels <= 0 || w->rate <= 0) {
        ret = CA_ERROR_CORRUPT;
        goto fail;
    }

    if (w->depth != 16 && w->depth != 8) {
        ret = CA_ERROR_NOTSUPPORTED;
        goto fail;
    }

    /* Skip to the data chunk */
    if ((ret = skip_to_chunk(w, 0x61746164U, &data_size)) < 0)
        goto fail;
    w->data_size = (off_t) data_size;

    if ((w->data_size % (w->depth/8)) != 0) {
        ret = CA_ERROR_CORRUPT;
        goto fail;
    }

    *_w = w;

    return CA_SUCCESS;

fail_io:

    if (feof(f))
        ret = CA_ERROR_CORRUPT;
    else if (ferror(f))
        ret = CA_ERROR_SYSTEM;
    else
        ca_assert_not_reached();

fail:

    ca_free(w);

    return ret;
}

void ca_wav_close(ca_wav *w) {
    ca_assert(w);

    fclose(w->file);
    ca_free(w);
}

unsigned ca_wav_get_nchannels(ca_wav *w) {
    ca_assert(w);

    return w->nchannels;
}

unsigned ca_wav_get_rate(ca_wav *w) {
    ca_assert(w);

    return w->rate;
}

ca_sample_type_t ca_wav_get_sample_type(ca_wav *w) {
    ca_assert(w);

    return w->depth == 16 ?
#ifdef WORDS_BIGENDIAN
        CA_SAMPLE_S16RE
#else
        CA_SAMPLE_S16NE
#endif
        : CA_SAMPLE_U8;
}

int ca_wav_read_s16le(ca_wav *w, int16_t *d, size_t *n) {
    off_t remaining;

    ca_return_val_if_fail(w, CA_ERROR_INVALID);
    ca_return_val_if_fail(w->depth == 16, CA_ERROR_INVALID);
    ca_return_val_if_fail(d, CA_ERROR_INVALID);
    ca_return_val_if_fail(n, CA_ERROR_INVALID);
    ca_return_val_if_fail(*n > 0, CA_ERROR_INVALID);

    remaining = w->data_size / (off_t) sizeof(int16_t);

    if ((off_t) *n > remaining)
        *n = (size_t) remaining;

    if (*n > 0) {
        *n = fread(d, sizeof(int16_t), *n, w->file);

        if (*n <= 0 && ferror(w->file))
            return CA_ERROR_SYSTEM;

        ca_assert(w->data_size >= (off_t) *n * (off_t) sizeof(int16_t));
        w->data_size -= (off_t) *n * (off_t) sizeof(int16_t);
    }

    return CA_SUCCESS;
}

int ca_wav_read_u8(ca_wav *w, uint8_t *d, size_t *n) {
    off_t remaining;

    ca_return_val_if_fail(w, CA_ERROR_INVALID);
    ca_return_val_if_fail(w->depth == 8, CA_ERROR_INVALID);
    ca_return_val_if_fail(d, CA_ERROR_INVALID);
    ca_return_val_if_fail(n, CA_ERROR_INVALID);
    ca_return_val_if_fail(*n > 0, CA_ERROR_INVALID);

    remaining = w->data_size / (off_t) sizeof(uint8_t);

    if ((off_t) *n > remaining)
        *n = (size_t) remaining;

    if (*n > 0) {
        *n = fread(d, sizeof(uint8_t), *n, w->file);

        if (*n <= 0 && ferror(w->file))
            return CA_ERROR_SYSTEM;

        ca_assert(w->data_size >= (off_t) *n * (off_t) sizeof(uint8_t));
        w->data_size -= (off_t) *n * (off_t) sizeof(uint8_t);
    }

    return CA_SUCCESS;
}

off_t ca_wav_get_size(ca_wav *v) {
    ca_return_val_if_fail(v, (off_t) -1);

    return v->data_size;
}
