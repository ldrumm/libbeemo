#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "buffer.h"
#include "import_export.h"
#include "drivers/driver_utils.h"
#include "drivers/ringbuffer.h"
#include "dsp/simple.h"
#include "error.h"
#include "definitions.h"
#include "dsp_obj.h"
#include "util.h"

static int _bmo_dsp_init(void * dsp, uint32_t flags)
{
    //Well apparently it does nothing...
    (void)dsp;
    (void)flags;
    return 0;
}
static int _bmo_dsp_update(void * obj, uint32_t flags)
{
    //just copy input buffers to output.
    (void)flags;
    BMO_dsp_obj_t * dsp = obj;
    bmo_mb_cpy(dsp->out_buffers, dsp->in_buffers, dsp->channels, dsp->frames);
    return 0;
}
static int _bmo_dsp_close(void * dsp, uint32_t flags)
{
    //Well apparently it does nothing...
    (void)flags;
    bmo_dsp_close(dsp);
    return 0;
}


BMO_dsp_obj_t *
bmo_dsp_new(uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate)
{
	BMO_dsp_obj_t * dsp = malloc(sizeof(BMO_dsp_obj_t));
	if(!dsp){
	    bmo_err("couldn't create dsp object:%s\n", strerror(errno));
		return NULL;
	}
	dsp->frames = frames;
	dsp->id = bmo_uid();
	dsp->type = flags; 			//FIXME define dsp-object flags
	dsp->channels = channels;	
	dsp->rate = rate;			
	dsp->tick = 0;
	dsp->in_buffers = bmo_mb_new(channels, frames);
	dsp->out_buffers = bmo_mb_new(channels, frames);
	dsp->ctl_buffers = bmo_mb_new(channels, frames);
	if((!dsp->ctl_buffers) || (!dsp->out_buffers) || (!dsp->in_buffers)){
		free(dsp->ctl_buffers);
		free(dsp->out_buffers);
		free(dsp->in_buffers);
		free(dsp);
		return NULL;
	}
	dsp->handle = NULL;
	dsp->_init = _bmo_dsp_init;
	dsp->_update = _bmo_dsp_update;
	dsp->_close = _bmo_dsp_close;
	dsp->in_ports = NULL;
	dsp->ctl_ports = NULL;
/*	dsp->update_obj_buffer = NULL;*/
	
	return dsp;
}

void
bmo_dsp_close(BMO_dsp_obj_t * dsp)
{
    uint32_t channels = dsp->channels;
    bmo_mb_free(dsp->out_buffers, channels);
    bmo_mb_free(dsp->in_buffers, channels);
    bmo_mb_free(dsp->ctl_buffers, channels);
    BMO_ll_t * ll = dsp->in_ports;
    BMO_ll_t * next;
    while(ll){
        next = ll->next;
        free(ll);
        ll = next;
    }
}

static int
_bmo_dsp_rb_init(void * obj, uint32_t flags)
{
    (void)obj;
    (void)flags;
    return 0;
}

static int
_bmo_dsp_rb_update(void * obj, uint32_t flags)
{
    (void)flags;
    BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *)obj;
    if(dsp->flags & BMO_DSP_TYPE_OUTPUT){
        bmo_write_rb(dsp->handle, dsp->in_buffers, dsp->frames);
    }
    else if(dsp->flags & BMO_DSP_TYPE_INPUT){
        bmo_zero_mb(dsp->out_buffers, dsp->channels, dsp->frames);
        bmo_read_rb(dsp->handle, dsp->out_buffers, dsp->frames);
    }
    else{
        assert(0 && "Ringbuffer dsp object must be declared as input or output"); //Bad type declarations
    }
    //TODO error handling in rb_*()calls
    return 0;
}

static int
_bmo_dsp_rb_close(void * obj, uint32_t flags)
{
    (void)obj;
    (void)flags;
    bmo_dsp_close(obj);
    return 0; //TODO dsp_*_close() functions should be declared void not int.  free(3) is void. bmo_dsp_close() is void.
}

BMO_dsp_obj_t *
bmo_dsp_rb_new(void * ringbuffer, uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate)
{
    /** create a new DSP object with the given ringbuffer as the backend.
    When (flags & BMO_DSP_TYPE_OUTPUT), input will be copied to output as well as written into the ringbuffer.
    This allows graphs like the following:
        |
    [some DSP]
        |
   [ringbuffer1]--->[DAC]
        |
    [some DSP]
        |
 [some other DSP]
        |
   [ringbuffer2]--->[Disk Writer]

    When (flags & BMO_DSP_TYPE_INPUT) input will ignored and rb will be copied to output.
    */
    BMO_dsp_obj_t * dsp = bmo_dsp_new(BMO_DSP_OBJ_RB, channels, frames, rate);
    assert(flags & (BMO_DSP_TYPE_INPUT | BMO_DSP_TYPE_OUTPUT));
    if(!dsp){
        return NULL;
    }
    dsp->handle = ringbuffer;
    dsp->type = BMO_DSP_OBJ_RB;
    dsp->flags = flags;
    dsp->_init = _bmo_dsp_rb_init;
    dsp->_update = _bmo_dsp_rb_update;
    dsp->_close = _bmo_dsp_rb_close;

    return dsp;
}

static int
_bmo_dsp_bo_init(void * obj, uint32_t flags)
{
    //But existing is basically all I do! (dsp->_init can't be NULL)
    (void)flags;(void)obj;
    return 0;
}

static int
_bmo_dsp_bo_update(void * obj, uint32_t flags)
{
    (void)flags;
    assert(obj);
    BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *)obj;
    BMO_buffer_obj_t * bo = dsp->handle;
    return bo->read(bo, dsp->out_buffers, dsp->frames);
}

static int
_bmo_dsp_bo_close(void * obj, uint32_t flags)
{
    (void)flags;
    assert(obj);
    BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *)obj;
    if(!dsp){
        return -1;
    }
    bmo_bo_free(dsp->handle);
    bmo_dsp_close(dsp);
    return 0;
}

BMO_dsp_obj_t *
bmo_dsp_bo_new(void * bo, uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate)
{
    BMO_dsp_obj_t * dsp = bmo_dsp_new(BMO_DSP_OBJ_BO|flags, channels, frames, rate);
    if(!dsp)
        return NULL;

    dsp->handle = bo;
    dsp->_init = _bmo_dsp_bo_init;
    dsp->_update = _bmo_dsp_bo_update;
    dsp->_close = _bmo_dsp_bo_close;
    dsp->type = BMO_DSP_OBJ_BO;
    dsp->flags = flags; ///FIXME This is already set in bmo_dsp_new()  but that behaviour may change.
    return dsp;
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

