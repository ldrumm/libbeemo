#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"
#include "definitions.h"
#include "dsp/simple.h"
#include "error.h"
#include "memory/map.h"
#include "multiplexers.h"


float **bmo_mb_new(uint32_t channels, uint32_t frames)
{
    uint32_t i;
    float **buffer = calloc(channels + 1, sizeof(float *));
    if (!buffer)
        return NULL;

    for (i = 0; i < channels; i++) {
        buffer[i] = calloc(frames, sizeof(float));
        if (!buffer[i])
            goto fail;
    }
    return buffer;

fail:
    while (i--)
        free(buffer[i]);
    free(buffer);
    return NULL;
}

void bmo_mb_free(float **buffer, uint32_t channels)
{
    for (uint32_t ch = 0; ch < channels; ch++) {
        free(buffer[ch]);
        buffer[ch] = NULL;
    }
    free(buffer);
}

static ssize_t seek_bo(BMO_buffer_obj_t *obj, ptrdiff_t offset, int whence)
{
    whence = whence ? whence : SEEK_SET;
    size_t frame;
    switch (whence) {
        case SEEK_SET: {
            assert(offset >= 0);
            frame = offset;
            break;
        }
        case SEEK_CUR: {
            assert(offset + obj->frame < obj->frames);
            frame = obj->frame + offset;
            break;
        }
        case SEEK_END: {
            assert(offset < 0);
            assert((-offset) <= (ptrdiff_t)obj->frames);
            frame = obj->frames + offset;
            break;
        }
        default: {
            bmo_err("invalid seek value for 'whence' parameter:%d", whence);
            assert(0);
            return -1;
        }
    }
    if (frame <= obj->frames) {
        obj->frame = frame;
        return 0;
    } else {
        obj->frame = obj->frames;
        return -1;
    }
}

static ssize_t
read_bo_mapped_ipcm(BMO_buffer_obj_t *obj, float **dest, uint32_t frames)
{
    assert(dest && obj && frames);
    uint32_t channels = obj->channels;
    size_t available = obj->frames - obj->frame;

    char z_idx = 0;
    if (available < frames) {
        // zero fill the end of the output buffer if the
        // input is not long enough;
        bmo_zero_mb_offset(dest, channels, frames, available);
        if (!obj->loop)
            z_idx = 1;
        else {
            bmo_conv_ibpcmtomb(
                dest,
                obj->userdata,
                channels,
                available,
                bmo_fmt_enc(obj->flags)
            );
            bmo_mb_cpy_offset(
                dest,
                dest,
                0,
                frames - available,
                channels,
                available
            );
            obj->frame = 0;
        }
        frames = available;
    }
    bmo_conv_ibpcmtomb(
        dest,
        obj->userdata + obj->frame * channels *
            bmo_fmt_stride(bmo_fmt_enc(obj->flags)),
        channels,
        frames,
        bmo_fmt_enc(obj->flags)
    );

    if (z_idx) {
        obj->frame = 0;
    } else {
        obj->frame += frames;
    }
    return 0;
}


static ssize_t
read_bo_mapped_if(BMO_buffer_obj_t *obj, float **dest, uint32_t frames)
{
    uint32_t channels = obj->channels;
    uint32_t encoding = bmo_fmt_enc(obj->flags);

    assert(dest && obj && frames);
    size_t available = obj->frames - obj->frame;
    char z_idx = 0;

    uint32_t stride = bmo_fmt_stride(encoding);
    if (available < frames) {
        // zero fill the end of the buffer if the source is not long enough.
        // and reset the source's index to zero, so that it loops.
        bmo_zero_mb_offset(dest, channels, frames - available, available);
        frames = available;
        if (obj->loop)
            z_idx = 1;
    }

    bmo_conv_ibftomb(
        dest,
        obj->userdata + obj->frame * stride * channels,
        channels,
        frames,
        encoding
    );
    obj->frame += frames;
    if (z_idx)
        obj->frame = 0;

    return 0;
}


static ssize_t read_bo_mb(BMO_buffer_obj_t *obj, float **dest, uint32_t frames)
{
    assert(dest && obj && frames);
    uint32_t channels = obj->channels;
    uint32_t available = obj->frames - obj->frame;
    char z_idx = 0;
    if (available < frames) {
        // zero the output when requested frames aren't available.
        bmo_zero_mb_offset(dest, channels, frames - available, available);
        frames = available;
        if (obj->loop)
            z_idx = 1;
    }
    bmo_mb_cpy_offset(
        dest,
        obj->userdata,
        obj->frame,
        0,
        channels,
        frames
    );
    obj->frame += frames;
    if (z_idx)
        obj->frame = 0;
    return 0;
}


