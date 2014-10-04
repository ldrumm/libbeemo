#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "definitions.h"
#include "dsp/simple.h"
#include "error.h"

static inline int64_t _bmo_pcmtoint64(char * in, int swap_in, uint32_t stride_in, int64_t range)
{
	int64_t tmp = 0x0000000000000000;
	uint32_t offset;
	#ifdef BMO_ENDIAN_LITTLE
	offset = 0;
	#else
	offset = 8 - stride_in;
	#endif
	char * cp = (char *) &tmp;
	if(swap_in)
	{
		uint32_t k = stride_in -1;
		for(uint32_t i = 0; i < stride_in; i++){
			cp[i + offset] = in[k];
			k--;
		}
	}
	else
	{
		for(uint32_t i = 0; i < stride_in; i++)	
			cp[i + offset] = in[i];
	}
	if(tmp >=  (range / 2))	//sign carry and convert to signed if input is unsigned.
	{
		tmp = 0 - (range - tmp);
	}
	return tmp;
}

static inline void _bmo_int64topcm(char * out, int64_t in, int swap_out, uint32_t stride)
{
	uint32_t offset;
	int64_t tmp = in;
	#ifdef BMO_ENDIAN_LITTLE
	offset = 0;
	#else
	offset = 8 - stride;
	#endif
	char * cp = (char *) &tmp;
	if(swap_out)
	{
		uint32_t k = stride -1;
		for(uint32_t i = 0; i < stride; i++){
			out[i] = cp[offset + k];
			k--;
		}
	}
	else
	{						
		for(uint32_t o = 0; o < stride; o++)
			out[o] = cp[offset + o];
	}
}


uint32_t 
bmo_host_endianness(void)
{
	union endian
	{
		uint32_t a;
		char b[4];
	}	endian;
	
	endian.a = 1;
	
	return endian.b[3] == 1 ? ENDIAN_BIG: ENDIAN_LITTLE ;
}


int
bmo_host_be(void)
{
	return(bmo_host_endianness() == ENDIAN_BIG ? 1 : 0);
}


int 
bmo_host_le(void)
{
	return(bmo_host_endianness() == ENDIAN_LITTLE ? 1 : 0);
}


void 
_bmo_swap_16(void * in)
{
	union bits{
		uint16_t word;
		uint8_t bytes[2];
	}bits;
	uint8_t * cp = (uint8_t *) in;
	bits.word = *((uint16_t *)in);
	cp[0] = bits.bytes[1];
	cp[1] = bits.bytes[0];
}


void 
_bmo_swap_24(void * data)
{
	uint8_t * cp = (uint8_t *) data;
	
	uint8_t bytes[3];
	bytes[0] = cp[2];
	bytes[2] = cp[0];
	
	cp[0] = bytes[0];
	cp[2] = bytes[2];

}


void 
_bmo_swap_32(void * data)
{
	uint8_t * cp = (uint8_t *) data;
	union 
	{
		uint32_t word;
		uint8_t bytes[4];
	}temp;

	temp.bytes[0] = cp[3];
	temp.bytes[1] = cp[2];
	temp.bytes[2] = cp[1];
	temp.bytes[3] = cp[0];
	*((uint32_t *)data) = temp.word;
}


void 
_bmo_swap_64(void * data)
{
	uint8_t * cp = (uint8_t *) data;
	union 
	{
		uint64_t word;
		uint8_t bytes[8];
	}temp;

	temp.bytes[0] = cp[7];
	temp.bytes[1] = cp[6];
	temp.bytes[2] = cp[5];
	temp.bytes[3] = cp[4];
	temp.bytes[4] = cp[3];
	temp.bytes[5] = cp[2];
	temp.bytes[6] = cp[1];
	temp.bytes[7] = cp[0];
	
	*((uint64_t *)data) = temp.word;
	return;
}


void 
_bmo_swap_ib(void * data, uint32_t stride, size_t len)
{
	assert(data);	//debug
	size_t i = 0;
	switch(stride)
	{
		case 1: return;		// single byte variables don't have endianness - do nothing
		case 2:
		;
			uint16_t * p16 = (uint16_t *) data;
			while(i < len)
			{
				_bmo_swap_16(p16);
				p16++;
				i++;
			}
			break;
		case 3:
		;
			char * p8 = (char *) data;
			while(i < len)
			{
				_bmo_swap_24(p8);
				p8 += stride;
				i++;
			}
			break;
		case 4:
		;
			uint32_t *p32 = (uint32_t *) data;
			while(i < len)
			{
				_bmo_swap_32(p32);
				p32++;
				i++;
			}
			break;
		case 8:
		;
			uint64_t *p64 = (uint64_t *) data;
			while(i < len)
			{
				_bmo_swap_64(p64);
				p64++;
				i++;
			}
			break;
			default:assert(0);	
	}
	return;
}


