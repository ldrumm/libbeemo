#include "definitions.h"
#include "multiplexers.h"
#include "memory/map.h"
#include "dsp/simple.h"
#include "error.h"
#include "buffer.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

float **
bmo_mb_new(uint32_t channels, uint32_t frames)
{
    uint32_t i;
    float ** buffer = calloc(channels + 1, sizeof(float *));
    if(!buffer)
        return NULL;

    for (i = 0; i < channels; i++){
        buffer[i] = calloc(sizeof(float), frames);
        if(!buffer[i])
            goto fail;
    }
    return buffer;
fail:
    while(i--)
        free(buffer[i]);
    free(buffer);
    return NULL;
}

void bmo_mb_free( float ** buffer, uint32_t channels)
{
    for(uint32_t ch = 0; ch < channels; ch++){
        free(buffer[ch]);
        buffer[ch] = NULL;
    }
    free(buffer);
}

static int
_bmo_seek_bo(void * bo, long offset, int whence)
{
    whence = whence ? whence : SEEK_SET;
    BMO_buffer_obj_t * obj = (BMO_buffer_obj_t *) bo;
    size_t index;
    switch(whence){
        case SEEK_SET:{
            assert(offset >= 0);
            index = offset;
            break;
        }
        case SEEK_CUR:{
            assert(offset + obj->index < obj->frames);
            index = obj->index + offset;
            break;
        }
        case SEEK_END:
        {
            assert(offset < 0);
            index = obj->frames - offset;
            break;
        }
        default:{
            bmo_err("invalid seek value for 'whence' parameter:%d", whence);
            assert(0);
            return -1;
        }
    }
    if(index <= obj->frames){
        obj->index = index;
        return 0;
    }
    else {
        obj->index = obj->frames;
        return -1;
    }
}

static int
_bmo_get_bo_mapped_ipcm(void * bo, float ** dest, uint32_t frames)
{
    BMO_buffer_obj_t * obj = (BMO_buffer_obj_t *) bo;
    assert(dest && obj && frames) ;
    uint32_t channels = ((BMO_buffer_obj_t *)obj)->channels;
    size_t available = obj->frames - obj->index;

    char z_idx = 0;
    if(available < frames)        //    zero fill the end of the output buffer if the input is not long enough;
    {
        if(!obj->loop){
            for(uint32_t ch = 0; ch < channels; ch++)
                bmo_zero_sb(dest[ch] + available, frames - available);
            z_idx = 1;
        }
        else {
            bmo_conv_ibpcmtomb(
                dest,
                obj->buffer.interleaved_audio,
                channels,
                available,
                obj->encoding
            );
            bmo_mb_cpy_offset(dest, dest, 0, frames - available, channels, available);
            obj->index = 0;
        }
        frames = available;
    }
    bmo_conv_ibpcmtomb(
        dest,
        obj->buffer.interleaved_audio + obj->index * (bmo_fmt_stride(obj->encoding) * channels),
        channels,
        frames,
        obj->encoding
    );

    if(z_idx){
        obj->index = 0;
    }
    else{
        obj->index += frames;
    }
    return 0;
}


static int
_bmo_get_bo_mapped_if(void * bo, float ** dest, uint32_t frames)
{
    BMO_buffer_obj_t * obj = (BMO_buffer_obj_t *) bo;
    uint32_t channels = ((BMO_buffer_obj_t *)obj)->channels;

    assert(dest && obj && frames);
    uint32_t available = obj->frames - obj->index;
    char z_idx = 0;
    if(available < frames){
        //    zero fill the end of the output buffer if the input is not long enough;
        for(uint32_t ch = 0; ch < obj->channels; ch++)
            bmo_zero_sb(dest[ch] + available, frames - available);
        frames = available;
        z_idx = 1;
    }
    bmo_conv_ibftomb(
        dest,
        obj->buffer.interleaved_audio + obj->index * (bmo_fmt_stride(obj->encoding) * channels),
        channels,
        frames,
        obj->encoding
    );
    obj->index +=frames;
    if(z_idx)
        obj->index = 0;
    return 0;
}


static int
_bmo_get_bo_mb(void * bo, float ** dest, uint32_t frames)
{
    BMO_buffer_obj_t * obj = (BMO_buffer_obj_t *) bo;
    assert(dest && obj && frames);
    uint32_t channels = obj->channels;
    uint32_t available = obj->frames - obj->index;
    char z_idx = 0;
    if(available < frames){        //    zero fill the end of the output buffer if the input is not long enough;
        for(uint32_t ch = 0; ch < obj->channels; ch++)
            bmo_zero_sb(dest[ch] + available, frames - available);
        frames = available;
        z_idx = 1;
    }
    for(uint32_t ch = 0; ch < channels; ch++)
        bmo_sbcpy(dest[ch], obj->buffer.buffered_audio[ch] + obj->index, frames);
    obj->index +=frames;
    if(z_idx)
        obj->index = 0;
    return 0;
}


