#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../definitions.h"
#include "../buffer.h"
#include "../dsp/simple.h"
#include "../error.h"

BMO_ringbuffer_t * bmo_init_rb(uint32_t frames, uint32_t channels)
{
	size_t tot_len = frames * 2;
	bmo_debug("bmo_init_rb(%d)\n", tot_len);
	BMO_ringbuffer_t * rb = NULL;
	
	// buffer length must be divisible by two or reads and writes may overlap
	if(frames % 2){
		bmo_err("frames needs to be even\n");
		return NULL;
	}
	
	if(!(rb = malloc(sizeof(BMO_ringbuffer_t)))){
		bmo_err("can't initialise BMO_ringbuffer_t: alloc fail\n");
		return NULL;
	}

	rb->data = bmo_mb_new(channels, tot_len);
	if(!(rb->data)){
		bmo_err("can't initialise BMO_ringbuffer_t: alloc fail\n");
		free(rb);
		return NULL;
	}
//	for(int i = 0; i < channels; i++)
//	    for(int j=0;j< tot_len; j++)
//	    {
//	        rb->data[i][j] = ((float)random()) / (float)(RAND_MAX);
//	    
//	    }
	//FIXME read and write index should be 0; read max el should be zero 
	//(buffer hasn't yet been filled, and write_max_el should be 
	rb->channels = channels;
	rb->frames = tot_len;
	rb->read_index = 0;
	rb->write_index = 0;
	rb->write_max_el = tot_len;
	rb->read_max_el= 0;

	return rb;
}


void bmo_rb_free(BMO_ringbuffer_t * rb)
{
	bmo_mb_free(rb->data, rb->channels);
	rb->data = NULL;
	free(rb);
} 


uint32_t bmo_write_rb(BMO_ringbuffer_t * rb, float ** in_buf, uint32_t frames)
{
	if( !rb || !in_buf )
		return -1;

	uint32_t to_write = 0;
	uint32_t i;
	uint32_t write;
	uint32_t written = 0;
	to_write = frames;
//	bmo_info("will write %d frames into %p\n", to_write, in_buf);
	if (to_write > rb->write_max_el){
	    // prevent overwriting data that hasn't been consumed.
		to_write = rb->write_max_el;
	}
	while(to_write){
		write = (rb->write_index + to_write < rb->frames) ? to_write : rb->frames - rb->write_index;
		for(i = 0; i < rb->channels; i++){
//		    bmo_debug("writing into channel %d at %p\n", i, rb->data[i]);
			bmo_sbcpy((rb->data[i]) + rb->write_index, in_buf[i] + written, write);
		}
		to_write -= write;
		written += write;
		rb->write_index += written;
		if(rb->write_index >= rb->frames){	
		    // once at the end return to start. this makes the buffer a ring.
			rb->write_index = 0;
		}
	}
	rb->read_max_el += written;
	rb->write_max_el -= written;
	return written;
}


uint32_t bmo_read_rb(BMO_ringbuffer_t * rb, float ** out_buf, uint32_t frames)
{
	if( !rb || !out_buf )
		return -1;
	uint32_t frames_left = 0;
	uint32_t i;
	uint32_t to_read = 0;
	uint32_t read = 0;

	frames_left = frames;
	if (frames_left > rb->read_max_el){
	    // prevent overwriting data that hasn't been consumed.		
		frames_left = rb->read_max_el;
    }
    
	while(frames_left){
		to_read = (rb->read_index + frames_left < rb->frames) ? frames_left : rb->frames - rb->read_index;
		for(i = 0; i < rb->channels; i++){
//		    bmo_err("reading samples!\n");
			bmo_sbcpy(out_buf[i] + read, rb->data[i] + rb->read_index, to_read);
		}
		frames_left -= to_read;
		rb->read_index += to_read;
		read += to_read;
		if(rb->read_index >= rb->frames){
			// once at the end return to start. this makes the buffer a ring.
			rb->read_index = 0;
		}
	}
    rb->read_max_el -= read; //FIXME READ MAX EL MUST BE UPDATED WITH EACH CALL.
    rb->write_max_el += read;
	
	return read;
}
