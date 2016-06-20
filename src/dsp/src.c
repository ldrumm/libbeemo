#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../definitions.h"
#include "../error.h"
#include "src.h"
#include "lpf.h"
#include "simple.h"

void bmo_src_sb(float * out, float * in, float rate_in, float rate_out, uint32_t flags, size_t samples_in)
{
	float ratio = rate_out / rate_in;
	size_t samples_out = (size_t)floor(0.5 + (samples_in * ratio));
	double idx_in = 0.0;	
	size_t idx_out = 0;

    bmo_debug("rate_in=%f rate_out=%f\n", rate_in, rate_out);
	bmo_debug("ratio between in and out = %f\n", ratio);
	bmo_debug("samples in\t=%d\n", samples_in);
	bmo_debug("samples out\t=%d\n", samples_out);

	switch(flags & 0xffff0000)
	{
		case NO_FILT: break;
		
		case LOW_Q_FILT:
		{
			bmo_debug("doing peephole LPF\n");
			bmo_lpf_peephole_s(out, in, samples_in);
			break;
		}
		
		case HI_Q_FILT:
		{
		    bmo_debug("doing sinc LPF\n");
		    bmo_lpf_sinc_s(out, in, samples_in, BMO_SINC_HIGH, rate_in, rate_out);
			break;
		}
		default: assert(0);	/// higher level functions should properly select a filter.
	}/*switch filter*/

	/* select interpolation method? */
	switch(flags & 0x0000ffff)
	{
		case ZOH_NO_INTERP:
		{
			bmo_debug("doing ZOH_NO_INTERP sample rate conversion\n");
		/**
		ZOH_NO_INTERP is the most basic form of sample rate conversion - 
		no interpolation is used - it's a fast nasty direct mapping across sample boundaries. 
		Only really ever useful when you have very little available processor time. */	
			while(idx_out < samples_out)
			{
				idx_in = idx_out / ratio;
				out[idx_out] = in[lrintf(idx_in)];
				idx_out++;
			}
			break;
		}
		
		case LINEAR_INTERP:
		{
		    bmo_debug("doing linear interpolation\n");
			bmo_lerp_sb(out, in, ratio, samples_out);
			break;
		}

		case POLYNOM_INTERP:
		{
		    BMO_NOT_IMPLEMENTED;
		}
		default: assert(0); ///a valid interpolator should always be chosen by the higher functions.
	}/*switch interpolator*/
}