uint32_t 
bmo_fmt_enc(uint32_t flags)
{
	return flags & (  
        BMO_FMT_PCM_U8       |		
        BMO_FMT_PCM_8 	    |
        BMO_FMT_PCM_16_LE	|	
        BMO_FMT_PCM_24_LE	|	
        BMO_FMT_PCM_32_LE	|	
        BMO_FMT_FLOAT_32_LE  |	
        BMO_FMT_FLOAT_64_LE	|	
        BMO_FMT_PCM_16_BE	|
        BMO_FMT_PCM_24_BE	|	
        BMO_FMT_PCM_32_BE	|
        BMO_FMT_FLOAT_32_BE	|	
        BMO_FMT_FLOAT_64BE	|
        BMO_FMT_SNDFILE			|
        BMO_FMT_NATIVE_FLOAT
    );
}


uint32_t 
bmo_fmt_stride(uint32_t flags)
{
	switch (bmo_fmt_enc(flags))
	{
		case BMO_FMT_PCM_U8:return 1;
		case BMO_FMT_PCM_8: return 1;
		case BMO_FMT_PCM_16_LE:return 2;
		case BMO_FMT_PCM_24_LE:return 3;
		case BMO_FMT_PCM_32_LE:return 4;
		case BMO_FMT_FLOAT_32_LE:return 4;
		case BMO_FMT_FLOAT_64_LE:return 8;
		case BMO_FMT_PCM_16_BE:return 2;
		case BMO_FMT_PCM_24_BE:return 3;
		case BMO_FMT_PCM_32_BE:return 4;
		case BMO_FMT_FLOAT_32_BE:return 4;
		case BMO_FMT_FLOAT_64BE:return 8;
		case BMO_FMT_NATIVE_FLOAT: return sizeof(float);
		case BMO_FMT_SNDFILE:	return sizeof (float);	//libsndfile interface only uses native floats
//		default:return 0;
	}
	assert(0);
	return 0x0; //compiler complains without the return
}


int
bmo_fmt_pcm(uint32_t flags)
{
	switch (bmo_fmt_enc(flags))
	{
		case BMO_FMT_PCM_U8:return 1;
		case BMO_FMT_PCM_8: return 1;
		case BMO_FMT_PCM_16_LE:return 1;
		case BMO_FMT_PCM_24_LE:return 1;
		case BMO_FMT_PCM_32_LE:return 1;
		case BMO_FMT_FLOAT_32_LE:return 0;
		case BMO_FMT_FLOAT_64_LE:return 0;
		case BMO_FMT_PCM_16_BE:return 1;
		case BMO_FMT_PCM_24_BE:return 1;
		case BMO_FMT_PCM_32_BE:return 1;
		case BMO_FMT_FLOAT_32_BE:return 0;
		case BMO_FMT_FLOAT_64BE:return 0;
		case BMO_FMT_SNDFILE: return 0;	//sndfile interfaces only use libsndfile's float routines
		default: return 0;
	}
	assert(0);
	return 0x0; //compiler complains without the return
}


uint32_t 
bmo_fmt_end(uint32_t flags)
{
	switch (bmo_fmt_enc(flags))
	{
		case BMO_FMT_PCM_8:return bmo_host_endianness();
		case BMO_FMT_PCM_16_LE:return ENDIAN_LITTLE;
		case BMO_FMT_PCM_24_LE:return ENDIAN_LITTLE;
		case BMO_FMT_PCM_32_LE:return ENDIAN_LITTLE;
		case BMO_FMT_FLOAT_32_LE:return ENDIAN_LITTLE;
		case BMO_FMT_FLOAT_64_LE:return ENDIAN_LITTLE;
		case BMO_FMT_PCM_16_BE:return ENDIAN_BIG;
		case BMO_FMT_PCM_24_BE:return ENDIAN_BIG;
		case BMO_FMT_PCM_32_BE:return ENDIAN_BIG;
		case BMO_FMT_FLOAT_32_BE:return ENDIAN_BIG;
		case BMO_FMT_FLOAT_64BE:return ENDIAN_BIG;
		case BMO_FMT_NATIVE_FLOAT: return bmo_host_endianness();
		default: return 0x0;
	}
	assert(0);
	return 0x0; //compiler complains without the return
}

