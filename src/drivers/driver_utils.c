#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef BMO_HAVE_PORTAUDIO
#include <portaudio.h>
#endif

#include "../definitions.h"
#include "../error.h"


BMO_state_t *bmo_new_state(void)
{
    BMO_state_t *state = malloc(sizeof(BMO_state_t));
    if (!state) {
        bmo_err("Alloc failure:%s\n", strerror(errno));
        return NULL;
    }
    state->driver_id = 0;
    state->driver_rate = 0;
    state->n_playback_ch = 0;
    state->n_capture_ch = 0;
    state->n_ticks = 0;
    state->dsp_flags = BMO_DSP_STOPPED;
    state->buffer_size = BMO_DEFAULT_BUF;
    state->dsp_load = 0.0;
    state->ringbuffer = NULL;
    // .driver = {{0}}
    return state;
}


uint32_t bmo_rate(BMO_state_t * state)
{
    if (!state)
        return 0;
    return state->driver_rate;
}

uint32_t bmo_playback_count(BMO_state_t * state)
{
    // counter intuitive?  These are inputs /into/ the driver similar to inputs
    // *into* an amplifier. This terminology comes from JACK.
    return state->n_playback_ch;
}

uint32_t bmo_capture_count(BMO_state_t * state)
{
    return state->n_capture_ch;
}

const char * bmo_strdriver(BMO_state_t * state)
{
    switch (state->driver_id) {
        case BMO_DUMMY_DRIVER: return "dummy driver writing to disk";
        case BMO_JACK_DRIVER: return "Jack driver";
        case BMO_PORTAUDIO_DRIVER: return "Portaudio Driver";
        default: return NULL;
    }
}

uint32_t bmo_driver_ver(BMO_state_t * state)
{
    switch (state->driver_id) {
        case BMO_DUMMY_DRIVER: return BMO_VERSION_MAJOR;
        case BMO_JACK_DRIVER:
            // jack does not seem to have a way to query the server version
            return 0;
        case BMO_PORTAUDIO_DRIVER:
        #ifdef BMO_HAVE_PORTAUDIO
            return (uint32_t)Pa_GetVersion();
        #else
            return 0;
        #endif
        default: return 0;
    }
}

uint32_t bmo_bufsize(BMO_state_t * state)
{
    return state->buffer_size;
}

void bmo_start(BMO_state_t * state)
{
    uint32_t status = state->dsp_flags;
    if (status & BMO_DSP_STOPPED) {
        status = status ^ BMO_DSP_STOPPED; // mask out the 'stopped' bits
        status |= BMO_DSP_RUNNING;
        state->dsp_flags = status;
    }
}

void bmo_stop(BMO_state_t * state)
{
    uint32_t status = state->dsp_flags;
    if (status & BMO_DSP_RUNNING) {
        status = status ^ BMO_DSP_RUNNING; // mask out the running bits
        status |= BMO_DSP_STOPPED;
        state->dsp_flags = status;
    }
}

uint32_t bmo_status(BMO_state_t * state)
{
    assert(((state->dsp_flags & (BMO_DSP_RUNNING | BMO_DSP_STOPPED)) ^
            (BMO_DSP_RUNNING | BMO_DSP_STOPPED)) != 0);
    return state->dsp_flags & (BMO_DSP_RUNNING | BMO_DSP_STOPPED);
}

