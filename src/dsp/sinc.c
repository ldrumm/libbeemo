#include <math.h>

#include "../definitions.h"
#include "sinc.h"

static const float TWO_PI = 2.0 * M_PI;
static const float FOUR_PI = 4.0 * M_PI;


void bmo_sinc_sb(float *sinc, uint32_t N, float fc)
{
    /*generate a sinc function -> adapted from BASIC on music.dsp mailing list*/
    const float M = N - 1;
    float n;
    // Generate sinc delayed by (N-1)/ 2
    for (uint32_t i = 0; i < N; i++) {
        if (i == M * 0.5)
            sinc[i] = 2.0 * fc;
        else {
            n = (float)i - M * 0.5;
            sinc[i] = sinf(2.0 * M_PI * fc * n) / (M_PI * n);
        }
    }
    return;
}

/* Below windowing functions from stephen W Smith DSP cookbook optimized by me*/
/*
EQUATION 16-1
The Hamming window. These
windows run from i = 0 to M,
for a total of M + 1 points.
w[i] = 0.54 - 0.46 cos (2PI*i / M )

EQUATION 16-2
The Blackman window.
w[i] = 0.42 - 0.5 cos (2PI*i / M ) + 0.08 cos (4PI*i / M )
*/
void bmo_hamming_window(float *buffer, uint32_t M)
{
    const float INV_M = 1.0 / M;

    for (uint32_t i = 0; i < M; i++)
        buffer[i] *= 0.54 - (0.46 * cosf(TWO_PI * i * INV_M));
    buffer[0] = 0.0;
    buffer[M - 1] = 0.0;
}


void bmo_blackman_window(float *buffer, uint32_t M)
{
    const float INV_M = 1.0 / M;

    for (uint32_t i = 0; i < M; i++)
        buffer[i] *= 0.42 - (0.5 * cosf(TWO_PI * i * INV_M)) +
                     (0.08 * cosf(FOUR_PI * i * INV_M));
}