BMO_buffer_obj_t *bmo_bo_new(
    uint32_t flags, uint32_t channels, size_t frames,
    uint32_t rate, size_t offset, size_t len, void *userdata)
{
    uint32_t encoding = bmo_fmt_enc(flags);
    flags &= (BMO_BUFFERED_DATA | BMO_MAPPED_FILE_DATA | BMO_EXTERNAL_DATA);

    assert(flags);

    BMO_buffer_obj_t *obj = malloc(sizeof(BMO_buffer_obj_t));
    if (!obj)
        return NULL;

    obj->flags = flags;
    obj->channels = channels;
    obj->frame = 0;
    obj->frames = frames;
    obj->rate = rate;
    obj->userdata = userdata;
    obj->buf_siz = len;
    obj->offset = offset;
    obj->is_alias = 0;
    obj->seek = NULL;
    obj->read = NULL;

    switch (flags) {
        case BMO_MAPPED_FILE_DATA: {
            // FIXME void * arithmetic
            obj->userdata = userdata + offset;
            // set the callbacks
            if (bmo_fmt_pcm(encoding))
                obj->read = read_bo_mapped_ipcm;
            else
                obj->read = read_bo_mapped_if;
            obj->seek = seek_bo;
            return obj;
            break; // never reached
        }
        case BMO_BUFFERED_DATA: {
            // alloc storage buffer, and copy data from file
            obj->userdata = bmo_mb_new(channels, frames);
            if (!obj->userdata) {
                free(obj);
                return NULL;
            }

            if (userdata) {
                // only copy data when there is a source, otherwise return
                // blank.
                if (bmo_fmt_pcm(encoding)) {
                    bmo_debug("demultiplexing PCM file and converting to float");
                    bmo_conv_ibpcmtomb(
                        obj->userdata,
                        (userdata + offset),
                        obj->channels,
                        obj->frames,
                        encoding
                    );
                } else {
                    bmo_debug("demultiplexing float file.");
                    bmo_conv_ibftomb(
                        obj->userdata,
                        userdata + offset,
                        obj->channels,
                        obj->frames,
                        encoding
                    );
                }
                bmo_unmap(userdata, len);
            }
            obj->userdata = NULL;
            obj->read = read_bo_mb;
            obj->seek = seek_bo;
            break;
        }
        case BMO_EXTERNAL_DATA: {
            obj->userdata = userdata;
            // hmmm...
            return obj;
        }
        default: assert("Unknown BMO_buffer_obj_t type" == NULL);
    } // switch(type)
    return obj;
}


BMO_buffer_obj_t *bmo_bo_alias(BMO_buffer_obj_t *obj)
{
    BMO_buffer_obj_t *cpy = malloc(sizeof(BMO_buffer_obj_t));
    if (!cpy)
        return NULL;
    memcpy(cpy, obj, sizeof(BMO_buffer_obj_t));

    cpy->is_alias = 1;
    return cpy;
}


BMO_buffer_obj_t *bmo_bo_cpy(BMO_buffer_obj_t *obj)
{
    BMO_buffer_obj_t *cpy = bmo_bo_alias(obj);
    if (!cpy)
        return NULL;

    cpy->frame = 0;
    cpy->userdata = NULL;

    switch (cpy->flags) {
        case BMO_MAPPED_FILE_DATA: {
            break;
        }
        case BMO_BUFFERED_DATA: {
            float **old = cpy->userdata;
            cpy->userdata = bmo_mb_new(cpy->channels, cpy->frames);
            bmo_mb_cpy(
                cpy->userdata,
                old,
                cpy->channels,
                cpy->frames
            );
            break;
        }
        case BMO_EXTERNAL_DATA: {
            bmo_err("can't duplicate buffer to external resource\n");
            bmo_bo_free(cpy);
            return NULL;
        }
        default: {
            assert(0);
            bmo_bo_free(cpy);
            return NULL;
        }
    }
    return cpy;
}


void bmo_bo_free(BMO_buffer_obj_t *obj)
{
    assert(obj);

    obj->frames = 0;
    obj->channels = 0;
    obj->rate = 0;
    obj->read = NULL;

    if (obj->is_alias) { // don't free resources of is_alias objects
        // FIXME refcount copies
        free(obj);
        return;
    }

    obj->read = NULL;
    /* free all audio data */
    if (obj->flags == BMO_MAPPED_FILE_DATA) {
        bmo_unmap(obj->userdata, obj->buf_siz);
        obj->userdata = NULL;
    } else if (obj->flags == BMO_BUFFERED_DATA) {
        if (obj->userdata) {
            bmo_mb_free(obj->userdata, obj->channels);
            obj->userdata = NULL;
        }
    } else {
        assert(0);
    }
    free(obj);

    return;
}
