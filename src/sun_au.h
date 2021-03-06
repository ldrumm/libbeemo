#ifndef BMO_SUN_AU_H
#define BMO_SUN_AU_H
#include <stdint.h>
#include <stdio.h>

#include "definitions.h"

BMO_buffer_obj_t *bmo_fopen_sun(const char *path, uint32_t flags);
size_t bmo_fwrite_sun(BMO_buffer_obj_t *buffer, FILE *file, uint32_t flags);
size_t
bmo_buf_save_sun(BMO_buffer_obj_t *buffer, const char *path, uint32_t flags);
int bmo_fwrite_header_sun(
    FILE *file,
    uint32_t flags,
    uint32_t channels,
    uint32_t frames,
    uint32_t rate
);

typedef struct {
    /// ASCII '.snd' or 0x2e736e64
    unsigned char au_magic_number[4];
    /// Audio data offset in bytes from the beginning of the file. This value
    /// is stored big endian. Most of the time this value = 24 bytes, but this
    /// is by no means guaranteed.
    uint32_t au_data_offset;
    /// The size in bytes of the data following the header. Usually filesize-24
    /// stored big endian in the file.
    uint32_t au_data_size;
    /// The encoding for the audio stream. Stored big-endian in the file.
    /// Common values are 3 - 16bit, linear PCM; 4 - 24bit, linear PCM; 6 -
    /// 32bit, IEEE floating point.
    uint32_t au_data_encoding;
    /// sample rate. stored big endian in the file
    uint32_t au_data_sample_rate;
    /// number of interleaved channels in the audio stream. stored big endian
    /// in the file.
    uint32_t au_data_channels;
    /// number of bytes for metadata storage
    uint32_t metadata_len;
    char *metadata;
} BMO_au_header_t;

/* these codes are straight off Wikipedia's page on sun AU */
/* only PCM and floating point are supported */
/// 8-bit G.711 µ-law
#define AU_FORMAT_G_711_ULAW (1)
/// 8-bit linear PCM
#define AU_FORMAT_8_BIT_PCM (2)
/// 16-bit linear PCM
#define AU_FORMAT_16_BIT_PCM (3)
////24-bit linear PCM
#define AU_FORMAT_24_BIT_PCM (4)
/// 32-bit linear PCM
#define AU_FORMAT_32_BIT_PCM (5)
/// 32-bit IEEE floating point
#define AU_FORMAT_32_BIT_FLOAT (6)
/// 64-bit IEEE floating point
#define AU_FORMAT_64_BIT_FLOAT (7)
/// Fragmented sample data
#define AU_FORMAT_SAMPLE_FRAG (8)
/// DSP program
#define AU_FORMAT_DSP_PROG (9)
/// 8-bit fixed point
#define AU_FORMAT_8_BIT_FIXED (10)
/// 16-bit fixed point
#define AU_FORMAT_16_BIT_FIXED (11)
/// 24-bit fixed point
#define AU_FORMAT_24_BIT_FIXED (12)
/// 32-bit fixed point
#define AU_FORMAT_32_BIT_FIXED (13)
/// 16-bit linear with emphasis
#define AU_FORMAT_16_BIT_EMPH (18)
/// 16-bit linear compressed
#define AU_FORMAT_16_BIT_COMP (19)
/// 16-bit linear with emphasis and compression
#define AU_FORMAT_16_BIT_EMPH_COMP (20)
/// Music kit DSP commands
#define AU_FORMAT_MUSIC_KIT_DSP (21)
/// 4-bit ISDN u-law compressed using the ITU-T G.721 ADPCM voice data encoding
/// scheme
#define AU_FORMAT_G_721 (23)
/// ITU-T G.722 ADPCM
#define AU_FORMAT_G_722 (24)
/// ITU-T G.723 3-bit ADPCM
#define AU_FORMAT_G_723 (25)
/// ITU-T G.723 5-bit ADPCM
#define AU_FORMAT_G_724 (26)
/// 8-bit G.711 A-law
#define AU_FORMAT_G_711_ALAW (27)
#endif
