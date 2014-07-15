#ifndef BMO_SUN_AU_H
#define BMO_SUN_AU_H
BMO_buffer_obj_t * bmo_fopen_sun(const char *path, uint32_t flags);
size_t bmo_fwrite_sun(BMO_buffer_obj_t * buffer, const char * path, uint32_t flags);

//BMO_AU_HEADER
typedef struct
{
	uint32_t au_magic_number;				/// ASCII '.snd' or 0x2e736e64
	uint32_t au_data_offset;				///	Audio data offset in bytes from the beginning of the file. This value is stored big endian. Most of the time this value = 24 bytes, but this is by no means guaranteed.
	uint32_t au_data_size;					///	The size in bytes of the data following the header. Usually filesize-24 stored big endian in the file.
	uint32_t au_data_encoding;				///	The encoding for the audio stream. Stored big-endian in the file. Common values are 3 - 16bit, linear PCM; 4 - 24bit, linear PCM; 6 - 32bit, IEEE floating point.
	
	uint32_t au_data_sample_rate;			///	sample rate. stored big endian in the file
	uint32_t au_data_channels;				///	number of interleaved channels in the audio stream. stored big endian in the file.
	uint32_t au_metadata;
}BMO_au_header_t;

/* these codes are straight off Wikipedia's page on sun AU */
/* only PCM and floating point are supported */

#define AU_FORMAT_G_711_ULAW		1 		/// 8-bit G.711 µ-law
#define AU_FORMAT_8_BIT_PCM 		2 		/// 8-bit linear PCM
#define AU_FORMAT_16_BIT_PCM		3 		/// 16-bit linear PCM
#define AU_FORMAT_24_BIT_PCM		4 		////24-bit linear PCM
#define AU_FORMAT_32_BIT_PCM		5 		/// 32-bit linear PCM
#define AU_FORMAT_32_BIT_FLOAT		6 		/// 32-bit IEEE floating point
#define AU_FORMAT_64_BIT_FLOAT		7 		/// 64-bit IEEE floating point
#define AU_FORMAT_SAMPLE_FRAG		8 		/// Fragmented sample data
#define AU_FORMAT_DSP_PROG			9 		/// DSP program
#define AU_FORMAT_8_BIT_FIXED		10 		/// 8-bit fixed point
#define AU_FORMAT_16_BIT_FIXED		11 		/// 16-bit fixed point
#define AU_FORMAT_24_BIT_FIXED		12 		/// 24-bit fixed point
#define AU_FORMAT_32_BIT_FIXED		13 		/// 32-bit fixed point
#define AU_FORMAT_16_BIT_EMPH		18 		/// 16-bit linear with emphasis
#define AU_FORMAT_16_BIT_COMP		19 		/// 16-bit linear compressed
#define AU_FORMAT_16_BIT_EMPH_COMP	20 		/// 16-bit linear with emphasis and compression
#define AU_FORMAT_MUSIC_KIT_DSP		21 		/// Music kit DSP commands
#define AU_FORMAT_G_721				23 		/// 4-bit ISDN u-law compressed using the ITU-T G.721 ADPCM voice data encoding scheme
#define AU_FORMAT_G_722				24 		/// ITU-T G.722 ADPCM
#define AU_FORMAT_G_723				25 		/// ITU-T G.723 3-bit ADPCM
#define AU_FORMAT_G_724				26 		/// ITU-T G.723 5-bit ADPCM
#define AU_FORMAT_G_711_ALAW		27 		/// 8-bit G.711 A-law
#endif
