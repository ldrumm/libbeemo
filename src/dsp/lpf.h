#ifndef BMO_LPF_H
#define BMO_LPF_H
void bmo_lpf_peephole_s(float * out, float * in, size_t samples);
void bmo_lpf_sinc_s(float * out, float * in, size_t samples, uint32_t sinc_len, uint32_t rate_in, uint32_t rate_out);
#define BMO_SINC_HIGH 10001
#define BMO_SINC_MED 101
#define BMO_SINC_LOW 65
#endif

