#ifndef BMO_DATATYPES_H
#define BMO_DATATYPES_H

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#ifdef BMO_HAVE_SNDFILE
#include <sndfile.h>
#endif

#ifdef BMO_HAVE_PORTAUDIO
#include <portaudio.h>
#endif

#ifdef BMO_HAVE_JACK
#include <jack/jack.h>
#endif

#ifdef BMO_HAVE_LUA
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#endif

#ifdef BMO_HAVE_PTHREAD
#include <pthread.h>
#endif

#define BMO_NOT_IMPLEMENTED assert("():Not Implemented" == NULL)

typedef float BMO_float_t;

#ifdef __GNUC__

typedef float vec4f __attribute__ ((vector_size (16)));
typedef int32_t vec4i __attribute__ ((vector_size (16)));
typedef uint32_t vec4u __attribute__ ((vector_size (16)));
#define BMO_DEPRECATED __attribute__ ((deprecated))
#ifdef _WIN32
#define BMO_EXPORT __attribute__ ((dllexport))
#endif
#endif

#define BMO_VERSION_MAJOR   0
#define BMO_VERSION_MINOR   0
#define BMO_VERSION_PATCH   0

#ifndef __STDC_IEC_559__
#warning Many routines expect IEEE 754 representation for floats: you have been warned
#endif 

#define BMO_NO_DRIVER 0
#define BMO_JACK_DRIVER 1
#define BMO_PORTAUDIO_DRIVER 2
#define BMO_DUMMY_DRIVER 3
/* Add extra driver definitions here*/

/* math constants*/
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif       
#ifndef PI_180
#define PI_180 	M_PI / 180.0	
#endif

/* MAGIC numbers / defaults */
#define BMO_MAX_IO 128
#define BMO_DEFAULT_CHANNELS 2
#define BMO_DEFAULT_BUF 8192
#define BMO_DEFAULT_STR_BUF 1024
#define BMO_DEFAULT_RATE 44100

#define ENDIAN_BIG	 	0xbbbbbbbb
#define ENDIAN_LITTLE	0xaaaaaaaa
/*endianess tests*/
#ifndef BMO_ENDIAN_BIG
#ifndef BMO_ENDIAN_LITTLE
#error host endianness not defined
#endif
#endif

#ifdef BMO_ENDIAN_LITTLE
#define SIGN_BIT 31
#else
#define SIGN_BIT 0
#endif

/*format numbers  (OR-able as flags)*/
#define BMO_FMT_PCM_U8 		    0x00000001
#define BMO_FMT_PCM_8 		    0x00000002 		///< 8-bit linear PCM
#define BMO_FMT_PCM_16_LE		0x00000004 		///< 16-bit linear PCM
#define BMO_FMT_PCM_24_LE		0x00000008 		///< 24-bit linear PCM
#define BMO_FMT_PCM_32_LE		0x00000010 		///< 32-bit linear PCM
#define BMO_FMT_FLOAT_32_LE		0x00000020 		///< 32-bit IEEE floating point
#define BMO_FMT_FLOAT_64_LE		0x00000040 		///< 64-bit IEEE floating point
#define BMO_FMT_PCM_16_BE		0x00000080 		///< 16-bit linear PCM
#define BMO_FMT_PCM_24_BE		0x00000100 		///< 24-bit linear PCM
#define BMO_FMT_PCM_32_BE		0x00000200 		///< 32-bit linear PCM
#define BMO_FMT_FLOAT_32_BE		0x00000400		///< 32-bit IEEE floating point
#define BMO_FMT_FLOAT_64BE		0x00000800 		///< 64-bit IEEE floating point

#define BMO_BUFFERED_DATA				0x00001000
#define BMO_MAPPED_FILE_DATA			0x00002000
#define BMO_EXTERNAL_DATA				0X00004000
#define BMO_INT_TO_FLOAT				0x00008000		//conversion direction flags
#define	BMO_FLOAT_TO_FLOAT				0x00010000
#define	BMO_FLOAT_TO_INT				0x00020000
#define BMO_INT_TO_INT					0x00040000
#define BMO_DITHER_NONE					0x00000000		//dither type flags
#define BMO_DITHER_TPDF					0x00100000		///<dither flag using a triangular Probability Density Function PRNG
#define BMO_DITHER_SHAPED				0x00200000		///<dither flag for shaped noise
#define BMO_FMT_NATIVE_FLOAT			0x00400000 		//TODO define this in terms of machine endianess
#define BMO_FMT_SNDFILE				    0x00800000
#define BMO_DSP_RUNNING					0x00100000		
#define BMO_DSP_STOPPED					0x00200000
#define BMO_REALTIME					0x00400000

