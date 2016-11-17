#include "simple.h"
#include "sinc.h"
#include "../definitions.h"

void bmo_lpf_peephole_s(float *out, float *in, size_t samples)
{
    /**peephole LPF - attempt 1
    a sliding window averaging filter based on a multiple pass variable window
    length. Very poor cutoff curve at about 3dB/8ve, though the duff's device
    implementation makes it fast as it produces nicely vectorized code*/

    int s = 8;
    for (size_t i = 0; i < samples - 8; i++) {
        switch (s) {
            case 8: {
                out[i] = (in[0] + in[1] + in[2] + in[3] + in[4] + in[5] +
                          in[6] + in[7]) *
                         0.125;
                s--;
            }
            case 7: {
                out[i] = (in[0] + in[1] + in[2] + in[3] + in[4] + in[5] + in[6]) *
                       0.142857142857;
                s--;
            }
            case 6: {
                out[i] = (in[0] + in[1] + in[2] + in[3] + in[4] + in[5]) *
                       0.166666666667;
                s--;
            }
            case 5: {
                out[i] = (in[0] + in[1] + in[2] + in[3] + in[4]) * 0.2;
                s--;
            }
            case 4: {
                out[i] = (in[0] + in[1] + in[2] + in[3]) * 0.25;
                s--;
            }
            case 3: {
                out[i] = (in[0] + in[1] + in[2]) * 0.333333333333;
                s--;
            }
            case 2: {
                out[i] = (in[0] + in[1]) * 0.5;
                s--;
            }
            default: s = 0;
        }
        in++;
    }
}


void bmo_lpf_sinc_s(float *out, float *in, size_t samples, uint32_t sinc_len,
                    uint32_t rate_in, uint32_t rate_out)
{
    float sinc[sinc_len];     //FIXME recipe for stack overflow
    bmo_sinc_sb(sinc, sinc_len, (0.499 * rate_out) / rate_in);
    bmo_hamming_window(sinc, sinc_len);
    bmo_convolve_sb(out , in, sinc, samples, sinc_len);
}
