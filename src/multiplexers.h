#ifndef BMO_MULTIPLEXERS_H
#define BMO_MULTIPLEXERS_H
uint32_t bmo_host_endianness(void);
int bmo_host_be(void);
int bmo_host_le(void);
void _bmo_swap_16(void * datum);
void _bmo_swap_24(void * datum);
void _bmo_swap_32(void * datum);
void _bmo_swap_64(void * datum);
void _bmo_swap_ib(void * data, uint32_t stride, size_t len);
uint32_t bmo_fmt_enc(uint32_t flags);
uint32_t bmo_fmt_stride(uint32_t flags);
int bmo_fmt_pcm(uint32_t flags);
uint32_t bmo_fmt_end(uint32_t flags);
uint32_t bmo_fmt_dither(uint32_t flags);
uint32_t _bmo_fmt_conv_type(uint32_t fmt_out, uint32_t fmt_in);
long double _bmo_fmt_pcm_range(uint32_t fmt);
void bmo_conv_iftoif(void * out, void * in, uint32_t fmt_out, uint32_t fmt_in, uint32_t samples);
void bmo_conv_iftoipcm(void * out_stream, void * in_stream, uint32_t fmt_out,  uint32_t fmt_in, uint32_t count);
void bmo_conv_ipcmtoipcm(void * out_stream, void * in_stream, uint32_t fmt_out, uint32_t fmt_in, uint32_t count);
void bmo_conv_ipcmtoif(float * out, void * in_stream, uint32_t fmt_out, uint32_t fmt_in, uint32_t count);
void bmo_conv_ibpcmtomb(float ** output_data, void * interleaved_data, uint32_t channels, uint32_t frames, uint32_t fmt_in);
int bmo_conv_ibftomb(float ** output_data, char * input_data, uint32_t channels, uint32_t frames, uint32_t fmt_in);
void bmo_conv_mftoix(void * out, float ** in, uint32_t channels, uint32_t fmt_out, uint32_t frames);
void bmo_conv_ibtoib(void * out, void * in, uint32_t fmt_out, uint32_t fmt_in, uint32_t count);
#endif
