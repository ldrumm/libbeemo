#ifdef BMO_HAVE_SNDFILE
#include <stdio.h>
#include <sndfile.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "buffer.h"
#include "multiplexers.h"
#include "error.h"
#include "dsp/simple.h"

static int 
_bmo_get_bo_sndfile(void * bo, float **dest, uint32_t frames)
{
	assert(bo && dest);
	BMO_buffer_obj_t * obj = (BMO_buffer_obj_t *) bo;
	float * buffer = malloc(sizeof(float)* frames * obj->channels);
	if(!buffer){
	    bmo_err("Failed to load data from sndfile:%s\n", strerror(errno));
	    bmo_zero_mb(dest, obj->channels, frames);
	    return -1;
	}
	sf_count_t read = sf_readf_float(obj->handle, buffer, (sf_count_t) frames) ;
	bmo_conv_ibftomb(dest, (char *) buffer, obj->channels, (uint32_t)read, BMO_FMT_NATIVE_FLOAT);
	free(buffer);
	return read;
}

static int 
_bmo_seek_bo_sndfile(void * bo, long index, int whence)
{
    whence = whence ? whence : SEEK_SET;
	assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);
	assert(bo);
	return sf_seek((SNDFILE *)((BMO_buffer_obj_t *)bo)->handle, (sf_count_t) index, whence) ;
}

BMO_buffer_obj_t * 
_bmo_fopen_sndfile(const char * path, uint32_t flags)
{
	(void)flags; //TODO
	SNDFILE * sf;
	SF_INFO info = {0,0,0,0,0,0};
	BMO_buffer_obj_t * obj;
	info.format = 0;	//readonly
	
	sf = sf_open(path, SFM_READ, &info);
	if(!sf){
		bmo_err("couldn't open file\n");
		return NULL;
	}
	obj = bmo_bo_new(BMO_EXTERNAL_DATA, (uint32_t)info.channels, (size_t)info.frames,  (uint32_t)info.samplerate, 0, 0, sf);
	obj->seek = _bmo_seek_bo_sndfile;
	obj->get_samples = _bmo_get_bo_sndfile;
	return obj;
}
#endif