uint32_t 
bmo_fmt_dither(uint32_t flags)
{
	return flags & (  
        BMO_DITHER_SHAPED	|	
        BMO_DITHER_TPDF		
    );
}

uint32_t 
_bmo_fmt_conv_type(uint32_t fmt_out, uint32_t fmt_in)
{
	//careful of the short-circuit logic!
	int pcm_in = bmo_fmt_pcm(fmt_in);
	int pcm_out = bmo_fmt_pcm(fmt_out);
	if((pcm_in & pcm_out))
		return BMO_INT_TO_INT;
	if(((pcm_in | pcm_out) == 0))
		return BMO_FLOAT_TO_FLOAT;
	if(pcm_in == 0)				
		return BMO_FLOAT_TO_INT;
	return BMO_INT_TO_FLOAT;
}


float 
_bmo_fmt_pcm_range(uint32_t fmt)
{
    //powers of two can be completely represented by s.p. float up to 
	switch (bmo_fmt_stride(fmt) * 8)
	{
		case  8: return 256.0;
		case 12: return 4096.0;
		case 16: return 65536.0;
		case 20: return 1048576.0;
		case 24: return 16777216.0;
		case 32: return 4294967296.0;
		case 48: return 281474976710656.0;
		case 64: return 18446744073709551616.0;
		default: return powf(2, bmo_fmt_stride(fmt) * 8);
	}
	assert(0);
}

#define _bmo_to_native_must_swap(x)((bmo_fmt_end((x)) != bmo_host_endianness()))

void 
bmo_conv_iftoif(void * out, void * in, uint32_t fmt_out, uint32_t fmt_in, uint32_t samples)
{
	bmo_debug("\n");
	assert((!bmo_fmt_pcm(fmt_in)) && (!bmo_fmt_pcm(fmt_out)));
	
	uint32_t endian_in =  bmo_fmt_end(fmt_in);
	uint32_t endian_out =  bmo_fmt_end(fmt_out);
	
	int swap_in = _bmo_to_native_must_swap(fmt_in);
	int swap_out = _bmo_to_native_must_swap(fmt_out);
	float f;
	float *fp;
	double d;
	double * dp;
	switch(bmo_fmt_stride(fmt_in))
	{
		case 4:
		{
			if(bmo_fmt_stride(fmt_out) == sizeof(float))		//float to float
			{ 
				memcpy(out, in, samples * sizeof(float));
				if(endian_in != endian_out)
					_bmo_swap_ib(out, sizeof(float), samples);
			}
			else if(bmo_fmt_stride(fmt_out) == sizeof(double))	//float to double
			{
				fp = (float *) in;
				dp = (double *) out;
				for(uint32_t i = 0; i < samples; i++)
				{
					f = fp[i];
					if(swap_in)
						_bmo_swap_32(&f);
					dp[i] = (double)f;
					if(swap_out)
						_bmo_swap_64(dp + i);	
				}
			}
			break;
		}
		case 8:
		{
			if(bmo_fmt_stride(fmt_out)	==	sizeof(double))		//double to double
			{ 
				memcpy(out, in, samples * sizeof(double));
				if(endian_in != endian_out)
					_bmo_swap_ib(out, sizeof(double), samples);
			}
			else if(bmo_fmt_stride(fmt_out) == sizeof(float))	//double to float
			{
				dp = (double *) in;
				fp = (float *) out;
				for(uint32_t i = 0; i < samples; i++)
				{
					d = dp[i];
					if(swap_in)
						_bmo_swap_64(&d);
					fp[i] = (float) d;
					if(swap_out)
						_bmo_swap_32(fp + i);	
				}
			}
			break;
		}
		default:assert(0);
	}
}


