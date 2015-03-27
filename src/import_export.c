#include <string.h>
#include <errno.h>
#include <assert.h>
#include <fenv.h>
#include "definitions.h"
#include "dsp_obj.h"
#include "riff_wav.h"
#include "sun_au.h"
#include "error.h"
#include "multiplexers.h"
#include "import_export.h"
#include "memory/map.h"
#include "multiplexers.h"
#include "dither.h"
#include "dsp/simple.h"
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
	assert(path);

#ifdef BMO_HAVE_SNDFILE
    (void)flags;
	bmo_debug("using libsndfile to open '%s'\n", path);
	obj = _bmo_fopen_sndfile(path, BMO_EXTERNAL_DATA);
	if(!obj)
	    bmo_err("sndfile_open failed for '%s'\n", path);
	return obj;
#else
        //buffer all data into memory if no flag given.
	if(!(flags & (BMO_BUFFERED_DATA | BMO_MAPPED_FILE_DATA))){
	    flags |= BMO_BUFFERED_DATA;
	}
	size_t len = strlen(path);
	if(strncmp(path + len - 4, ".wav", 4) == 0)
		return bmo_fopen_wav(path, flags);
	if(strncmp(path + len - 3, ".au", 3) == 0)
		return bmo_fopen_sun(path, flags);
	if(!obj){
		bmo_err("open failed for '%s'\n", path);
		return NULL;
	}
#endif
    bmo_err("couldn't open %s\n", path);
	return obj;
}

size_t
bmo_fwrite_mb(FILE * file, float ** in_buf, uint32_t channels, uint32_t out_fmt, uint32_t frames, uint32_t dither)
{
	bmo_debug("%p->%p [%u][%uframes]", in_buf, file, channels, frames);
	uint32_t i;
	char * tmp = malloc(frames * channels * bmo_fmt_stride(out_fmt));
	if(!tmp){
		bmo_err("alloc failure\n");
		return 0;
	}

	fesetround(FE_TONEAREST);	//should be true in c99/posix
	bmo_conv_mftoix(tmp, in_buf, channels, out_fmt, frames);

	if(bmo_fmt_pcm(out_fmt)){
		switch(dither){
			case BMO_DITHER_TPDF:	bmo_dither_tpdf(tmp, out_fmt, frames * channels);break;
			case BMO_DITHER_SHAPED: BMO_NOT_IMPLEMENTED; break;
			default:break;
		}
	}
	bmo_debug("%u\n", bmo_fmt_stride(out_fmt));
    bmo_debug("%p, %u,                      %u *    %u,         %p\n",  tmp, bmo_fmt_stride(out_fmt), frames, channels, file);
	i = fwrite(tmp, bmo_fmt_stride(out_fmt), frames * channels, file);
	free(tmp);
	return i;
}

size_t
bmo_fwrite_ib(FILE * file, void * in, uint32_t channels, uint32_t out_fmt, uint32_t in_fmt, uint32_t frames, uint32_t dither)
{
	bmo_debug("%p->%p [%u][%uframes]", in, file, channels, frames);
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


BMO_dsp_obj_t *
bmo_dsp_bo_new_fopen(const char * path, uint32_t flags, uint32_t frames)
{
    BMO_buffer_obj_t * bo = bmo_fopen(path, flags);
    if(!bo){
        bmo_err("couldn't create buffer object from file. '%s' could not be opened\n", path);
        return NULL;
    }
    return bmo_dsp_bo_new(bo, flags, bo->channels, frames, bo->rate);
}


static int _bmo_dsp_fwrite_init(void *obj, uint32_t flags)
{
        (void)flags;
    BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *)obj;
    FILE * file = (FILE *)dsp->handle;
    bmo_fwrite_header_sun(file, dsp->flags, dsp->channels, 0xffffffff, dsp->rate);
    fflush(file);
    return 0;

};
static int _bmo_dsp_fwrite_update(void *obj, uint32_t frames)
{
    (void)frames;
    BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *)obj;
    FILE * file = (FILE *)dsp->handle;

    bmo_fwrite_mb(
        file,
        dsp->in_buffers,
        dsp->channels,
        bmo_fmt_enc(dsp->flags),
        dsp->frames,
        bmo_fmt_dither(dsp->flags)
    );
    bmo_mb_cpy(dsp->out_buffers, dsp->in_buffers, dsp->channels, dsp->frames);
    return 0;
};

static int _bmo_dsp_fwrite_close(void *obj, uint32_t flags){
    (void)flags;
    BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *)obj;
    FILE * file = dsp->handle;
    fflush(file);
    fclose(file);
    return 0;
}


BMO_dsp_obj_t * bmo_dsp_bo_new_fwrite(const char *path, uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate)
{
    FILE *file = fopen(path, "w");
    if(!file){
        bmo_err("couldn't open path:%s\n", strerror(errno));
        return NULL;
    }
    BMO_dsp_obj_t *dsp = bmo_dsp_new(flags, channels, frames, rate);
    if(!dsp){
        bmo_err("could not create DSP\n");
        return NULL;
    }
    dsp->_init = _bmo_dsp_fwrite_init;
    dsp->_update = _bmo_dsp_fwrite_update;
    dsp->_close = _bmo_dsp_fwrite_close;
    dsp->flags = flags;
    dsp->handle = file;
    return dsp;
}
