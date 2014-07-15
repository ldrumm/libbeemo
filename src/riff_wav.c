#include <stdint.h>

#include "definitions.h"
#include "riff_wav.h"
#include "multiplexers.h"
#include "buffer.h"
#include "memory/map.h"
#include "import_export.h"
#include "error.h"

BMO_buffer_obj_t * bmo_fopen_wav(const char * path, uint32_t flags)
{
/*wav header reader for AUD library*/
/*gets information from the wav header and returns a structure with floating point audio data etc*/

/*based on original wav header info from Wed 26 Dec 2007 03:03:21 GMT by me, Luke Drummond*/
/*rewritten for AUD Fri 02 Sep 2011 02:28:58 BST */
/*TODO:recognise the following subchunk ids and do something useful - probably ignore them:
	'bext' chunk found in files recorded in protools. seems to come directly after WAVE, and contain some sort of program metadata
	'minf' again found in protools files as part of the 'fmt ' chunk content 16 bytes and purpose unknown
	'elm1' subchunk of 'fmt ':- contained 7kiB	of null bytes. go protools.
*/
	BMO_wav_header header;
	void * data = NULL;
	char * hp = NULL;	//wav header pointer
	uint32_t temp;
//	uint32_t samples = 0; //warning: variable ‘samples’ set but not used [-Wunused-but-set-variable]
	size_t size = 0;
	uint16_t temp16;
//	int tofloat;	//TODO warning: variable ‘samples’ set but not used [-Wunused-but-set-variable]
	int fact = 0;
	int be = bmo_host_be();
	int swap = 0;
	int rifx = 0;
	int err;
/* allocate storage */
	size = bmo_fsize(path, &err);
	if((!size) || err){
	    bmo_err("%s does not exist or empty", path);
	}
	data = bmo_map(path, 0, 0); //FIXME flags

/* copies bytes 0-4, tests for "RIFF", quits on failure */
	hp = (char *) data;
	header.chunk_id = (*(uint32_t *)hp);
	
	if(header.chunk_id == 0x46464952)		{	//'RIFF' in little endian decoding
		rifx = 0;	
		if(be)
			swap = 1;
		bmo_debug("%s is a RIFF\n", path);
	}
	else if(header.chunk_id == 0x58464952)	{	//'RIFX' denotes a big endian file
		rifx = 1;	
		if(!be)
			swap = 1;
		bmo_err("RIFF chunk not verified: value is %8x - dodgy file?\n",header.chunk_id);
		return NULL;
	}
	else {
		bmo_err("RIFF chunk not verified: value is %8x - dodgy file?\n",header.chunk_id);
		return NULL;
	}

/* copies bytes 4-7, records chunk_size quits on failure */
	hp += 4;
	header.chunk_size = (*(uint32_t *)hp);
	if(swap)
		_bmo_swap_32(&(header.chunk_size));

/* copies bytes 8-11, tests for "WAVE", quits on failure */
	hp += 4;
	header.format = (*(uint32_t *)hp);
	if(header.format != 0x45564157){	
	    //'WAVE' in little endian decoding 0x57415645
		bmo_err("'WAVE' chunk not found - dodgy file?\n");
		return NULL;
	}
	hp += 4;
	
/*skip bext chunk if it exists */
	if((*(uint32_t *) hp) == 0x74786562||(*(uint32_t *) hp) == 0x74786562){
	    //bext = 0x74786562
		bmo_debug("bwf BEXT chunk found, ignoring...\n");
		hp+=4;
		temp = (*(uint32_t *) hp);
		if(swap)
			_bmo_swap_32(&(temp));
		hp+=4;
		hp+=temp;
	}

/* copies bytes 12-15, tests for "fmt ", quits on failure */
	header.subchunk_1_id = (*(uint32_t *)hp);
	if(header.subchunk_1_id  != 0x20746d66){
		bmo_err("subchunk_1_id not verified error - dodgy file?\n");
		return NULL;
	}

/* copies bytes 16-19, tests for the size of subhcunk 1, quits on failure */
	hp += 4;
	header.subchunk_1_size = (*(uint32_t *)hp);
	if(header.subchunk_1_size  == 16) {	
    	bmo_debug(" subchunk_1_size %d\n", header.subchunk_1_size);
	}

/* tests file encoding ( 20-23) */
	hp += 4;
	header.format_no = (*(uint16_t *)hp);
	
	if(swap)
		_bmo_swap_16(&(header.format_no));
	
//	switch(header.format_no)
//	{
//	case WAVE_FORMAT_PCM:
//		tofloat = 1;
//		
//			bmo_debug("Linear PCM\n");
//		break;
//	case WAVE_FORMAT_IEEE_FLOAT:
//		fact = 1;
//		tofloat = 0;
//		
//			bmo_debug("IEEE float\n");
//		break;
//	case WAVE_FORMAT_EXTENSIBLE:
//		
//			bmo_debug("extensible wav file - checking extra chunks...\n");
//			bmo_debug("extensible wav parsing not yet implemented...\n");
//			return NULL;
//			break;
//	
//	default:
//		bmo_info(stderr,  "%d is an unrecognised wavformat code\n",header.format_no);
//		bmo_info(stderr, "ERROR:bad format number:%d\n", header.format_no);
//		bmo_err("couldn't read wave file %s\n", path);
//		return NULL;
//	}


/* get number of channels: 1 for mono, 2 for stereo etc. 24-25*/
	hp += 2;
	header.num_channels = (*(uint16_t *)hp);
	if(swap)
		_bmo_swap_16(&(header.num_channels));
	
	bmo_debug("%d channels:\n", header.num_channels);

/* get sample rate 26-29 */
	hp += 2;
	header.sample_rate = (*(uint32_t *)hp);
	if(swap)
		_bmo_swap_32(&(header.sample_rate));
	
	bmo_debug("sample rate %d Hz:\n", header.sample_rate);

/* get average byte rate of sample data 30-33*/
	hp += 4;
	header.byte_rate = (*(uint32_t *) hp);
	if(swap)
		_bmo_swap_32(&(header.byte_rate));
	
	bmo_debug("Byte Rate of audio stream:%d\n", header.byte_rate);

/* get block align (number of bytes per sample across all channels 34-35 */
	hp += 4;
	header.block_align = (*(uint16_t *) hp);
	if(swap)
		_bmo_swap_16(&(header.block_align));
	
	bmo_debug("Block Align (bytes per sample across all channels): %d\n", header.block_align);

/* get the bit depth of the samples 36-37*/
	hp += 2;
	header.bits_per_sample = (*(uint16_t *) hp);
	if(swap)
		_bmo_swap_16(&(header.bits_per_sample));
	
	bmo_debug("Resolution:%d bits per sample\n", header.bits_per_sample);

	if(header.subchunk_1_size != 16){
		hp+=2;
		temp16 = (*(uint16_t *) hp);
		_bmo_swap_16(&(temp16));
		hp+=temp16;
	}

	if(fact){
		hp+=2;
		if( (*(uint32_t *)hp) == 0x74636166 || (*(uint32_t *)hp) == 0x66616374){
			hp+=4;
			header.extra_param_size = (*(uint32_t *)hp);
			if(swap)
			_bmo_swap_32(&(header.extra_param_size));
			
				bmo_debug("extra parameters in 'fact' chunk occupy %d bytes\n", header.extra_param_size);
			hp+=4;
			{
				//TODO : add parsing code for extra parameters here
			}
			hp+=header.extra_param_size;
		}
		else {
			bmo_err("fact chunk not recognised:0x%x\n",(*(uint32_t *)hp));
			return NULL;
		}
	}
	else{
		hp+=2;
	}
	
	/*skip list chunk if it exists */
	if((*(uint32_t *) hp) == 0x4c495354 || (*(uint32_t *) hp) == 0x5453494c){
		//LIST = 0x4c495354
		bmo_info("LIST chunk found  in %s; ignoring...\n", path);
		hp+=4;
		temp = (*(uint32_t *) hp);
		if(swap)
			_bmo_swap_32(&(temp));
		hp+=4;
		hp+=temp;
	}
	
	/*skip peak chunk if it exists */
	if((*(uint32_t *) hp) == 0x5045414b || (*(uint32_t *) hp) == 0x4b414550){
	    //PEAK = 0x5045414b
		bmo_info("PEAK chunk found in %s; ignoring...\n", path);
		hp+=4;
		temp = (*(uint32_t *) hp);
		if(swap)
			_bmo_swap_32(&(temp));
		hp+=4;	
		hp+=temp;
	}

	/*skip minf chunk if it exists */
	if((*(uint32_t *) hp) == 0x666e696d || (*(uint32_t *) hp) == 0x6d696e66){
			//minf == 0x666e696d
		bmo_info("minf chunk found in %s; ignoring...\n", path);
		hp+=4;
		temp = (*(uint32_t *) hp);
		if(swap)
			_bmo_swap_32(&(temp));
		hp+=4;	
		hp+=temp;
	}
	
	/*skip elm1 chunk if it exists */
	if((*(uint32_t *) hp) == 0x656c6d31 || (*(uint32_t *) hp) == 0x316d6c65){
		//0x656c6d31 == 'elm1'
		bmo_info("elm1 chunk found in %s ;ignoring...\n" );
		hp+=4;
		temp = (*(uint32_t *) hp);
		if(swap)
			_bmo_swap_32(&(temp));
		hp+=4;	
		hp+=temp;
	}
	
/* get subchunk_2_id */
	header.subchunk_2_id = (*(uint32_t *) hp);
	if(swap)
		_bmo_swap_32(&(header.subchunk_2_id));
	if(header.subchunk_2_id == 0x61746164) {	//'data' in little endian ASCII
		
			bmo_debug("subchunk_2_id: data\n");
	}
	else{	
		bmo_err("wav open error:subchunk 2 id not verified: 0x%x\n", header.subchunk_2_id);
		//FIXME cleanup before return otherwise memory leak
		return NULL;
	}
	
/* get subchunk_2_size */
	hp += 4;
	header.subchunk_2_size = (*(uint32_t *) hp);
	if(swap)
		_bmo_swap_32(&(header.subchunk_2_size));
	
	bmo_debug("subchunk_2_size (total size in bytes of audio data) : %d\n", header.subchunk_2_size);
	
/*do the actual copying of audio data and set up the audiotrack struct with relevant information */	
	hp+=4;
//	samples = header.subchunk_2_size / (header.bits_per_sample / 8);
	if(header.subchunk_2_size > size){
		bmo_err("AUD ERROR: wav file malformed. requested read longer than file.");
		//FIXME cleanup before return otherwise memory leak
		return NULL;
	}
	
	switch(header.format_no)
	{
		case WAVE_FORMAT_PCM: 
		{	
			if(rifx)
				switch( header.bits_per_sample)
				{
					case 8: flags |= BMO_FMT_PCM_U8; break;
					case 16:flags |= BMO_FMT_PCM_16_BE; break;
					case 24:flags |= BMO_FMT_PCM_24_BE; break;
					case 32:flags |= BMO_FMT_PCM_32_BE; break;
					default: return NULL;
				}
			else
				switch(header.bits_per_sample)
				{
					case 8: flags |= BMO_FMT_PCM_U8; break;
					case 16:flags |= BMO_FMT_PCM_16_LE; break;
					case 24:flags |= BMO_FMT_PCM_24_LE; break;
					case 32:flags |= BMO_FMT_PCM_32_LE; break;
					default: return NULL;
				}
				break;
		}
		case WAVE_FORMAT_IEEE_FLOAT:
		{
			if(rifx)
				switch(header.bits_per_sample)
				{
					case 32:flags |= BMO_FMT_FLOAT_32_BE; break;
					case 64:flags |= BMO_FMT_FLOAT_64BE; break;	
					default: return NULL;
				}
			else
				switch(header.bits_per_sample)
				{
					case 32:flags |= BMO_FMT_FLOAT_32_LE; break;
					case 64:flags |= BMO_FMT_FLOAT_64_LE; break;	
					default: return NULL;
				}
			break;
		}
	}
   	return bmo_bo_new(flags, 
   	    header.num_channels, 
   	    (header.subchunk_2_size / bmo_fmt_stride(flags) / header.num_channels), 
   	    header.sample_rate, 
   	    0, 
   	    size, 
   	    (void *)hp
   	);
}	/*bmo_fopen_wav*/