/**
The audio buffer object contains data as to the encoding and mapping of audio data in memory on disk, or fetched from a dynamic stream (ADC).
Audio buffers can either be decoded into memory, dynamically demultiplexed and decoded from from an mmapped file, or got from a network / audio interface / jack buffer.  This is transparent to the dsp and audio object functions which will call the get_samples() callback function defined at object creation.  This will work regardless of the input method, and simplifies logic.
*/
#define BMO_DSP_OBJ_LADSPA              0x00800000
#define BMO_DSP_OBJ_LUA                 0x01000000
#define BMO_DSP_OBJ_RB                 0x02000000
#define BMO_DSP_OBJ_BO                  0x04000000
#define BMO_DSP_TYPE_INPUT              0x08000000
#define BMO_DSP_TYPE_OUTPUT 0x10000000
#define BMO_DSP_TYPE_USER   0x20000000


#define LINEAR_INTERP 	0x0000000f
#define POLYNOM_INTERP 	0x000000f0
#define ZOH_NO_INTERP 	0x00000f00
#define	SPLINE_INTERP	0x0000f000
#define LOW_Q_FILT		0x000f0000
#define	MED_Q_FILT		0x00f00000
#define	HI_Q_FILT		0x0f000000
#define NO_FILT			0xf0000000

#ifdef BMO_HAVE_SNDFILE
typedef struct
{
	struct SF_INFO info;
} BMO_sndfile_state_t;
#endif

typedef union
{
	float ** buffered_audio;			//store decoded and demultiplexed floating point multichannel data.
	void * interleaved_audio;		//void because the datatype is unknown ->force conversion to the correct type
	struct BMO_dsp_obj_t * dsp_obj;
	#ifdef BMO_HAVE_SNDFILE
	BMO_sndfile_state_t sf_state;	
	#endif						
	//Add your storage handles here
}BMO_buffer_obj_storage_t;

typedef struct BMO_buffer_obj_t
{
	uint32_t type;	        		///< file-backed mmapped interleaved audio, decoded buffered audio, stream or external library callback
									///< one of BMO_BUFFERED_DATA, BMO_MAPPED_FILE_DATA, BMO_EXTERNAL_DATA BMO_DSP_OBJ
	uint32_t encoding;				///< format of the stream one of BMO_FORMAT_...
	uint32_t channels;				///< channel count of buffer
	uint32_t loop;
	size_t index;					///< current frame
	size_t frames;				    ///< total frames in the audio data
	uint32_t rate;					///< sample rate of the audio data
	size_t file_len;				///< total bytes of whole file for unmapping
	void * handle;					///< retargetable pointer for bmo_map, or other codec state. (e.g.the sndfile callback uses this for SNDFILE *)
	BMO_buffer_obj_storage_t buffer;
	int (*get_samples)(void * bo, float ** dest, uint32_t frames);			// function pointer to callback that delivers samples.
	int (*seek)(void * obj, long off, int whence);				
	size_t (*tell)(void * obj);	//
	char is_alias;//flags for object aliasing another (don't free resources of alias types)
}BMO_buffer_obj_t;

typedef struct BMO_ll_t
{
    //basic singly linked list
    struct BMO_ll_t * next;
     void * data;
}BMO_ll_t;

typedef struct BMO_dsp_obj_t
{
	uint64_t id;							///< unique identifier for this object
	uint32_t type;							///< determines the type of the dsp object. one of plugin, lua interpreter, builtin etc.
	uint32_t channels;						///< number of channels in the data
	uint32_t rate;							///< sample rate
	uint32_t tick;							///< last buffer processed
	uint32_t frames;
	uint32_t flags;
	float ** in_buffers;					///< audio/data state for the object
	float ** out_buffers;					///< plugins write altered samples back here
	float ** ctl_buffers;					///< sidechain data expected to be both read 

	//DSP processing state/functions
	void  * handle;							///< misc reusable internal state storage data
	
	int (*_init)(void *, uint32_t);						///<setup function.
	int (*_update)(void *, uint32_t);					///<function pointer to call process update routines 
	int (*_close)(void *, uint32_t);					///<pointer to function deallocating state
	
	BMO_ll_t * in_ports;					///< list of objects from which to get and mix input data
	BMO_ll_t * ctl_ports;					///< list of objects from which to get and mix control(sidechain)input data
} BMO_dsp_obj_t;


