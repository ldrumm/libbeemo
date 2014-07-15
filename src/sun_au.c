#include "definitions.h"
#include "sun_au.h"
#include "multiplexers.h"
#include "buffer.h"
#include "memory/map.h"
#include "import_export.h"
#include "error.h"
#include <stdint.h>
#include <assert.h>

BMO_buffer_obj_t * 
bmo_fopen_sun(const char * path, uint32_t flags)
{
	BMO_au_header_t header;
	uint32_t le = 0;
	void * data = NULL;
	uint32_t * hp = NULL;			//au header pointer
	size_t size = 0;
	int err;
	size = bmo_fsize(path, &err);
	if(!size || err ){
	    bmo_err("%s does not exist or is empty\n", path);
	    return NULL;
	}
	if(size < 24){
	    bmo_err("%s's header is too short\n", path);
		return NULL;
	}
	data = bmo_map(path, 0, 0); //FIXME flags
	if(!data){
	    bmo_err("couldn't map file:%s\n", path);
	    return NULL;
	}
	hp = (uint32_t *) data;
	/*load info from header*/
	header.au_magic_number = hp[0];
	header.au_data_offset = hp[1];
	header.au_data_size = hp[2];
	header.au_data_encoding = hp[3];
	header.au_data_sample_rate = hp[4];
	header.au_data_channels = hp[5];
	header.au_metadata = hp[6];
	if(bmo_host_le())
		_bmo_swap_32(&header.au_magic_number);
	/*Sanity check header*/
	if (header.au_magic_number == 0x2e736e64)	//FIXME support loading le
	{	
		le = 0;
		if(bmo_host_le())
		{
			_bmo_swap_32(&header.au_data_offset);
			_bmo_swap_32(&header.au_data_size);
			_bmo_swap_32(&header.au_data_encoding);
			_bmo_swap_32(&header.au_data_sample_rate);
			_bmo_swap_32(&header.au_data_channels);
		//	_bmo_swap_32(&header.au_metadata);
		}	
	}	
	else if(header.au_magic_number == 0x646e732e)	//"dns."
	{
		le = 1;
		if(bmo_host_be())
		{
			_bmo_swap_32(&header.au_data_offset);
			_bmo_swap_32(&header.au_data_size);
			_bmo_swap_32(&header.au_data_encoding);
			_bmo_swap_32(&header.au_data_sample_rate);
			_bmo_swap_32(&header.au_data_channels);
		//	_bmo_swap_32(&header.au_metadata);
		}		
	}
	else
	{
		bmo_err("%s is not a valid AU file\n", path);
		bmo_debug("0x%8x\n", header.au_magic_number);
		return NULL;
	}
	
	if (header.au_data_offset <= 28){
		bmo_debug("WARNING: %s has non-standard data offset of %d \n", path, header.au_data_offset);
	}
	if (!header.au_data_size){
		bmo_err("%s has no audio data\n", path);
		return NULL;
	}
	if(header.au_data_offset >= header.au_data_size){	//check possible overflow
		bmo_info("Badly crafted header\n");
	}
	if(header.au_data_size == 0xffffffff)
		header.au_data_size = size - header.au_data_offset;

	if (!header.au_data_channels){
		bmo_err("%s has no audio channels \n", path);
		return NULL;
	}

	/*test the data encoding of the file, and configure the conversion */	
	switch(header.au_data_encoding)
	{
		case AU_FORMAT_8_BIT_PCM: 	flags |= BMO_FMT_PCM_8 ;   break;		
		case AU_FORMAT_16_BIT_PCM: 	flags |= ((le == 0) ? BMO_FMT_PCM_16_BE   : BMO_FMT_PCM_16_LE);   break;
		case AU_FORMAT_24_BIT_PCM: 	flags |= ((le == 0) ? BMO_FMT_PCM_24_BE   : BMO_FMT_PCM_24_LE);   break;
		case AU_FORMAT_32_BIT_PCM: 	flags |= ((le == 0) ? BMO_FMT_PCM_32_BE   : BMO_FMT_PCM_32_LE);   break;
		case AU_FORMAT_32_BIT_FLOAT:flags |= ((le == 0) ? BMO_FMT_FLOAT_32_BE : BMO_FMT_FLOAT_32_LE); break;
		case AU_FORMAT_64_BIT_FLOAT:flags |= ((le == 0) ? BMO_FMT_FLOAT_64BE : BMO_FMT_FLOAT_64_LE); break;
		default:
		{
			bmo_err(" au encoding of type %d not supported\n", header.au_data_encoding);
			bmo_unmap(data, size);
			return NULL;
		}
	}
   	return bmo_bo_new(
   		flags, 
   		header.au_data_channels,
   		(header.au_data_size / bmo_fmt_stride(flags) / header.au_data_channels), 
   		header.au_data_sample_rate, 
   		header.au_data_offset, 
   		size, 
   		data
   	);

}//bmo_fopen_sun