//static char * BMO_gen_wav_header(int samples, int rate, int channels, uint32_t fmt)
//{
//	int i = 0;
//	int resolution;
//	int pcm = 0;
//	int floating  = 0;
//	int datalength;
//	BMO_wav_header header;
//	
//	switch (fmt){
//		case WAV_FORMAT_8_BIT_PCM 	: resolution = 8; pcm = 1;break;
//		case WAV_FORMAT_16_BIT_PCM	: resolution = 16; pcm = 1;break;
//		case WAV_FORMAT_24_BIT_PCM	: resolution = 24; pcm = 1;break;
//		case WAV_FORMAT_32_BIT_PCM	: resolution = 32; pcm = 1;break;
//		case WAV_FORMAT_32_BIT_FLOAT: resolution = 32; floating = 1;break;
//		case WAV_FORMAT_64_BIT_FLOAT: resolution = 64; floating = 1;break;
//		default:assert(0); //user should not be calling this function with any other fmt
//	}
//	
//	(void)floating; //FIXME conform to wave format extensible in case of floating point  
//	
//	datalength = samples * resolution;

//	header.chunk_id = 0x46464952;					// "RIFF"
//	header.chunk_size = (uint32_t)samples - 8;
//	header.format = 0x45564157;						// "WAVE"
//	header.subchunk_1_id = 0x20746d66; 				// "fmt "
//	header.subchunk_1_size = 16;					(pcm == 1) ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;// this should be
//	header.format_no = (uint16_t) (pcm == 1) ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
//	header.num_channels = (uint16_t) channels;		
//	header.sample_rate = (uint32_t) rate;
//	header.byte_rate = (uint32_t) (rate * channels * resolution) / 8;
//	header.block_align = (uint16_t) channels * resolution / 8;
//	header.bits_per_sample = (uint16_t) resolution;	// resolution
//	header.subchunk_2_id = 0x61746164;				// "data"
//	header.subchunk_2_size = (uint32_t)datalength;

