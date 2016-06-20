#ifndef BMO_RIFF_WAV_H
#define BMO_RIFF_WAV_H
BMO_buffer_obj_t *bmo_fopen_wav(const char *path, uint32_t flags);

typedef struct {
    uint32_t
        chunk_id; /// Contains the letters "RIFF" in ASCII form (0x52494646).
    uint32_t chunk_size; ///	36 + SubChunk2Size, or more precisely: 4 + (8 +
                         ///SubChunk1Size) + (8 + SubChunk2Size).size of the
                         ///rest of the chunk following this number;the entire
                         ///file in bytes minus 8 bytes for: ChunkID and
                         ///ChunkSize.
    uint32_t format;     ///	Contains the letters "WAVE" (0x57415645).
    uint32_t subchunk_1_id;   ///	Contains the letters "fmt " (0x666d7420).
    uint32_t subchunk_1_size; ///	16 for PCM.  This is the size of the rest of
                              ///the Subchunk which follows this number.
    uint16_t format_no; ///	PCM = 1 (i.e. Linear quantization)  32 bit float = 3
                        ///we'll ignore other compression types
    uint16_t num_channels;     ///	max 65535 channels in a wav file
    uint32_t sample_rate;      ///	max 4GHz sample rate
    uint32_t byte_rate;        ///	SampleRate * NumChannels * BitsPerSample/8
    uint16_t block_align;      ///	NumChannels * BitsPerSample/8: The number of
                               ///bytes for one sample including all channels.
    uint16_t bits_per_sample;  ///	bit depth - should be 32 or 64 bits for IEEE
                               ///float
    uint32_t extra_param_size; ///	if PCM, then doesn't exist, otherwise
                               ///determines the size of the extra parameter
                               ///chunk
    char *extra_params;        ///	space for extra parameters
    uint32_t subchunk_2_id;    ///	Contains the letters "data"(0x64617461
                               ///big-endian form).
    uint32_t subchunk_2_size;  ///	NumSamples * NumChannels * BitsPerSample/8:
                               ///This is the number of bytes in the data. You
                               ///can also think of this as the size of the read
                               ///of the subchunk following this number.
} BMO_wav_header;


#define WAV_FORMAT_8_BIT_PCM 2        /// 8-bit linear PCM unsigned
#define WAV_FORMAT_16_BIT_PCM 3       /// 16-bit linear PCM signed
#define WAV_FORMAT_24_BIT_PCM 4       /// 24-bit linear PCM
#define WAV_FORMAT_32_BIT_PCM 5       /// 32-bit linear PCM
#define WAV_FORMAT_32_BIT_FLOAT 6     /// 32-bit IEEE floating point
#define WAV_FORMAT_64_BIT_FLOAT 7     /// 64-bit IEEE floating point
#define WAVE_FORMAT_PCM 0x0001        /// uncompressed PCM
#define WAVE_FORMAT_IEEE_FLOAT 0x0003 /// 32 or 64 bit floating point format
#define WAVE_FORMAT_ALAW 0x0006       /// UNSUPPORTED
#define WAVE_FORMAT_MULAW 0x0007      /// UNSUPPORTED
#define WAVE_FORMAT_EXTENSIBLE                                                 \
    0xfffe /// when the wav header contains extra chunks and wide datatypes

#endif
