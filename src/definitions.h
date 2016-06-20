#ifndef BMO_DATATYPES_H
#define BMO_DATATYPES_H

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stddef.h>
#include <sys/types.h>
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
#include <lua.h>
#include <lauxlib.h>
#endif

#ifdef BMO_HAVE_PTHREAD
#include <pthread.h>
#endif

#ifndef NDEBUG
#define BMO_ABORT abort
#else
#define BMO_ABORT()
#endif
#define BMO_UNREACHABLE(fmt, ...)                                         \
    do {                                                                  \
        fprintf(stderr, "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
        BMO_ABORT();                                                      \
    } while (0);

#define BMO_NOT_IMPLEMENTED BMO_UNREACHABLE("%s", "Not Implemented")

typedef float BMO_float_t;

#ifdef __GNUC__
typedef float vec4f __attribute__((vector_size(16)));
typedef int32_t vec4i __attribute__((vector_size(16)));
typedef uint32_t vec4u __attribute__((vector_size(16)));
#define BMO_DEPRECATED __attribute__((deprecated))
#ifdef _WIN32
#define BMO_EXPORT __attribute__((dllexport))

#endif
#endif

#define BMO_VERSION_MAJOR (0)
#define BMO_VERSION_MINOR (0)
#define BMO_VERSION_PATCH (0)

#define BMO_NO_DRIVER (0)
#define BMO_JACK_DRIVER (1)
#define BMO_PORTAUDIO_DRIVER (2)
#define BMO_DUMMY_DRIVER (3)
/* Add extra driver definitions here*/

/* math constants*/
#ifndef M_PI
#define M_PI (3.14159265358979323846264338327)
#endif
#ifndef PI_180
#define PI_180 (M_PI / 180.0)
#endif

/* MAGIC numbers / defaults */
#define BMO_MAX_IO (128)
#define BMO_DEFAULT_CHANNELS (2)
#define BMO_DEFAULT_BUF (8192)
#define BMO_DEFAULT_STR_BUF (1024)
#define BMO_DEFAULT_RATE (44100)

#define ENDIAN_BIG (0x00000000)
#define ENDIAN_LITTLE (0xffffffff)
/*endianess tests*/
#ifndef BMO_ENDIAN_BIG
#ifndef BMO_ENDIAN_LITTLE
#error host endianness not defined
#endif
#endif

#ifdef BMO_ENDIAN_LITTLE
#define SIGN_BIT (31)
#else
#define SIGN_BIT (0)
#endif

/*format numbers  (OR-able as flags)*/
#define BMO_FMT_PCM_U8          (0x00000001)
/// 8-bit linear PCM
#define BMO_FMT_PCM_8           (0x00000002)
/// 16-bit linear PCM
#define BMO_FMT_PCM_16_LE       (0x00000004)
/// 24-bit linear PCM
#define BMO_FMT_PCM_24_LE       (0x00000008)
/// 32-bit linear PCM
#define BMO_FMT_PCM_32_LE       (0x00000010)
/// 32-bit IEEE floating point
#define BMO_FMT_FLOAT_32_LE     (0x00000020)
/// 64-bit IEEE floating point
#define BMO_FMT_FLOAT_64_LE     (0x00000040)
/// 16-bit linear PCM
#define BMO_FMT_PCM_16_BE       (0x00000080)
/// 24-bit linear PCM
#define BMO_FMT_PCM_24_BE       (0x00000100)
/// 32-bit linear PCM
#define BMO_FMT_PCM_32_BE       (0x00000200)
/// 32-bit IEEE floating point
#define BMO_FMT_FLOAT_32_BE     (0x00000400)
/// 64-bit IEEE floating point
#define BMO_FMT_FLOAT_64BE      (0x00000800)

#define BMO_BUFFERED_DATA       (0x00001000)
#define BMO_MAPPED_FILE_DATA    (0x00002000)
#define BMO_EXTERNAL_DATA       (0X00004000)
// conversion direction flags
#define BMO_INT_TO_FLOAT        (0x00008000)
#define BMO_FLOAT_TO_FLOAT      (0x00010000)
#define BMO_FLOAT_TO_INT        (0x00020000)
#define BMO_INT_TO_INT          (0x00040000)
// dither type flags
#define BMO_DITHER_NONE         (0x00000000)
/// dither flag using a triangular Probability Density Function PRNG
#define BMO_DITHER_TPDF         (0x00100000)
/// dither flag for shaped noise
#define BMO_DITHER_SHAPED       (0x00200000)
//TODO define this in terms of machine endianess
#define BMO_FMT_NATIVE_FLOAT    (0x00400000)
// #define BMO_FMT_SNDFILE         (0x00800000)
#define BMO_DSP_RUNNING         (0x00100000)
#define BMO_DSP_STOPPED         (0x00200000)
#define BMO_REALTIME            (0x00400000)

/**
The audio buffer object contains data as to the encoding and mapping of audio
data in memory on disk, or fetched from a dynamic stream (ADC).
Audio buffers can either be decoded into memory, dynamically demultiplexed and
decoded from from an mmapped file, or got from a network / audio interface /
jack buffer.  This is transparent to the dsp and audio object functions which
will call the read() callback function defined at object creation.  This will
work regardless of the input method, and simplifies logic.
*/
#define BMO_DSP_OBJ_LADSPA  (0x00800000)
#define BMO_DSP_OBJ_LUA     (0x01000000)
#define BMO_DSP_OBJ_RB      (0x02000000)
#define BMO_DSP_OBJ_BO      (0x04000000)
#define BMO_DSP_TYPE_INPUT  (0x08000000)
#define BMO_DSP_TYPE_OUTPUT (0x10000000)
#define BMO_DSP_TYPE_USER   (0x20000000)
#define BMO_BUF_READ        (0x40000000)
#define BMO_BUF_WRITE       (0x80000000)

#define LINEAR_INTERP   (0x0000000f)
#define POLYNOM_INTERP  (0x000000f0)
#define ZOH_NO_INTERP   (0x00000f00)
#define SPLINE_INTERP   (0x0000f000)
#define LOW_Q_FILT      (0x000f0000)
#define MED_Q_FILT      (0x00f00000)
#define HI_Q_FILT       (0x0f000000)
#define NO_FILT         (0xf0000000)

#ifdef BMO_HAVE_SNDFILE
typedef struct {
    struct SF_INFO info;
    struct SF_VIRTUAL_IO vio_info;
} BMO_sndfile_state_t;

#endif

/*
typedef union {
    // store decoded and demultiplexed floating point multichannel data.
    float **mb_native;
    // void because the datatype is unknown ->force conversion to the correct
    // type
    void *interleaved_raw;
    void *mapped_data;
    int fd;
    struct BMO_dsp_obj_t * dsp_obj;
    #ifdef BMO_HAVE_SNDFILE
    BMO_sndfile_state_t sf_state;
    #endif
    //Add your storage handles here
} BMO_buffer_obj_storage_t;
*/

/*
typedef struct {
    uint32_t channels;
    uint32_t frames;
    uint32_t rate;
    uint32_t flags;
} BMO_spec_t;
*/

typedef struct BMO_buffer_obj_t {
    /// total bytes of whole file for unmapping
    size_t buf_siz;
    size_t offset;
    /// userdata
    void *userdata;
    /// current frame indext
    size_t frame;
    /// one of BMO_BUFFERED_DATA, BMO_MAPPED_FILE_DATA, BMO_EXTERNAL_DATA
    uint32_t flags;
    /// channel count of buffer
    uint32_t channels;
    /// total frames in the audio data
    uint32_t frames;
    /// sample rate of the audio data
    uint32_t rate;
    /// callback that delivers `frames` samples into `dest`
    ssize_t (*read)(struct BMO_buffer_obj_t *bo, float **dest, uint32_t frames);
    ssize_t (*write)(struct BMO_buffer_obj_t *bo, float **src, uint32_t frames);
    ssize_t (*seek)(struct BMO_buffer_obj_t *bo, ptrdiff_t offset, int whence);
    // Get the current frame (returns -1 on failure)
    ssize_t (*tell)(struct BMO_buffer_obj_t *bo);
    void (*close)(struct BMO_buffer_obj_t *bo);
    // flags for object aliasing another (don't free resources of alias types)
    char is_alias;
    char loop;
} BMO_buffer_obj_t;

typedef struct BMO_ll_t
{
    //basic singly linked list
    struct BMO_ll_t * next;
     void * data;
} BMO_ll_t;

typedef struct BMO_dsp_obj_t
{
    /// unique identifier for this object
    uint64_t id;
    /// determines the type of the dsp object. one of plugin, lua interpreter,
    /// builtin etc.
    uint32_t type;
    /// number of channels in the data
    uint32_t channels;
    /// sample rate
    uint32_t rate;
    /// last buffer processed
    uint32_t tick;
    /// frames per tick
    uint32_t frames;
    uint32_t flags;
    /// audio/data state for the object
    float **in_buffers;
    /// plugins write altered samples back here
    float **out_buffers;
    /// sidechain data expected to be both read
    float **ctl_buffers;

    // DSP processing state/functions
    /// misc reusable internal state storage data
    void *userdata;

    /// setup function.
    int (*_init)(struct BMO_dsp_obj_t *, uint32_t);
    /// function pointer to call process update routines
    int (*_update)(struct BMO_dsp_obj_t *, uint32_t);
    /// pointer to function deallocating state
    int (*_close)(struct BMO_dsp_obj_t *, uint32_t);

    /// list of objects from which to get and mix input data
    BMO_ll_t *in_ports;
    /// list of objects from which to get and mix control(sidechain)input data
    BMO_ll_t *ctl_ports;
} BMO_dsp_obj_t;


typedef struct
{
    uint32_t channels;
    /// number of samples in each channel
    uint32_t frames;
    /// current position (aligned to channel 0) ie frame
    volatile uint32_t read_index;
    /// current position to write data
    volatile uint32_t write_index;
    /// maximum number of frames to write in one go - otherwise unread data will
    /// be overwritten
    volatile uint32_t write_max_el;
    /// maximum number of frames to read in one go - otherwise redundant data
    /// will output
    volatile uint32_t read_max_el;
    /// audio buffers
    float **data;
} BMO_ringbuffer_t;


#ifdef BMO_HAVE_JACK
typedef struct {
    /// list of port ids returned by the jack server
    const char **ports;
    /// current server name
    const char *server_name;
    /// client id if none is set by the user, this will default to "BMO_client"
    const char *client_name;
    /// the client pointer used to identify who's making requests to jack
    jack_client_t *client;
    /// a pointer to this is passed to jack, and it should be checked by the
    /// client for status codes
    jack_status_t status;
    /// OR-able options flags given to the jackserver on client connection
    jack_options_t options;
    /// pointers that identify output ports.
    jack_port_t *output_ports[BMO_MAX_IO];
    ///
    jack_port_t *input_ports[BMO_MAX_IO];
} BMO_jack_state_t;
#endif

#ifdef BMO_HAVE_PORTAUDIO
typedef struct
{
    PaStream *stream;
    PaStreamParameters output_params;
    PaError error_num;
    PaDeviceInfo *device_info;
} BMO_PA_state_t;
#endif

typedef struct {
    int driver_pipefd[2];
    int process_pipefd[2];
    int message_pipefd[2];
} BMO_ipc_t;

typedef union {
#ifdef BMO_HAVE_PORTAUDIO
        BMO_PA_state_t pa;
#endif
#ifdef BMO_HAVE_JACK
        BMO_jack_state_t jack;
#endif
#ifdef __linux__
//    BMO_dummy_state_t dummy;
    #endif
} BMO_driver_state_t;

typedef struct {
    void *(*allocfn)(void *ptr, size_t oldsize, size_t newsize, void *userdata);
    void *userdata;
} BMO_alloc_t;

typedef struct BMO_mq_t BMO_mq_t;
typedef struct {
    /// Since engine start, how many buffers have been processed.
    /// referenced by each instance of a BMO_dsp_obj_t to ensure
    /// buffer updates don't duplicate.
    uint64_t n_ticks;
    uint32_t flags;
    /// identifier to decide on capabilities and driver use
    uint32_t driver_id;
    /// audio driver's current sample rate
    uint32_t driver_rate;
    /// number of playback channels
    uint32_t n_playback_ch;
    /// number of capture channels
    uint32_t n_capture_ch;
    /// contains the current state of the dsp system BMO_DSP_RUNNING,
    /// BMO_DSP_STOPPED etc.
    uint32_t dsp_flags;
    /// ringbuffer and driver buffer length
    uint32_t buffer_size;
    /// current load of the system (jack and portaudio report this)
    float dsp_load;
    /// a pointer into the ringbuffer structure
    BMO_buffer_obj_t *getter;
    /// all driver unique pointers / identifiers / ports are in this union
    BMO_driver_state_t driver;
    BMO_ringbuffer_t *ringbuffer;
    BMO_ipc_t ipc;
    BMO_alloc_t alloc;
  //  BMO_mq_t *mq;

} BMO_state_t;

typedef struct
{
    BMO_dsp_obj_t * obj;
    int position;
} alias;
#endif