//	char * data = NULL;
//	uint32_t * value32 = (uint32_t *) data;
//	uint16_t * value16 = NULL;
//	
//	data = malloc(44);
//	if(!data)
//		return NULL;
//	
//	value32 = (uint32_t *) &data[i];
//	*(value32) = header.chunk_id;
//	i+=4;
//	
//	value32 = (uint32_t *) &data[i];
//	*(value32) = header.chunk_size;
//	i+=4;
//	
//	value32 = (uint32_t *) &data[8];	
//	*(value32) = header.format;
//	i+=4;

//	value32 = (uint32_t *) &data[12];
//	*(value32) = header.subchunk_1_id;
//	i+=4;

//	value32 = (uint32_t *) &data[16];	
//	*(value32) = header.subchunk_1_size;
//	i+=4;
//	
//	value16 = (uint16_t *) &data[20];
//	*(value16) = header.format_no;
//	i+=2;;
//	
//	value16 = (uint16_t *) &data[22];
//	*(value16) = header.num_channels;
//	i+=2;
//	
//	value32 = (uint32_t *) &data[24];
//	*(value32) = header.sample_rate;
//	i+=4;
//	
//	value32 = (uint32_t *) &data[28];
//	*(value32) = header.byte_rate;
//	i+=4;
//	
//	value16 = (uint16_t *) &data[32];
//	*(value16) = header.block_align;
//	i+=2;
//	
//	value16 = (uint16_t *) &data[34];
//	*(value16) = header.bits_per_sample;
//	i+=2;
//	
//	value32 = (uint32_t *) &data[36];
//	*(value32) = header.subchunk_2_id;
//	i+=4;
//	
//	value32 = (uint32_t *) &data[40];
//	*(value32) = header.subchunk_2_size;

//	return data;

//}/* BMO_gen_wav_header */


