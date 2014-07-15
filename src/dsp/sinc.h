#ifndef BMO_SINC_H
#define BMO_SINC_H
#include <stdint.h>
void bmo_sinc_sb(float * sinc, uint32_t N, float fc);
void bmo_hamming_window(float * buffer, uint32_t M);
void bmo_blackman_window(float * buffer, uint32_t M);
#endif