void 
bmo_conv_iftoipcm(void * out_stream, void * in_stream, uint32_t fmt_out,  uint32_t fmt_in, uint32_t count)
{	
    bmo_debug("\n");
	assert(((!bmo_fmt_pcm(fmt_in)) && bmo_fmt_pcm(fmt_out)));
	
	float range = (float)_bmo_fmt_pcm_range(fmt_out);
	uint32_t stride_out = bmo_fmt_stride(fmt_out);

	int swap_in = _bmo_to_native_must_swap(fmt_in);
	int swap_out = _bmo_to_native_must_swap(fmt_out);

	uint32_t i;
	char * out = (char *) out_stream;
    
    int64_t pcm;
	if(bmo_fmt_stride(fmt_in)== 4)	//standard float
	{	
		float f;
		float * fp = (float *) in_stream;
		for(i = 0; i < count; i++)
		{
			f = fp[i];
			if(swap_in)
				_bmo_swap_32(&f);
			pcm = (int64_t) (f * range / 2);
			_bmo_int64topcm(out, pcm, swap_out, stride_out);
			out += stride_out;
		}
	}
	else if(bmo_fmt_stride(fmt_in) == 8)	//standard double
	{	
		double d;
		double * dp = (double *) in_stream;
		for(i = 0; i < count; i++)
		{
			d = dp[i];
			if(swap_in)
				_bmo_swap_64(&d);
			pcm = (int64_t) (d * range / 2);
			_bmo_int64topcm(out, pcm, swap_out, stride_out);
			out += stride_out;
		}
	}
	return;
}


void 
bmo_conv_ipcmtoipcm(void * out_stream, void * in_stream, uint32_t fmt_out, uint32_t fmt_in, uint32_t count)
{	
    bmo_debug("\n");
	assert((bmo_fmt_pcm(fmt_in) && bmo_fmt_pcm(fmt_out)));
	
	char * in = (char *) in_stream;
	char * out = (char *) out_stream;
	
	//check the stupid case
	if(fmt_in == fmt_out)
		memcpy(out_stream, in_stream, count * bmo_fmt_stride(fmt_in));
		
	/** copies the interleaved data from in_stream to out_stream, converting the sample data as requested.
	TODO where possible (i.e. where conversion is int -> int), integer only arithmetic is used to speed up the computation*/
	/**for integer conversion:
		type A is wider than B:
			B = A/(Arange/Brange)
		else:
			B = A*((Arange/Brange)
	
	At present, normalise all input samples to signed 64bit, then normalize to output format to avoid conditional in inner loop
	*/
	int swap_in = _bmo_to_native_must_swap(fmt_in);
	int swap_out = _bmo_to_native_must_swap(fmt_out);
	
	uint32_t stride_out = bmo_fmt_stride(fmt_out);
	uint32_t stride_in = bmo_fmt_stride(fmt_in);
	
	int64_t factor = (int64_t)(_bmo_fmt_pcm_range(fmt_in) / _bmo_fmt_pcm_range(fmt_out));
	uint32_t shrink = ((stride_in > stride_out) ? 1 : 0);
	int64_t range = (int64_t)_bmo_fmt_pcm_range(fmt_in);
	int64_t pcm;

	for(uint32_t f = 0; f < count; f++)
	{
		pcm = _bmo_pcmtoint64(in, swap_in, stride_in, range);
		in += stride_in;

		//scale temp according to in/out ratio
		if(shrink)
			pcm /= factor;					
		else
			pcm *= factor;
		_bmo_int64topcm(out, pcm, swap_out, stride_out);
		out += stride_out;	
		//copy temp to output
	}
	return;
}


void 
bmo_conv_ipcmtoif(float * out, void * in_stream, uint32_t fmt_out, uint32_t fmt_in, uint32_t count)
{	
    bmo_debug("\n");
	assert((bmo_fmt_pcm(fmt_in) && (!bmo_fmt_pcm(fmt_out))));
	assert(bmo_fmt_stride(fmt_out) == 4);
	
	int swap_in = _bmo_to_native_must_swap(fmt_in);
	int swap_out = _bmo_to_native_must_swap(fmt_out);

	
	char * in = (char *) in_stream;
	int64_t pcm;
	float f;
	int64_t range = _bmo_fmt_pcm_range(fmt_in);
	uint32_t stride_in = bmo_fmt_stride(fmt_in);
	
	for(uint32_t s = 0; s < count; s++)
	{
		pcm = _bmo_pcmtoint64(in, swap_in, stride_in, range);
		in += stride_in;
		f = (float)pcm / (range / 2);;
		if(swap_out)
			_bmo_swap_32(&f);
		out[s] = f;
	}
	return;
}


