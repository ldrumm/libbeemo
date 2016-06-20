#ifdef BMO_HAVE_SNDFILE
#include <assert.h>
#include <errno.h>
#include <sndfile.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multiplexers.h"
#include "buffer.h"
#include "error.h"
#include "dsp/simple.h"
#include "memory/map.h"


/*
typedef struct
{
   sf_vio_get_filelen  get_filelen ;
   sf_vio_seek         seek ;
   sf_vio_read         read ;
   sf_vio_write        write ;
   sf_vio_tell         tell ;
} SF_VIRTUAL_IO ;
      typedef sf_count_t  (*sf_vio_get_filelen) (void *user_data) ;
      typedef sf_count_t  (*sf_vio_seek)        (sf_count_t offset, int whence,
void *user_data) ;
      typedef sf_count_t  (*sf_vio_read)        (void *ptr, sf_count_t count,
void *user_data) ;
      typedef sf_count_t  (*sf_vio_write)       (const void *ptr, sf_count_t
count, void *user_data) ;
      typedef sf_count_t  (*sf_vio_tell)        (void *user_data) ;
*/

struct bmo_sf_state {
    SNDFILE *sf;
    void *bo;
};

static sf_count_t filelen_sf_vio(void *userdata)
{
    BMO_buffer_obj_t *bo = userdata;
    if (bo->buf_siz > SF_COUNT_MAX) {
        bmo_err("overflow in sndfile");
        return -1;
    }
    bmo_debug("file size is %zu\n", bo->buf_siz);
    return (sf_count_t)bo->buf_siz;
}

static sf_count_t tell_sf_vio(void *userdata)
{
    BMO_buffer_obj_t *bo = userdata;
    if (bo->offset > SF_COUNT_MAX) {
        bmo_err("overflow in SF file\n");
        return -1;
    }
    bmo_debug("%offset: zu\n", bo->offset);
    return bo->offset;
}

static sf_count_t seek_sf_vio(sf_count_t offset, int whence, void *userdata)
{
    /* From the libsnfile docs http://www.mega-nerd.com/libsndfile/api.html :
        The virtual file context must seek to offset using the seek mode
        provided by whence which is one of
              SEEK_CUR
              SEEK_SET
              SEEK_END
        The return value must contain the new offset in the file.
    */
    BMO_buffer_obj_t *bo = userdata;
    switch (whence) {
        case SEEK_END: {
            bmo_debug("offset=%zi, whence=SEEK_END\n", (long)offset);
            bo->offset = bo->buf_siz - offset;
            if (bo->offset + offset >= bo->buf_siz) {
                bo->offset = bo->buf_siz - 1;
                break;
            }
            bo->offset += offset;
            break;
        }
        case SEEK_SET: {
            bmo_debug("offset=%zi, whence=SEEK_SET\n", (long)offset);
            assert(offset >= 0);
            if (offset < 0)
                return -1;
            assert((size_t)offset < bo->buf_siz);
            if ((size_t)offset >= bo->buf_siz)
                bo->offset = bo->buf_siz;
            else if (offset < 0)
                bo->offset = 0;
            else
                bo->offset = offset;
            break;
        }
        default: return -1;
    }

    return bo->offset;
}


static sf_count_t read_sf_vio(void *dest, sf_count_t count, void *userdata)
{
    bmo_debug("count = %zu\n", (size_t)count);
    BMO_buffer_obj_t *bo = userdata;
    assert(count >= 0);
    if (count < 0)
        return -1;

    struct bmo_sf_state *state = bo->userdata;
    switch (bo->flags & BMO_EXTERNAL_DATA) {
        case BMO_EXTERNAL_DATA: {
            memcpy(dest, state->bo + bo->offset, (size_t)count);
            bo->offset += count;
            break;
        }
        default: {
            bmo_err("unknown source for sndfile virtio: %" PRIu32, bo->flags);
            return -1;
        }
    }
    return count;
}


