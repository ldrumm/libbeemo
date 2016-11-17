/** @file Functions for dealing with NeXT/Sun .au files */

#include <stdint.h>
#include <assert.h>

#include "definitions.h"
#include "sun_au.h"
#include "multiplexers.h"
#include "buffer.h"
#include "memory/map.h"
#include "import_export.h"
#include "error.h"

#define AU_HEADER_SIZE (28u)
#define AU_MAGIC_BE ".snd"
#define AU_MAGIC_LE "dns."

#define AU_UNKOWN_LEN (0xffffffffu)
#define AU_BMO_SUPPORTED_FORMATS (\
     BMO_FMT_PCM_8 | \
     BMO_FMT_PCM_16_BE | \
     BMO_FMT_PCM_24_BE | \
     BMO_FMT_PCM_32_BE | \
     BMO_FMT_FLOAT_32_BE | \
     BMO_FMT_FLOAT_64_BE \
)

#define _swap32(i)\
(((0xff & (i)) << 24) \
        | ((0xff00 & (i)) << 8) \
        | ((0xff0000 & (i)) >> 8) \
        | ((0xff000000 &(i))>> 24))

#define native_32(i, host_be, file_le) \
    ((((host_be) && (file_le))||((!host_be) && (!file_le))) ? _swap32((i)): (i))

#define be32(i, host_be) \
    ((host_be) ? (i): _swap32((i)))


/**
Opens a sun .au file as a buffer object.
Standard sun .au files and little endian files with or without metadata are
opened.
Only PCM and floating point audio is supported.
Metadata is ignored.

@returns buffer of audio data from the file.
@param [in] path The filesystem path to the audio file
@param [in] flags Specify open behaviour. Significant flags are:
    * BMO_BUFFERED_DATA/BMO_MAPPED_FILE_DATA - whether to read the entire file
    immediately or let the OS page as the file is accessed.
    * BMO_MAP_READWRITE - Whether to open the file as writable.
    (valid only with BMO_MAPPED_FILE_DATA)
    * Other flags are ignored and passed on to the underlying buffer handler.
*/