typedef struct
{
	uint32_t channels;
	uint32_t frames;						///<number of samples in each channel
	volatile uint32_t read_index;			///<current position (aligned to channel 0) ie frame
	volatile uint32_t write_index;			///<current position to write data
	volatile uint32_t write_max_el;			///<maximum number of frames to write in one go - otherwise unread data will be overwritten
	volatile uint32_t read_max_el;			///<maximum number of frames to read in one go - otherwise redundant data will output
	float ** data;							///<audio buffers
} BMO_ringbuffer_t;


#ifdef BMO_HAVE_JACK
typedef struct
{	const char **ports;						    ///< list of port ids returned by the jack server
	const char *server_name;				    ///< current server name
	const char *client_name;				    ///< client id if none is set by the user, this will default to "BMO_client"
	jack_client_t *client;					    ///< the client pointer used to identify who's making requests to jack
	jack_status_t status;					    ///< a pointer to this is passed to jack, and it should be checked by the client for status codes
	jack_options_t options;					    ///< OR-able options flags given to the jackserver on client connection
	jack_port_t * output_ports[BMO_MAX_IO];	///< pointers that identify output ports.
	jack_port_t * input_ports[BMO_MAX_IO];	///< 
}BMO_jack_state_t;
#endif

#ifdef BMO_HAVE_PORTAUDIO
typedef struct
{
	PaStream * stream;                          ///<
	PaStreamParameters output_params;           ///<
	PaError error_num;                          ///
	PaDeviceInfo * device_info; 
}BMO_PA_state_t;
#endif

#ifdef __linux__
#include <sys/epoll.h>
#include <pthread.h>
typedef struct {
	int epfd;
	int fd;
	struct epoll_event event;
}BMO_dummy_ipc_t_;

typedef struct
{
    
	FILE * file;
	const char * file_path;
	uint64_t nano_secs_per_buffer;
	uint64_t nano_secs_last_callback;
	uint64_t nano_secs_sleep_callback;
	uint32_t error_num;
	uint32_t dither_type;
	uint32_t stream_format;
	float ** out_buffers;
	int (*process_callback)(void * arg, uint32_t frames);		// the dummy process callback address.
	int (*xrun_callback)(void * arg, double secs_late);
	int (*error_callback)(void * arg, uint32_t err);	
	int (*finished_callback)(void * arg, uint32_t n);
	int is_realtime;
	int is_running;
	pthread_t disk_io_thread;
	pthread_t supervisor_thread;
	BMO_dummy_ipc_t_ ipc;
}BMO_dummy_state_t;
#endif
typedef struct {
    int driver_pipefd[2];
    int process_pipefd[2];
    int message_pipefd[2];
} BMO_ipc_t;

typedef union
{
	#ifdef BMO_HAVE_PORTAUDIO
	BMO_PA_state_t pa;
	#endif
	#ifdef BMO_HAVE_JACK
	BMO_jack_state_t jack;
	#endif
	#ifdef __linux__
	BMO_dummy_state_t dummy;
	#endif
}BMO_driver_state_t;

typedef struct
{
    uint64_t n_ticks;				///< since engine start, how many buffers have been processed.  referenced by each instance of a BMO_dsp_obj_t to ensure buffer updates don't duplicate.
	uint32_t flags;
	uint32_t driver_id;				///< identifier to decide on capabilities and driver use
	uint32_t driver_rate;			///< audio driver's current sample rate
	uint32_t n_playback_ch; ///< number of playback channels
	uint32_t n_capture_ch;	///< number of capture channels
	
	uint32_t dsp_flags;				///< contains the current state of the dsp system BMO_DSP_RUNNING, BMO_DSP_STOPPED etc.
	uint32_t buffer_size;			///< ringbuffer and driver buffer length
	float dsp_load;					///< current load of the system (jack and portaudio report this)
	BMO_ringbuffer_t * ringbuffer;	///< a pointer into the ringBuffer structure
	BMO_buffer_obj_t * getter;
	BMO_driver_state_t driver;		///< all driver unique pointers / identifiers / ports are in this union
	BMO_ipc_t ipc;
} BMO_state_t;

typedef struct
{
	BMO_dsp_obj_t * obj;
	int position;
} alias;
#endif

