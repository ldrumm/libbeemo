#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../definitions.h"
#include "simple.h"
#include "../error.h"

void bmo_mix_sb(float * out, float * f1, float * f2, uint32_t samples)
{
	vec4f * vec_f1 = (vec4f *) f1;
	vec4f * vec_f2 = (vec4f *) f2;
	vec4f * vec_out = (vec4f *) out;
	if(samples % 4 == 0)
	{
		samples /= 4;
		while(samples--)
			vec_out[samples] = vec_f1[samples] + vec_f2[samples];
	}
	else
		while(samples--)
			out[samples] = f1[samples] + f2[samples];
}

void bmo_mix_mb(float ** out, float ** f1, float ** f2, uint32_t channels, uint32_t frames)
{
    while(channels--){
        bmo_mix_sb(out[channels], f1[channels], f2[channels], frames);
    }
}

void bmo_sbcpy(float * out, float * in, uint32_t samples)
{
//    bmo_debug("copying %d samples from %p to %p", samples, in, out);
	vec4f * vec_in = (vec4f *) in;  //FIXME test on non x86_64 platforms
	vec4f * vec_out = (vec4f *) out;
	if(samples % 4 == 0)
	{
		samples /= 4;
		while(samples--)
			vec_out[samples] = vec_in[samples];
	}
	else
		while(samples--)
			out[samples] = in[samples];
}

void bmo_sbcpy_offset(float * out, float * in, uint32_t out_offset, uint32_t in_offset, uint32_t samples)
{
    out = out + out_offset;
    in = in + in_offset;
    for (uint32_t i = 0; i < samples; i++){
        out[i] = in[i];
    }
}


void _bmo_dither_sb(float * buffer, uint32_t resolution)
{
    (void) buffer;
    (void) resolution;
    BMO_NOT_IMPLEMENTED;
	return;
}

//void bmo_osc_sine_mix_sb(float * buffer, float freq, float phase, float amplitude, uint32_t rate, uint32_t samples)
//{
//    bmo_info("%fHz\n", freq);
//	//fast sine generator: adapted from musicdsp.org posted by Niels Gorisse based on work by Julius O Smith
//	float y2, y0, y1, w;
//	w = freq * 2.0 * M_PI / rate;
//	float b1 = 2.0 * cos(w);
//
//	y1 = sin(phase - w);
//	y2 = sin(phase - 2.0 * w);
//    bmo_debug("phase:%f\n", phase);
//	for (size_t f = 0; f < samples ; f++){
//		y0 = b1 * y1 - y2;
//		y2 = y1;
//		y1 = y0;
//		buffer[f] += y0 * amplitude;
//	}
//}


void bmo_mb_cpy_offset(float ** dest, float **src, size_t src_off, size_t dest_off, uint32_t channels, uint32_t frames)
{
    for(uint32_t ch = 0; ch < channels; ch++){
        for(uint32_t i = 0; i < frames; i++){
            dest[ch][i+dest_off] = src[ch][i+src_off];
        }
    }
}

void bmo_mb_cpy(float ** dest, float ** src, uint32_t channels, size_t frames)
{
    while(channels--){
        bmo_sbcpy(dest[channels], src[channels], frames);
    }
}

double bmo_osc_sine_mix_sb(float * buffer, float freq, double phase, float amplitude, uint32_t rate, uint32_t samples)
{
    double rad_per_sample = (2. * M_PI * freq)/rate;
	for (size_t f = 0; f < samples ; f++){
		buffer[f] += sin(phase) * amplitude;
		phase += rad_per_sample;
	}
	return fmod(phase, M_PI * 2.);
}

double bmo_osc_sq_mix_sb(float * buffer, float freq, double phase, float amplitude, uint32_t rate, uint32_t samples)
{
// uses the fast sine generator from above and clamps the peaks of the sine wave.
// the resultant wave is an approximate bandlimited squarewave, though not a true odd harmonic sine-additive squarewave.
// It has a low slew rate, and only an approximation of a true square wave but could be accepted considering its execution speed(it's faster than audacity)
	float  y0, y1, y2, w;
	w = freq * 2.0 * M_PI / rate;
	float b1 = 2.0 * cos(w);
	float x;
	y1 = sin(phase - w);
	y2 = sin(phase - 2 * w);
	amplitude *= 2;	// all samples are halved during clamping, so to get the requested amplitude, we must double again.

	while(samples --)
	{
		y0 = b1 * y1 - y2;
		y2 = y1;
		y1 = y0;
		x = y0;
		x = _bmo_clampf(x * 8., 1.);
		buffer[samples] += x * amplitude;
	}
	return phase + (2. * M_PI * samples * freq) / rate;
}

double bmo_osc_sq_mix_sbB(float * buffer, float freq, double phase, float amplitude, uint32_t rate, uint32_t samples)  //TODO phase offset, frequency
{
    (void) phase;
	float w;
	w = freq * (2.0 * M_PI / rate);
	while(samples --)
	{
		buffer[samples] += amplitude * sin(2. * cos(-2. * cosh(2. * cos(w * samples * 0.125))));
	}
	return phase;
}

