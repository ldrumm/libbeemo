#include "simple.h"
#include "sinc.h"
#include "../definitions.h"

void bmo_lpf_peephole_s(float * out, float * in, size_t samples)
{
    /**peephole LPF - attempt 1
    a sliding window averaging filter based on a multiple pass variable window
    length. Very poor cutoff curve at about 3dB/8ve, though the duff's device
    implementation makes it fast as it produces nicely vectorized code*/

    int s = 8;
    for(size_t i = 0; i < samples -8 ; i++)
    {
        float temp;
	    switch(s)
	    {
		    case 8:
		    {
			    temp = (
                    in[0]
			        + in[1]
			        + in[2]
			        + in[3]
			        + in[4]
			        + in[5]
			        + in[6]
			        + in[7]
			    ) * 0.125;
			    out[i] = temp;
			    s--;
		    }
		    case 7:
		    {
			    temp = (
			        in[0]
			        + in[1]
			        + in[2]
			        + in[3]
			        + in[4]
			        + in[5]
			        + in[6]
			    ) * 0.142857142857;
			    out[i] = temp;
			    s--;
		    }
		    case 6:
		    {
			    temp = (
			        in[0]
			        + in[1]
			        + in[2]
			        + in[3]
			        + in[4]
			        + in[5]
			    ) * 0.166666666667;
			    out[i] = temp;
			    s--;
		    }
		    case 5:
		    {
			    temp = (
			        in[0]
			        + in[1]
			        + in[2]
			        + in[3]
			        + in[4]
			    ) * 0.2;
			    out[i] = temp;
			    s--;
		    }
		    case 4:
		    {
			    temp = (
			        in[0]
			        + in[1]
			        + in[2]
			        + in[3]
			    ) * 0.25;
			    out[i] = temp;
			    s--;
		    }
		    case 3:
		    {
			    temp = (
			        in[0]
			        + in[1]
			        + in[2]
			    ) * 0.333333333333;
			    out[i] = temp;
			    s--;
		    }
		    case 2:
		    {
			    temp = (
			        in[0]
			        + in[1]
			    ) * 0.5;
			    out[i] = temp;
			    s--;
		    }
		    default:s = 0;
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