void 
bmo_conv_ibpcmtomb(float ** output_data, void * interleaved_data, uint32_t channels, uint32_t frames, uint32_t fmt_in)
{		
    bmo_debug("\n");
	assert(bmo_fmt_pcm(fmt_in));
	
	char * in = (char *) interleaved_data;
	int swap_in = _bmo_to_native_must_swap(fmt_in);
	uint32_t stride_in = bmo_fmt_stride(fmt_in);
	
	int64_t range = (int64_t) _bmo_fmt_pcm_range(fmt_in);
	float multiplier = (float) 1.0 / (range / 2.0);

		
	for(uint32_t f = 0; f < frames; f++)
	{
		for(uint32_t ch = 0; ch < channels; ch++)
		{
			output_data[ch][f] = (float)_bmo_pcmtoint64(in, swap_in, stride_in, range) * multiplier;
			in += stride_in;
		}
	}
}


int 
bmo_conv_ibftomb(float ** output_data, char * input_data, uint32_t channels, uint32_t frames, uint32_t fmt_in)
{	
    bmo_debug("\n");
	assert((!bmo_fmt_pcm(fmt_in)));
	assert(bmo_fmt_stride(fmt_in) == sizeof(float)); //FIXME can't yet read from double precision buffer
	uint32_t count = channels * frames;
	uint32_t swap =  _bmo_to_native_must_swap(fmt_in);
	float * interleaved_data = (float *)input_data;

	/* copy into output buffers */
	uint32_t i, j, frame;
	i = j = frame = 0;
	while(i < count) 
	{
		for(j = 0; j < channels; j++) 
		{
			output_data[j][frame] = interleaved_data[i];
		//	bmo_debug("%f\n", output_data[j][frame]);
			i++;
		}
		frame++;
	}
	/*convert to machine endianness if necessary */
	if(swap)
	{
		bmo_info("changing endian of floating point stream\n");
		for(i = 0; i < channels; i++)
		{
				_bmo_swap_ib(output_data[i], sizeof(float), frames);
		}
	}
	return 0;
}


void 
bmo_conv_mftoix(void * out, float ** in, uint32_t channels, uint32_t fmt_out, uint32_t frames)
{	
    bmo_debug("\n");
	assert( (bmo_fmt_pcm(fmt_out) | (bmo_fmt_stride(fmt_out) == 4)));	//TODO add double precision float output
	
	int swap_out = _bmo_to_native_must_swap(fmt_out);
	uint32_t stride_out = bmo_fmt_stride(fmt_out);
	float multiplier = (float)((_bmo_fmt_pcm_range(fmt_out) / 2.0) -1.0);
	char * out_buf = (char *) out;
	
	switch(bmo_fmt_pcm(fmt_out))
	{
		case 1:
		{
			;
			int64_t temp;
			for ( uint32_t f = 0; f < frames; f++ )
			{
				for( uint32_t ch = 0; ch < channels; ch++ )
				{
					temp = (int64_t) lrint(((double)in[ch][f] * (double) multiplier));
					_bmo_int64topcm(out_buf, temp, swap_out, stride_out);
					out_buf += stride_out;
				}
			}
			break;
		}
		case 0:
		{
			if(stride_out == sizeof(float))
			{
				float * outf = (float *)out;
				uint32_t sample = 0;
				for ( uint32_t f = 0; f < frames; f++ )
				{
					for( uint32_t ch = 0; ch < channels; ch++ )
					{
						outf[sample] = in[ch][f];
						if(swap_out)
							_bmo_swap_32(outf+sample);
						sample++;
					}
				}
			}
			else 
			{	;} //FIXME haven't sorted 64bit float outputs yet
			break;
		}
		default:assert(0);
	}	//switch
}


void 
bmo_conv_ibtoib(void * out, void * in, uint32_t fmt_out, uint32_t fmt_in, uint32_t count)
{	
    bmo_debug("\n");
	switch(_bmo_fmt_conv_type(fmt_out, fmt_in))
	{
		case BMO_INT_TO_INT:
		{
			bmo_conv_ipcmtoipcm(out, in, fmt_out, fmt_in, count);
			break;
		}
		
		case BMO_INT_TO_FLOAT://TODO
		{
			assert(bmo_fmt_stride(fmt_out) == sizeof(float));	//FIXME nteger PCM to double precision float conversion
			bmo_conv_ipcmtoif(out, in, fmt_out, fmt_in, count);
			break;
		}
		
		case BMO_FLOAT_TO_FLOAT://TODO
		{
			bmo_conv_iftoif(out, in, fmt_out, fmt_in, count);
			break;
		}
		
		case BMO_FLOAT_TO_INT:
		{
			bmo_conv_iftoipcm(out, in, fmt_out, fmt_in, count);	//TODO endianness conversions
			break;
		}
	}
}