BMO_buffer_obj_t *
bmo_bo_new(uint32_t flags, uint32_t channels, size_t frames,  uint32_t rate, size_t offset, size_t file_len, void * data)
{
    uint32_t encoding = bmo_fmt_enc(flags);
    flags &= (BMO_BUFFERED_DATA | BMO_MAPPED_FILE_DATA | BMO_EXTERNAL_DATA);

//    assert(encoding);
    assert(flags);
//    if(!data)
//        return NULL;

    BMO_buffer_obj_t * obj = malloc(sizeof(BMO_buffer_obj_t));
    if(!obj)
        return NULL;

    obj->type = flags;
    obj->encoding = encoding;
    obj->channels = channels;
    obj->index = 0;
    obj->frames = frames;
    obj->rate = rate;
    obj->handle = data;
    obj->file_len = file_len;
    obj->is_alias = 0;
    obj->seek = NULL;
    obj->read = NULL;

    switch(flags)
    {
        case BMO_MAPPED_FILE_DATA:
        {
            obj->buffer.interleaved_audio = data + offset; //FIXME void * arithmetic
            if(bmo_fmt_pcm(encoding))                                //set the callbacks
                obj->read = _bmo_get_bo_mapped_ipcm;
            else
                obj->read = _bmo_get_bo_mapped_if;
            obj->seek = _bmo_seek_bo;
            return obj;
            break;    //never reached
        }
        case BMO_BUFFERED_DATA://alloc storage buffer, and copy data from file
        {
            /* allocate persistant buffers for storage of audio data*/
            obj->buffer.buffered_audio = bmo_mb_new(channels, frames);
            if(!obj->buffer.buffered_audio){
                free(obj);
                return NULL;
            }

            if(data)
            {    //only copy data when there is a source, otherwise return blank buffer object
                if(bmo_fmt_pcm(encoding)){
                    bmo_debug("demultiplexing PCM file and converting to float...\n");
                    bmo_conv_ibpcmtomb(
                        obj->buffer.buffered_audio,
                        (data + offset),
                        obj->channels,
                        obj->frames,
                        encoding
                    );
                }
                else
                {
                    bmo_debug("demultiplexing float file...\n\n");
                    bmo_conv_ibftomb(
                        obj->buffer.buffered_audio,
                        data + offset,
                        obj->channels,
                        obj->frames,
                        encoding);
                }
                bmo_unmap(data, file_len);
            }
            obj->handle = NULL;
            obj->read = _bmo_get_bo_mb;
            obj->seek = _bmo_seek_bo;
            break;
        }
        case BMO_EXTERNAL_DATA:
        {
            //hmmm...
            return obj;
        }
        default: assert("Unknown BMO_buffer_obj_t type" == NULL);
    }//switch(type)
    return obj;
}


BMO_buffer_obj_t *
bmo_bo_alias(BMO_buffer_obj_t * obj)
{
    BMO_buffer_obj_t * cpy = malloc(sizeof(BMO_buffer_obj_t));
    if(!cpy)
        return NULL;
    memcpy(cpy, obj, sizeof(BMO_buffer_obj_t));

    cpy->is_alias = 1;
    return cpy;
}


BMO_buffer_obj_t *
bmo_bo_cpy(BMO_buffer_obj_t * obj)
{
    BMO_buffer_obj_t * cpy = bmo_bo_alias(obj);
    if(!cpy)
        return NULL;

    cpy->index = 0;
    cpy->handle = NULL;

    switch(cpy->type)
    {
        case BMO_MAPPED_FILE_DATA:
        {
            break;
        }
        case BMO_BUFFERED_DATA:
        {
            float ** old = cpy->buffer.buffered_audio;
            cpy->buffer.buffered_audio = bmo_mb_new(cpy->channels, cpy->frames);
            for(uint32_t ch = 0; ch < cpy->channels; ch++)
                bmo_sbcpy(cpy->buffer.buffered_audio[ch], old[ch], cpy->frames);
            break;
        }
        case BMO_EXTERNAL_DATA:
        {
            bmo_err("can't duplicate buffer to external resource\n");
            bmo_bo_free(cpy);
            return NULL;
        }
        default: {
            assert(0);
            break;
        }
    }
    return cpy;
}


void
bmo_bo_free(BMO_buffer_obj_t * obj)
{
    if(!obj)
        return;

    obj->frames = 0;
    obj->channels = 0;
    obj->rate = 0;
    obj->read = NULL;

    if(obj->is_alias){    //don't free resources of is_alias objects
        //FIXME
        free(obj);
        return;
    }

    obj->read = NULL;
    /*free all audio data*/
    if(obj->type == BMO_MAPPED_FILE_DATA){
        bmo_unmap(obj->handle, obj->file_len);
        obj->handle = NULL;
        obj->buffer.interleaved_audio = NULL;
    }
    else if(obj->type == BMO_BUFFERED_DATA){
        if(obj->buffer.buffered_audio){
            bmo_mb_free(obj->buffer.buffered_audio, obj->channels);
            obj->buffer.buffered_audio = NULL;
        }
    }
    else {
        assert(0);
    }
    free(obj);
    return;
}