static int _bmo_fwrite_header_sun(uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate, FILE * file)
{
//	uint32_t le = 0;
	uint32_t encoding = bmo_fmt_enc(flags);
	BMO_au_header_t header = {0,0,0,0,0,0,0};
	
	header.au_magic_number = 0x2e736e64;
	header.au_data_offset = 28;
	header.au_data_size = bmo_fmt_stride(flags) * frames * channels;
	header.au_data_sample_rate = rate;
	header.au_data_channels = channels;
	header.au_metadata = 0;							//we're not interested in metadata at all when writing
	
	switch(encoding)
	{
		case BMO_FMT_PCM_U8:
		{
			bmo_info("Warning: AU files do not support unsigned data. Writing 8bit signed\n");
			header.au_data_encoding = AU_FORMAT_8_BIT_PCM;
//			le = 1;
		}	
		case BMO_FMT_PCM_8:	header.au_data_encoding = AU_FORMAT_8_BIT_PCM; 	  break;
		case BMO_FMT_PCM_16_LE:	header.au_data_encoding = AU_FORMAT_16_BIT_PCM;   break;
		case BMO_FMT_PCM_24_LE:	header.au_data_encoding = AU_FORMAT_24_BIT_PCM;   break;
		case BMO_FMT_PCM_32_LE:	header.au_data_encoding = AU_FORMAT_32_BIT_PCM;   break;
		case BMO_FMT_FLOAT_32_LE:header.au_data_encoding = AU_FORMAT_32_BIT_FLOAT; break;
		case BMO_FMT_FLOAT_64_LE:header.au_data_encoding = AU_FORMAT_64_BIT_FLOAT; break;
		case BMO_FMT_PCM_16_BE:	header.au_data_encoding = AU_FORMAT_16_BIT_PCM;   break;
		case BMO_FMT_PCM_24_BE:	header.au_data_encoding = AU_FORMAT_24_BIT_PCM;   break;
		case BMO_FMT_PCM_32_BE:	header.au_data_encoding = AU_FORMAT_32_BIT_PCM;	  break;
		case BMO_FMT_FLOAT_32_BE:header.au_data_encoding = AU_FORMAT_32_BIT_FLOAT; break;
		case BMO_FMT_FLOAT_64BE:header.au_data_encoding = AU_FORMAT_64_BIT_FLOAT; break;
		default:
		{
			bmo_err("Unsupported Sun/AU format type:%x\n", encoding);
			assert(0); //TODO
		} 	
	}

	if(bmo_fmt_end(encoding) != bmo_host_endianness())
		_bmo_swap_ib(&header, 4, 7);
	
	fwrite(&header.au_magic_number, 4, 1, file); 	//7 write calls in case structure is not packed
	fwrite(&header.au_data_offset, 4, 1, file);
	fwrite(&header.au_data_size, 4, 1, file);
	fwrite(&header.au_data_encoding, 4, 1, file); 
	fwrite(&header.au_data_sample_rate, 4, 1, file);
	fwrite(&header.au_data_channels, 4, 1, file);
	fwrite(&header.au_metadata, 4, 1, file);
	
	return 0;
}	//_bmo_fwrite_header_sunBE


size_t bmo_fwrite_sun(BMO_buffer_obj_t * buffer, const char * path, uint32_t flags)
{
	struct stat statbuf;
	uint32_t encoding = bmo_fmt_enc(flags);
	size_t written;
	FILE * file = NULL;
	if(!buffer)
	{
		bmo_err( "passed NULL pointer - no AU file written\n");
		return 0;
	}

	/* check and open file for writing */
	if ((stat(path, &statbuf)) != -1)
	{
		bmo_err( "%s exists, data not written\n", path);
		return 0;
	}

	file = fopen(path, "wb");
	if(!file)
	{
		bmo_err( "file open failure\n");
		return 0;
	}

	_bmo_fwrite_header_sun(flags, buffer->channels, buffer->frames, buffer->rate, file);
	if(buffer->type == BMO_MAPPED_FILE_DATA)
	{
		written = bmo_fwrite_ib(
			file, 
			buffer->buffer.interleaved_audio, 
			buffer->channels, 
			encoding, 
			buffer->encoding,
			buffer->frames, 
			bmo_fmt_dither(flags)
		);
	}
	else if (buffer->type == BMO_BUFFERED_DATA)
	{
		written = bmo_fwrite_mb(
			file, 
			buffer->buffer.buffered_audio, 
			buffer->channels, 
			encoding, 
			buffer->frames, 
			bmo_fmt_dither(flags)
		);
	}
	else{
		assert("bmo_fwrite_sun() called with unknown flag" == NULL);
		fflush(file);
		fclose(file);
		return 0;
	}
	fflush(file);
	fclose(file);
	return written;
} //bmo_fwrite_sun
