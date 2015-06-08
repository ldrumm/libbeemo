#ifdef BMO_HAVE_JACK
#include <jack/types.h>
#include <jack/jack.h>
#include <jack/statistics.h>

#include <stdio.h>
#include <stdlib.h>
#include "jack_functions.h"
#include "../definitions.h"
#include "../deleteme_sched.h"
#include "../error.h"
#include "ringbuffer.h"
#include "driver_utils.h"
#include "../dsp/simple.h"


static void
_bmo_jack_err_cb(jack_status_t status)
{
    /* descriptions are straight from the jack docs*/
    if(status & JackServerStarted)  bmo_err("The JACK server was started\n");
    if(status & JackFailure)        bmo_err("Overall operation failed. \n");
    if(status & JackInvalidOption)  bmo_err("The operation contained an invalid or unsupported option.\n");
    if(status & JackNameNotUnique)  bmo_err("The request client name was not unique.\n");
    if(status & JackServerFailed)   bmo_err("Unable to connect to the JACK server.\n");
    if(status & JackServerError)    bmo_err("Communication error with the JACK server.\n");
    if(status & JackNoSuchClient)   bmo_err("Requested client does not exist.\n");
    if(status & JackLoadFailure)    bmo_err("Unable to load internal client.\n");
    if(status & JackInitFailure)    bmo_err("Unable to initialize client.\n");
    if(status & JackShmFailure)     bmo_err("Unable to access shared memory.\n");
    if(status & JackVersionError)   bmo_err("Client's protocol version does not match.\n");
    if(status & JackBackendError)   bmo_err("Jack Backend Error.\n");
    if(status & JackClientZombie)   bmo_err("Braaaains...\n");
    if(!status)
        ;
    else bmo_err("an unknown Jack error occurred \n");
}

static void
_bmo_jack_finished_cb(void * arg)
{
    (void)arg;
       bmo_info( "Jack Stream Completed\n" );
       bmo_jack_stop(arg);
}

static int
_bmo_jack_xrun_cb(void * arg)
{
    float delay = jack_get_xrun_delayed_usecs(((BMO_state_t *)arg)->driver.jack.client);
    bmo_err("XRUN occurred of %fusecs. Consider increasing the buffer size.\n", delay);
    return 0;
}

static int
_bmo_jack_bufsize_cb(jack_nframes_t frames, void * arg)
{
    (void)arg;
    bmo_err("jack buffersize has been set at %d frames/call\n", frames);
//    bmo_jack_stop(arg);
    return 0;
}

static int
_bmo_jack_src_cb(jack_nframes_t src, void *arg)
{
    (void)arg;
    bmo_err( "AUD:Jack Samplerate has changed to %d. \n    \
        Will attempt to update the output, but expect weirdness.\n", src);
//    bmo_jack_stop(arg);
    return 0;
}

static int
_bmo_jack_process_cb(jack_nframes_t frames, void *arg)
{
    if(!arg){
        return -1;
    }
    uint32_t i, j;
    i = j = 0;

    BMO_state_t * state = (BMO_state_t *) arg;
    jack_default_audio_sample_t ** outs[bmo_playback_count(state)];
    /*get output ports pointer*/
    while(i< state->n_playback_ch){
        if(state->driver.jack.output_ports[i])
            if(jack_port_connected(state->driver.jack.output_ports[i]) > 0){
                outs[j] = jack_port_get_buffer(state->driver.jack.output_ports[i], frames);
                if(!outs[j]){
                    bmo_err("outport %d is null");
                    break;
                }
                bmo_zero_sb((float *)outs[j], frames);
                j++;
            }
        i++;
    }
    if(i == bmo_playback_count(state)){
        uint32_t frames_read = bmo_read_rb(state->ringbuffer, (float **)outs, frames);
        if(frames_read < frames){
            bmo_err("An underrun of %u samples occurred\n");
        }
        state->dsp_load = (float)jack_cpu_load(state->driver.jack.client);
        bmo_driver_callback_done(state, BMO_DSP_RUNNING);
        return 0;
    }
    else bmo_err("jack/client output ports mismatch:%d channels available\n", i);
    bmo_driver_callback_done(state, BMO_DSP_STOPPED);
    return -1;
}

void
bmo_jack_stop(BMO_state_t * state)
{
    uint32_t i = 0;
    bmo_stop(state);
    while(state->driver.jack.output_ports[i]){
        jack_port_unregister(
            state->driver.jack.client, state->driver.jack.output_ports[i]
        );
        state->driver.jack.output_ports[i] = NULL;
        i++;
    }
    //TODO NULLify ports list etc.
    jack_client_close(state->driver.jack.client);
}

