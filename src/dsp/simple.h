#ifndef DSP_BASICS_H
#define DSP_BASICS_H
#include <stdint.h>
#include <stdlib.h>
#define _bmo_clampf(in, max) (((in) > (max)) ? (max): ((in) <(-(max))) ?-(max): (in))
void bmo_mb_cpy(float ** dest, float ** src, uint32_t channels, size_t frames);
void bmo_mb_cpy_offset(float ** dest, float **src, size_t src_off, size_t dest_off, uint32_t channels, uint32_t frames);
void bmo_mix_sb(float * out, float * in_a, float * in_b, uint32_t samples);
void bmo_mix_mb(float ** out, float ** f1, float ** f2, uint32_t channels, uint32_t frames);
void bmo_sbcpy(float * in, float * out, uint32_t samples);
void _bmo_dither_sb(float * buffer, uint32_t resolution);
double bmo_osc_sine_mix_sb(float * buffer, float freq, double phase, float amplitude, uint32_t rate, uint32_t samples);
double bmo_osc_sq_mix_sb(float * buffer, float freq, double phase, float amplitude, uint32_t rate, uint32_t samples);
double bmo_osc_saw_sb(float * buffer, float freq, double phase, float amplitude, uint32_t rate, uint32_t samples);
void bmo_gain_sb(float *buffer, uint32_t samples, float gain_db);
void bmo_inv_sb(float * buffer, uint32_t samples);
void bmo_zero_sb(float * buffer, uint32_t samples);
void bmo_zero_sb2(float * buffer, uint32_t samples);
void bmo_zero_sb3(float * buffer, uint32_t samples);
void bmo_zero_mb(float ** buffer, uint32_t channels, uint32_t frames);
void bmo_zero_mb_off(float ** in, uint32_t channels, uint32_t frames, uint32_t offset);
void bmo_convolve_sb(float * out, float * in, float * impulse, uint32_t samples, uint32_t impulselen);
void bmo_lerp_sb(float * out, float * in, float ratio, size_t out_frames);
#endif