double bmo_osc_saw_sb(float * buffer, float freq, double phase, float amplitude, uint32_t rate, uint32_t samples)
{
		float a1;
		float inc;
		a1 = rate / freq;
		inc = (2.0 * amplitude) / a1;
		phase = fmod(phase, 360.);
		phase /=360.;
		phase *= a1;

		while(samples--)
			buffer[samples] += ((inc * ((samples + (int)phase) % (int)a1)) - amplitude) ;
    return phase;
}

void bmo_gain_sb(float * buffer, uint32_t samples, float gain_db)
{
	float gain = powf(10.0, gain_db * 0.05);
	vec4f * vec_buf =(vec4f *) buffer;
	vec4f gain_vec = {gain, gain, gain, gain};
	if(samples % 4 == 0)
	{
		samples /= 4;
/*		gain_vec[0] = gain;*/
/*		gain_vec[1] = gain;*/
/*		gain_vec[2] = gain;*/
/*		gain_vec[3] = gain;*/
		while(samples--)
			vec_buf[samples] *= gain_vec;
	}
	else
	while(samples--)
		buffer[samples] *= gain;
}

void bmo_inv_sb(float * buffer, uint32_t samples)
{
	int * int_buf = (int *) buffer;
	vec4i * vec_buf = (vec4i *) buffer;
	vec4i sign_bit = {(1 << SIGN_BIT),(1 << SIGN_BIT),(1 << SIGN_BIT),(1 << SIGN_BIT)};
	if(samples % 4 == 0){
		samples /= 4;
		while(samples--)
			vec_buf[samples] ^= sign_bit;
	}
	else{
		while(samples--)
			int_buf[samples] ^= (1 << SIGN_BIT);
	}
}

void bmo_zero_sb(float * buffer, uint32_t samples)
{
	// ieee 754 has all bits set to zero for 0.0
	memset(buffer, 0, samples * sizeof(float));
}

void bmo_zero_sb2(float * in, uint32_t samples)
{
	int * int_buf = (int *) in;
	vec4i * vec_buf = (vec4i *) in;
	if(samples % 4 == 0){
		samples /= 4;
		while(samples--)
			vec_buf[samples] ^= vec_buf[samples];
	}
	else{
		while(samples--)
			int_buf[samples] = 0;
	}
}

void bmo_zero_sb3(float * in, uint32_t samples)
{
	int * int_buf = (int *) in;
	vec4i * vec_buf = (vec4i *) in;
	vec4i vec_zero = {0x00000000, 0x00000000, 0x00000000, 0x00000000};
	if(samples % 4 == 0){
		samples /= 4;
		while(samples--)
			vec_buf[samples] = vec_zero;
	}
	else{
		while(samples--)
			int_buf[samples] = 0;
	}
}

void bmo_zero_mb(float ** in, uint32_t channels, uint32_t frames)
{
    for(uint32_t i = 0; i < channels; i++){
        bmo_zero_sb(in[i], frames);
    }
}

void bmo_zero_mb_off(float ** in, uint32_t channels, uint32_t frames, uint32_t offset)
{
    for(uint32_t i = 0; i < channels; i++){
        bmo_zero_sb(in[i]+offset, frames);
    }
}


void bmo_convolve_sb(float * out, float * in, float * impulse, uint32_t samples, uint32_t impulselen)
{
	uint32_t i, j;
	float * tmp;
	float *freeme;
	freeme = tmp = calloc(samples, sizeof(float));   //FIXME this is not suitable for a realtime context. remove malloc call and try and avoid temporary storage.
	if(!tmp){
		bmo_err("allocation error:%s\n", strerror(errno));
		return;
	}
	/*convolveHack input with impulse*/
	for(i = 0; i < samples; i++){
	    float f;
		f = in[i];
		tmp++;
		for(j = 0; j < impulselen; j++){
			if((j + i) < samples)                   //TODO remove conditional to speed up execution
				tmp[j] += (impulse[j] * f);   //TODO vectorize this mixing code
		}
	}
	bmo_sbcpy(out, tmp - samples, samples);
	free(freeme);
	return;
}


void bmo_lerp_sb(float * out, float * in, float ratio, size_t out_frames)
{
/**
	bmo_lerp_sb() is a fast linear interpolator based on simple trig.
	For linear interpolation, the intermediate value is
	calculated by scaling the right triangle created between
	the next and previous origf1l samples.
	This should be a fairly fast method. \f$a_1\f$ needs only to
	be calculated once - i.e. the constant time between two samples.
	\f$b_2 = a_2*(b_1/a_1)\f$
	\verbatim
		prev		next
		|           |
		|			V
		|			/
		|		  /	|
		|		/  	|
		|	  /|	|
		|	/  |b2 	|b1
		V /	   |   _|
		/______|__|_|

		|<-a2->|
		|<-	  a1  ->|
	\endverbatim*/
	size_t out_idx = 0;
	float a1, a2, b1, b2;
	a1 = ratio;
	float inv_a1 = 1. / ratio;
	float prev, next;
	float f_idx = 0.0f;

	while(out_idx < out_frames)
	{
		f_idx = out_idx * inv_a1;
		prev = in[lrintf(f_idx)];
		next = in[lrintf(f_idx +1)];
		a2 = f_idx - floorf( f_idx );			/// ca length of a2
		b1 = next - prev;						///	find the height (b1) of triangle
		b2 = a2 * (b1 * inv_a1);				///
		out[out_idx] = prev + (a1 * b2);
		out_idx++;
	}
}
