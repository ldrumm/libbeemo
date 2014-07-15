#include <string.h>
#include <assert.h>
#include <fenv.h>

#include "definitions.h"
#include "riff_wav.h"
#include "sun_au.h"
#include "error.h"
#include "multiplexers.h"
#include "import_export.h"
#include "memory/map.h"
#include "multiplexers.h"
#include "dither.h"
#ifdef BMO_HAVE_SNDFILE
#include "sndfile.h"
#endif


uint32_t bmo_ftype(const char * path)
{
	uint32_t ret = 0;
	int err;
	size_t size = bmo_fsize(path, &err);
	if(err){
	    bmo_err("%s does not exist", path);
	    return 0;
	}
	bmo_debug("filesize is %ld bytes", size);
	uint32_t * file = bmo_map(path, 0, 0);
	
	if(!file) 
		return 0;
	else ret = file[0];
	if(bmo_host_le())
		bmo_err("little Endian\n");
	_bmo_swap_32(&ret);
	bmo_unmap(file, size);
	return ret;
}

BMO_buffer_obj_t * 
bmo_fopen(const char * path, uint32_t flags)
{
	//naive implementation of a format tester that tests the filename only
	BMO_buffer_obj_t * obj = NULL;
	size_t len = strlen(path);
	assert(path);
	#ifdef BMO_HAVE_SNDFILE
	bmo_debug("using libsndfile to open '%s'\n", path);
	obj = _bmo_fopen_sndfile(path, BMO_EXTERNAL_DATA);
	if(!obj)
	    bmo_err("sndfile_open failed for '%s'\n", path);
	return obj;
	#endif
	if(strncmp(path + len - 4, ".wav", 4) == 0)
		return bmo_fopen_wav(path, flags);
	if(strncmp(path + len - 3, ".au", 3) == 0)
		return bmo_fopen_sun(path, flags);
	if(!obj){	
		bmo_err("open failed for '%s'\n", path);
		return NULL;
	}
	return obj;
}

size_t 
bmo_fwrite_mb(FILE * file, float ** in_buf, uint32_t channels, uint32_t out_fmt, uint32_t frames, uint32_t dither)
{	
	bmo_debug("\n");
	uint32_t i;
	char * tmp = malloc(frames * channels * bmo_fmt_stride(out_fmt));
	if(!tmp)
	{
		bmo_err("alloc failure\n");
		return 0;
	}
	
	fesetround(FE_TONEAREST);	//should be true in c99/posix
	bmo_conv_mftoix(tmp, in_buf, channels, out_fmt, frames);
	
	if(bmo_fmt_pcm(out_fmt))
	{
		switch(dither)
		{
			case BMO_DITHER_TPDF:	bmo_dither_tpdf(tmp, out_fmt, frames * channels);break;
			case BMO_DITHER_SHAPED: assert("shaped dither not yet implemented" == NULL);break;
			default:break;
		}
	}	
	
	i = fwrite(tmp, 1, frames * channels * bmo_fmt_stride(out_fmt), file);
	free(tmp);
	return i;
}

size_t 
bmo_fwrite_ib(FILE * file, void * in, uint32_t channels, uint32_t out_fmt, uint32_t in_fmt, uint32_t frames, uint32_t dither)
{	
	bmo_debug("\n");
	uint32_t i;
	char * tmp = malloc(frames * channels * bmo_fmt_stride(out_fmt));
	if(!tmp)
	{
		bmo_err("alloc failure\n");
		return 0;
	}
	
	bmo_conv_ibtoib(tmp, in, out_fmt, in_fmt, frames * channels);
	switch(dither)
	{
		case BMO_DITHER_TPDF:bmo_dither_tpdf(tmp, out_fmt, frames * channels);break;
		case BMO_DITHER_SHAPED: assert("shaped dither not yet implemented" == NULL);
		default:break;
	}
	i = fwrite(tmp, bmo_fmt_stride(out_fmt), frames * channels, file);
	free(tmp);
	return i;
}