BMO_state_t * bmo_jack_start(
        BMO_state_t * state,
        uint32_t channels,
        uint32_t rate,
        uint32_t buf_len,
        uint32_t flags,
        const char *client_name
)
{
    (void) flags; //FIXME unused
    size_t portname_buf_len = jack_port_name_size(); //includes the trailing NUL
    char * portname_buf;
    uint32_t i = 0;

    if(!state){
        bmo_err("bmo_jack_start:couldn't get state\n");
        return NULL;
    }

    state->driver_id = BMO_JACK_DRIVER;
    state->driver.jack.ports = NULL;
    state->driver.jack.server_name = NULL;
    state->driver.jack.client_name = NULL;
    state->driver.jack.client = NULL;
    state->driver.jack.options = JackNullOption;
    while(i < bmo_playback_count(state)){
        state->driver.jack.output_ports[i] = NULL;
        state->driver.jack.input_ports[i] = NULL;
        i++;
    }

    /* set client name*/
    if(!client_name){
        client_name = "BMO_client";
    }
    state->driver.jack.client_name = client_name;
    bmo_debug("Client name is '%s'\n", client_name);

    /* connect to jack and setup return client identifier */
    state->driver.jack.client =
    jack_client_open(
        client_name,
        state->driver.jack.options,
        &state->driver.jack.status,
        state->driver.jack.server_name
    );
    if(!state->driver.jack.client){
        _bmo_jack_err_cb(state->driver.jack.status);
        return NULL;
    }

    state->driver.jack.client_name = jack_get_client_name(state->driver.jack.client);

    /* register the processing, error, and event-notification callbacks with jack */
    jack_set_process_callback (state->driver.jack.client, _bmo_jack_process_cb, state);
    jack_on_shutdown(state->driver.jack.client, _bmo_jack_finished_cb, state);
    jack_set_xrun_callback(state->driver.jack.client, _bmo_jack_xrun_cb, state);
    jack_set_sample_rate_callback(state->driver.jack.client, _bmo_jack_src_cb, state);
    jack_set_buffer_size_callback(state->driver.jack.client, _bmo_jack_bufsize_cb, state);

    /*get and set buffer_size in running jack instance */
    state->buffer_size = jack_get_buffer_size(state->driver.jack.client);
    if(state->buffer_size != buf_len){
        bmo_info("requested buf_len does not match jack's buf_len.\n");
    }

    /* initialise the ringbuffer if not already done */
    if(!state->ringbuffer){
        state->ringbuffer = bmo_init_rb(state->buffer_size, channels);
        if(!state->ringbuffer)
            return NULL;
    }

    state->driver_rate = jack_get_sample_rate (state->driver.jack.client);
    if(state->driver_rate != rate){
        bmo_info("jack is running at %dHz. using this instead of requested %d\n",
            state->driver_rate,
            rate
        );
        //TODO something useful like resetting sample rate conversion etc
    }

    /* create client output ports of the requested number of channels */
    portname_buf = calloc(portname_buf_len, sizeof(char));
    if(!portname_buf){
        bmo_err("out of memory\n");
        return NULL;
    }
    for(i = 0; i < channels; i++){
        snprintf(portname_buf, portname_buf_len, "channel %d", i+1);
        state->driver.jack.output_ports[i] = jack_port_register(
            state->driver.jack.client,
            portname_buf,
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsOutput,
            0
        );
        if((!state->driver.jack.output_ports[i])){
            bmo_err("no more Jack ports available\n");
            free(portname_buf);
            return NULL;
        }
    }
    free(portname_buf);
    bmo_debug("created %d client output ports\n", i);
    state->n_playback_ch = i;

    /*start jack processing this client*/
    if(jack_activate(state->driver.jack.client)){
        bmo_err("Jack cannot activate client");
        return NULL;
    }

    /* query and connect client outputs to jack inputs*/
    state->driver.jack.ports = jack_get_ports(
        state->driver.jack.client,
        NULL,
        NULL,
        JackPortIsPhysical | JackPortIsInput
    );
    if(!state->driver.jack.ports){
        bmo_err("Couldn't get any ports into jack server\n");
        bmo_jack_stop(state);
        return NULL;
    }

    for(i = 0; state->driver.jack.ports[i] ; i++){
        ;
    }
    bmo_debug("Jack has %d physical input ports\n", i);
    if(!i){
        bmo_err("Jack has no input ports available\n");
        bmo_jack_stop(state);
        return NULL;
    }
    state->n_playback_ch = i;

    /* connect all client ports to the Jack server */
    i = 0;
    while(state->driver.jack.output_ports[i]){
        jack_connect(
            state->driver.jack.client,
            jack_port_name(state->driver.jack.output_ports[i]),
            state->driver.jack.ports[i % state->n_playback_ch]
        );
        i++;
    }
    bmo_debug("Connected %d ports to physical inputs on the jack server\n", i);

    if(i < channels){
        bmo_info("Couldn't connect all output ports to independent physical outputs;"\
            "will connect all extra ports between existing %d channels\n", i);
        i = 0;
        bmo_debug("jack has these system playback ports available:\n");
        while(state->driver.jack.ports[i]){
            bmo_debug("%s\n", state->driver.jack.ports[i]);
            i++;
        }
    }

    return state;
}


#endif