BMO_buffer_obj_t *bmo_fopen_sun(const char *path, uint32_t flags)
{
    BMO_au_header_t header = {0};
    void *data;
    uint32_t *hp;
    int err = 0;
    int host_be = bmo_host_be();
    int file_le = 0;

    size_t size = bmo_fsize(path, &err);
    if (!size || err) {
        bmo_err("%s does not exist or is empty\n", path);
        return NULL;
    }
    if (size < AU_HEADER_SIZE - 4) {
        bmo_err("`%s`'s header is too short\n", path);
        return NULL;
    }
    // If we want to write to the file, we need to assure that we've mapped
    // readwrite
    assert(((flags & BMO_MAP_READWRITE) == 0) ||
           (flags & BMO_MAPPED_FILE_DATA));

    hp = data = bmo_map(path, 0, 0);
    if (!data) {
        bmo_err("couldn't map file:%s\n", path);
        goto fail;
    }

    {
        const size_t msize = sizeof header.au_magic_number;
        memcpy(header.au_magic_number, data, msize);
        // Check magic number for ".snd"
        if (memcmp(header.au_magic_number, AU_MAGIC_BE, msize)  == 0) {
            file_le = 0;
        } else if (memcmp(header.au_magic_number, AU_MAGIC_LE, msize) == 0) {
            file_le = 1;
        } else {
            bmo_err(
                "%s is not a valid AU file; magic number is 0x%8x\n",
                path,
                header.au_magic_number
            );
            goto fail;
        }
    }

    header.au_data_offset = native_32(hp[1], host_be, file_le);
    header.au_data_size = native_32(hp[2], host_be, file_le);
    header.au_data_encoding = native_32(hp[3], host_be, file_le);
    header.au_data_sample_rate = native_32(hp[4], host_be, file_le);
    header.au_data_channels = native_32(hp[5], host_be, file_le);

    if (header.au_data_offset >= AU_HEADER_SIZE) {
        // metadata is ignored. If you want it, make the 'if 0' true somehow
#if 0
        header.metadata_len = (size_t)header.au_data_offset - 24;
        header.metadata = malloc(header.metadata_len);
        strncpy(header.metadata, data + 24, header.metadata_len);
        // the spec requires the metadata to end with an ASCII NUL, but who
        // knows...
        header.metadata[header.metadata_len - 1] = '\0';
        bmo_debug("'%s metadata:\n%s\n", path, header.metadata);
        if (!header.metadata) {
            // this is non-fatal because we don't care about metadata, right?
            bmo_err("couldn't allocate memory for file metatdata");
            header.metadata_len = 0;
        }
#endif
        bmo_debug(
            "AU file has metadata of %zu bytes\n",
            (size_t)header.au_data_offset - (AU_HEADER_SIZE - 4)
        );
    }

    bmo_debug(
        "'%s':\n"
        "magic number:%x\t\n"
        "data offset:%x,\n\t"
        "data size:%x\n\t"
        "encoding:%x,\n\t"
        "sample rate:%x,\n\t"
        "channels:%x\n\t",
        path, header.au_magic_number, header.au_data_offset,
        header.au_data_size, header.au_data_encoding,
        header.au_data_sample_rate, header.au_data_channels
    );

    if (header.au_data_offset < AU_HEADER_SIZE) {
        bmo_info(
            "'%s' has non-standard data offset of %u\n",
            path,
            header.au_data_offset
        );
    }
    if (!header.au_data_size) {
        // It is valid to have a zero-length audio stream, but is there any
        // point?
        bmo_err("%s has no audio data section\n", path);
        goto fail;
    }
    if (header.au_data_offset >= size - 1 ||
        header.au_data_offset < AU_HEADER_SIZE - 4) {
        // possible overflow attempt or just corrupt.
        bmo_err("Badly crafted header\n");
        goto fail;
    }

    size_t data_size = header.au_data_size;
    if (header.au_data_size == AU_UNKOWN_LEN) {
        /*
        http://pubs.opengroup.org/external/auformat.html
        If the datasize is unknown at the time of writing the header it is valid
        to set the size to (uint32_t)(~0), in which case, the application is
        responsible for detecting the real number of samples.
        This also means files > 4GiB are valid without change to the spec.
        though in that case the number of bytes can't be saved in the header.
        */
        data_size = size - header.au_data_offset;
        if (data_size < UINT32_MAX)
            header.au_data_size = size - header.au_data_offset;
    }

    if (!header.au_data_channels) {
        bmo_err("%s has no audio channels\n", path);
        goto fail;
    }

    if (!header.au_data_sample_rate) {
        bmo_err("sample rate of '%s' unkown.\n");
        goto fail;
    }

    /*test the data encoding of the file, and configure the conversion */
    #define PCM_FMT(BITS) \
        ((file_le == 0 ? BMO_FMT_PCM_##BITS##_BE : BMO_FMT_PCM_##BITS##_LE))
    #define FLOAT_FMT(BITS) \
        ((file_le == 0 ? BMO_FMT_FLOAT_##BITS##_BE : BMO_FMT_FLOAT_##BITS##_LE))
    switch (header.au_data_encoding) {
        case AU_FORMAT_8_BIT_PCM: flags |= BMO_FMT_PCM_8; break;
        case AU_FORMAT_16_BIT_PCM: flags |= PCM_FMT(16); break;
        case AU_FORMAT_24_BIT_PCM: flags |= PCM_FMT(24); break;
        case AU_FORMAT_32_BIT_PCM: flags |= PCM_FMT(32); break;
        case AU_FORMAT_32_BIT_FLOAT: flags |= FLOAT_FMT(32); break;
        case AU_FORMAT_64_BIT_FLOAT: flags |= FLOAT_FMT(64); break;
        default: {
            bmo_err("au encoding of type %d not supported\n",
                    header.au_data_encoding);
            goto fail;
        }
    }
#undef PCM_FMT
#undef FLOAT_FMT

    return bmo_bo_new(
        flags,
        header.au_data_channels,
        data_size / bmo_fmt_stride(flags) / header.au_data_channels,
        header.au_data_sample_rate,
        header.au_data_offset,
        size,
        data
    );

fail:
    bmo_unmap(data, size);
    return NULL;
} //bmo_fopen_sun


/**Writes a sun/NeXT header to be later populated with audio.

@returns 0 on success, -1 on error.
@param [in] file        Pointer to the file to write.
@param [in] flags       Desired encoding.
@param [in] channels    Number of channels.
@param [in] frames      Total samples per channel.
@param [in] rate        Sample rate.
*/

int bmo_fwrite_header_sun(
    FILE *file, uint32_t flags, uint32_t channels,
    uint32_t frames, uint32_t rate)
{
    int host_be = bmo_host_be();
    uint32_t encoding = bmo_fmt_enc(flags);
    /*
    Sun's specification states that metadata must be a non-zero multiple of 8
    bytes
    and it must be terminated with at least one null (zero) byte.
    However, in the same spec, they give it a default length of 4 bytes.
         * SoX uses 4 bytes.
         * libsndfile ignores it on read and write.
         * libaudiofile ignores it completely on read and write.
    http://pubs.opengroup.org/external/auformat.html
   */
    char padding[4] = {'\0'};
    BMO_au_header_t header = {{".snd"}, 0, 0, 0, 0, 0, 0, NULL};

    header.au_data_offset = be32(AU_HEADER_SIZE, host_be);
    header.au_data_size =
        be32((frames
                  ? (UINT32_MAX / (bmo_fmt_stride(flags) * channels)) > frames
                        ? bmo_fmt_stride(flags) * frames * channels
                        : AU_UNKOWN_LEN
                  : AU_UNKOWN_LEN),
             host_be);
    header.au_data_sample_rate = be32(rate, host_be);
    header.au_data_channels = be32(channels, host_be);

    uint32_t enc;
    switch (encoding) {
        case BMO_FMT_PCM_8: enc = AU_FORMAT_8_BIT_PCM; break;
        case BMO_FMT_PCM_16_LE: enc = AU_FORMAT_16_BIT_PCM; break;
        case BMO_FMT_PCM_24_LE: enc = AU_FORMAT_24_BIT_PCM; break;
        case BMO_FMT_PCM_32_LE: enc = AU_FORMAT_32_BIT_PCM; break;
        case BMO_FMT_FLOAT_32_LE: enc = AU_FORMAT_32_BIT_FLOAT; break;
        case BMO_FMT_FLOAT_64_LE: enc = AU_FORMAT_64_BIT_FLOAT; break;
        case BMO_FMT_PCM_16_BE: enc = AU_FORMAT_16_BIT_PCM; break;
        case BMO_FMT_PCM_24_BE: enc = AU_FORMAT_24_BIT_PCM; break;
        case BMO_FMT_PCM_32_BE: enc = AU_FORMAT_32_BIT_PCM; break;
        case BMO_FMT_FLOAT_32_BE: enc = AU_FORMAT_32_BIT_FLOAT; break;
        case BMO_FMT_FLOAT_64_BE: enc = AU_FORMAT_64_BIT_FLOAT; break;
        default: {
            bmo_err("Unsupported Sun/AU format type:%x\n", encoding);
            assert(0);
            return -1;
        }
    }
    header.au_data_encoding = be32(enc, host_be);

    size_t written = 0;
    written += fwrite(&header.au_magic_number, 4, 1, file);
    written += fwrite(&header.au_data_offset, 4, 1, file);
    written += fwrite(&header.au_data_size, 4, 1, file);
    written += fwrite(&header.au_data_encoding, 4, 1, file);
    written += fwrite(&header.au_data_sample_rate, 4, 1, file);
    written += fwrite(&header.au_data_channels, 4, 1, file);
    written += fwrite(&padding, 1, 4, file);
    if (written != AU_HEADER_SIZE) {
        return -1;
    }
    return 0;

}	//bmo_fwrite_header_sun

/** Dumps an audio buffer as a sun/NeXT audio file

@returns size of the resultant file 0 on failure.
If the write fails, the state of @file are unspecified.
@param [in]     buffer  Buffer of audio to save.
@param [out]    file    Open target stream.
@param [in]     flags   Preferred encoding/dither information.
*/
size_t bmo_fwrite_sun(BMO_buffer_obj_t *buffer, FILE *file, uint32_t flags)
{
    uint32_t encoding = bmo_fmt_enc(flags);
    size_t written = 0;

    if (!(encoding & AU_BMO_SUPPORTED_FORMATS)) {
        bmo_err("Unsupported Sun/AU format type:%x\n", encoding);
        return 0;
    }

    if (bmo_fwrite_header_sun(
          file,
          flags,
          buffer->channels,
          buffer->frames,
          buffer->rate
    ) != 0) {
        bmo_err("couldn't write AU file header\n");
        return 0;
    };
    if (buffer->flags == BMO_MAPPED_FILE_DATA) {
        written = bmo_fwrite_ib(
            file,
            buffer->userdata,
            buffer->channels,
            encoding,
            bmo_fmt_enc(buffer->flags),
            buffer->frames,
            bmo_fmt_dither(flags)
        );
    } else if (buffer->flags == BMO_BUFFERED_DATA) {
        written = bmo_fwrite_mb(
            file,
            buffer->userdata,
            buffer->channels,
            encoding,
            buffer->frames,
            bmo_fmt_dither(flags)
        );
    } else {
        assert("bmo_fwrite_sun() called with unknown flag" == NULL);
        fflush(file);
        return 0;
    }

    return written + AU_HEADER_SIZE;
} //bmo_fwrite_sun


/**Dumps a buffer as a sun/NeXT .au file into a file on the filesystem
@returns size in bytes of the saved file if successful or 0 on failure.
@param [in] buffer audio data to save.
@param [in] path filesystem path for resultant file.
@param [in] flags encoding and dither parameters.
*/
size_t bmo_buf_save_sun(BMO_buffer_obj_t *buffer, const char *path, uint32_t flags)
{
    struct stat statbuf;
    FILE *file = NULL;
    if (!buffer) {
        bmo_err("passed NULL pointer - no AU file written\n");
        return 0;
    }

    /* check and open file for writing */
    if ((stat(path, &statbuf)) != -1) {
        bmo_err("%s exists, data not written\n", path);
        return 0;
    }

    file = fopen(path, "wb");
    if (!file) {
        bmo_err("file open failure\n");
        return 0;
    }
    size_t ret = bmo_fwrite_sun(buffer, file, flags);
    fclose(file);

    return ret;
}