static sf_count_t write_sf_vio(const void *src, sf_count_t count, void *userdata)
{
    bmo_debug("count=%zi", count);
    if (count < 0)
        return -1;

    BMO_buffer_obj_t *bo = userdata;
    struct bmo_sf_state *state = bo->userdata;
    switch (bo->flags) {
        case BMO_EXTERNAL_DATA:
        case BMO_MAPPED_FILE_DATA: {
            memcpy(state->bo, src, (size_t)count);
            return count;
        }
        default: {
            bmo_err("unknown datasource for sndfile virtio: %" PRIu32,
                    bo->flags);
            BMO_NOT_IMPLEMENTED;
            return -1;
        }
    }
    return -1;
}


static ssize_t
read_bo_sf(BMO_buffer_obj_t *bo, float **dest, uint32_t frames)
{
    assert(bo && dest);
    // TODO The C99 VLA stuff needs to be worked out because stack
    // overflow is too easy.
    float buffer[(sizeof(float) * frames * bo->channels)];

    struct bmo_sf_state *state = bo->userdata;
    sf_count_t read = sf_readf_float(state->sf, buffer, (sf_count_t)frames);
    if (read < frames) {
        bmo_zero_mb_offset(dest, bo->channels, frames - read, read);
    }
    bmo_conv_ibftomb(dest, (char *)buffer, bo->channels, (uint32_t)read,
                     BMO_FMT_NATIVE_FLOAT);
    return read;
}


static ssize_t
seek_bo_sf(BMO_buffer_obj_t *bo, ssize_t index, int whence)
{
    whence = whence ? whence : SEEK_SET;
    assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);
    assert(bo);
    struct bmo_sf_state *state = bo->userdata;
    return sf_seek(state->sf, (sf_count_t)index, whence);
}

static void close_bo_sf(BMO_buffer_obj_t *bo)
{
    bmo_debug("closing sf object");
    struct bmo_sf_state *state = bo->userdata;
    sf_close(state->sf);
}

// _bmo_fopen_sndfile is considered a private function but
// requires external linkage because it is used in other parts of
// the library for reading audio files - hence no `static` qualifier.
BMO_buffer_obj_t *_bmo_fopen_sndfile(const char *path, uint32_t flags)
{
    bmo_debug("opening '%s' with libsndfile", path);

    SF_INFO info = {0};
    SNDFILE *sf = sf_open(path, SFM_READ, &info);
    if (!sf) {
        bmo_err("couldn't open file\n");
        return NULL;
    }
    BMO_buffer_obj_t *bo = bmo_bo_new(
        flags | BMO_EXTERNAL_DATA,
        (uint32_t)info.channels,
        (size_t)info.frames,
        (uint32_t)info.samplerate,
        0, 0, sf
    );
    bo->seek = seek_bo_sf;
    bo->read = read_bo_sf;
    bo->close = close_bo_sf;

    return bo;
}

BMO_buffer_obj_t *_bmo_fopen_sndfile_vio(const char *path, uint32_t flags)
{
    int err = 0;
    const size_t size = bmo_fsize(path, &err);
    if (err)
        return NULL;

    struct bmo_sf_state *state = malloc(sizeof *state);
    assert(state);
    state->bo = bmo_map(path, BMO_MAP_READONLY, 0);
    SF_INFO info = {0};
    SF_VIRTUAL_IO vio = {
        .get_filelen = filelen_sf_vio,
        .seek = seek_sf_vio,
        .read = read_sf_vio,
        .write = write_sf_vio,
        .tell = tell_sf_vio
    };
    flags |= BMO_EXTERNAL_DATA;
    BMO_buffer_obj_t *bo = bmo_bo_new(flags, 0, 0, 0, 0, size, state);
    state->sf = sf_open_virtual(&vio, SFM_READ, &info, bo);

    if (!state->sf) {
        bmo_err("couldn't open file\n");
        return NULL;
    }
    if (info.frames <= 0) {
        sf_close(state->sf);
        bmo_bo_free(bo);
        return NULL;
    }

    bo->channels = (uint32_t)info.channels;
    bo->frames = (uint32_t)info.frames;
    bo->rate = (uint32_t)info.samplerate;

    bo->seek = seek_bo_sf;
    bo->read = read_bo_sf;
    bo->close = close_bo_sf;

    return bo;
}
#endif
