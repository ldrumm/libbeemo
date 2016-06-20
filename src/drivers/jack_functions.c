#ifdef BMO_HAVE_JACK
#include <stdio.h>
#include <stdlib.h>

#include <jack/jack.h>
#include <jack/statistics.h>
#include <jack/types.h>

#include "../definitions.h"
#include "../sched.h"
#include "../dsp/simple.h"
#include "../error.h"
#include "driver_utils.h"
#include "jack_functions.h"
#include "ringbuffer.h"


static void err_cb(jack_status_t status)
{
    /* descriptions are straight from the jack docs*/
    if (status & JackServerStarted)
        bmo_err("The JACK server was started\n");
    if (status & JackFailure)
        bmo_err("Overall operation failed. \n");
    if (status & JackInvalidOption)
        bmo_err("The operation contained an invalid or unsupported option.\n");
    if (status & JackNameNotUnique)
        bmo_err("The request client name was not unique.\n");
    if (status & JackServerFailed)
        bmo_err("Unable to connect to the JACK server.\n");
    if (status & JackServerError)
        bmo_err("Communication error with the JACK server.\n");
    if (status & JackNoSuchClient)
        bmo_err("Requested client does not exist.\n");
    if (status & JackLoadFailure)
        bmo_err("Unable to load internal client.\n");
    if (status & JackInitFailure)
        bmo_err("Unable to initialize client.\n");
    if (status & JackShmFailure)
        bmo_err("Unable to access shared memory.\n");
    if (status & JackVersionError)
        bmo_err("Client's protocol version does not match.\n");
    if (status & JackBackendError)
        bmo_err("Jack Backend Error.\n");
    if (status & JackClientZombie)
        bmo_err("Braaaains...\n");
    else
        bmo_err("an unknown Jack error occurred \n");
}

static void finished_cb(void *arg)
{
    (void)arg;
    bmo_info("Jack Stream Completed\n");
    bmo_jack_stop(arg);
}

static int xrun_cb(void *arg)
{
    BMO_state_t *state = arg;
    float delay = jack_get_xrun_delayed_usecs(state->driver.jack.client);
    bmo_err("XRUN occurred of %fusecs. Consider increasing the buffer size.\n",
            delay);

    return 0;
}

static int bufsize_changed_cb(jack_nframes_t frames, void *arg)
{
    (void)arg;
    bmo_err("jack buffersize has been set at %d frames/call\n", frames);
    return 0;
}

static int sr_change_cb(jack_nframes_t src, void *arg)
{
    (void)arg;
    bmo_err("Jack Samplerate has changed to %d. "
            "Will attempt to update the output, but expect weirdness.\n",
            src);
    return 0;
}

static int process_cb(jack_nframes_t frames, void *arg)
{
    assert(arg);
    uint32_t i, j;
    i = j = 0;

    BMO_state_t *state = arg;
    jack_default_audio_sample_t *output_ports[bmo_playback_count(state)];
    jack_port_t **regd_out_ports = state->driver.jack.output_ports;
    while (i < state->n_playback_ch) {
        jack_port_t *port = regd_out_ports[i];
        if (port)
            if (jack_port_connected(port) > 0) {
                output_ports[j] = jack_port_get_buffer(port, frames);
                if (!output_ports[j]) {
                    bmo_err("out port %d is null\n");
                    break;
                }
                bmo_zero_sb(output_ports[j], frames);
                j++;
            }
        i++;
    }
    if (i == bmo_playback_count(state)) {
        uint32_t frames_read = bmo_read_rb(state->ringbuffer, output_ports, frames);
        if (frames_read < frames) {
            bmo_err("An underrun of %u samples occurred\n");
        }
        state->dsp_load = (float)jack_cpu_load(state->driver.jack.client);
        bmo_driver_callback_done(state, BMO_DSP_RUNNING);
        return 0;
    } else
        bmo_err("jack/client output ports mismatch:%d channels available\n", i);
    bmo_driver_callback_done(state, BMO_DSP_STOPPED);
    return -1;
}

void bmo_jack_stop(BMO_state_t *state)
{
    uint32_t i = 0;
    bmo_stop(state);
    jack_client_t *client = state->driver.jack.client;
    while (state->driver.jack.output_ports[i]) {
        jack_port_unregister(client, state->driver.jack.output_ports[i]);
        state->driver.jack.output_ports[i] = NULL;
        i++;
    }
    // TODO NULLify ports list etc.
    jack_client_close(client);
}

BMO_state_t *bmo_jack_start(
    BMO_state_t *state, uint32_t channels,
    uint32_t rate, uint32_t buf_len, uint32_t flags,
    const char *client_name
)
{
    (void)flags; // FIXME unused
    uint32_t i = 0;

    if (!state) {
        bmo_err("bmo_jack_start:couldn't get state\n");
        return NULL;
    }

    state->driver_id = BMO_JACK_DRIVER;
    state->driver.jack.ports = NULL;
    state->driver.jack.server_name = NULL;
    state->driver.jack.client_name = NULL;
    state->driver.jack.client = NULL;
    state->driver.jack.options = JackNullOption;
    while (i < bmo_playback_count(state)) {
        state->driver.jack.output_ports[i] = NULL;
        state->driver.jack.input_ports[i] = NULL;
        i++;
    }

    /* set client name*/
    if (!client_name) {
        client_name = "BMO_client";
    }
    state->driver.jack.client_name = client_name;
    bmo_debug("Client name is '%s'\n", client_name);

    /* connect to jack and setup return client identifier */
    state->driver.jack.client = jack_client_open(
        client_name,
        state->driver.jack.options,
        &state->driver.jack.status,
        state->driver.jack.server_name
    );
    jack_client_t *client = state->driver.jack.client;
    if (!client) {
        err_cb(state->driver.jack.status);
        return NULL;
    }

    state->driver.jack.client_name =
        jack_get_client_name(client);

    /* register the processing, error, and event-notification callbacks */
    jack_set_process_callback(client, process_cb, state);
    jack_on_shutdown(client, finished_cb, state);
    jack_set_xrun_callback(client, xrun_cb, state);
    jack_set_sample_rate_callback(client, sr_change_cb, state);
    jack_set_buffer_size_callback(client, bufsize_changed_cb, state);

    /* get and set buffer_size in running jack instance */
    state->buffer_size = jack_get_buffer_size(client);
    if (state->buffer_size != buf_len) {
        bmo_info("requested buf_len does not match jack's buf_len.\n");
    }

    /* initialise the ringbuffer if not already done */
    if (!state->ringbuffer) {
        state->ringbuffer = bmo_init_rb(state->buffer_size, channels);
        if (!state->ringbuffer)
            jack_client_close(client);
        return NULL;
    }

    state->driver_rate = jack_get_sample_rate(client);
    if (state->driver_rate != rate) {
        bmo_info(
            "jack is running at %dHz. using this instead of requested %d\n",
            state->driver_rate, rate);
        // TODO something useful like resetting sample rate conversion etc
    }

    /* create client output ports of the requested number of channels */
    size_t portname_buf_len = jack_port_name_size(); // includes the trailing
                                                     // NUL
    char portname_buf[portname_buf_len];
    // Jack docs just says "This will be a constant", but in practice is plenty
    // big enough.
    assert(portname_buf_len > 24);

    for (i = 0; i < channels; i++) {
        int chars =
            snprintf(portname_buf, portname_buf_len, "channel %d", i + 1);
        if (chars < 0) {
            bmo_err("couldn't assign jack port names:'%s'\n");
            jack_client_close(state->driver.jack.client);
            return NULL;
        }
        if ((size_t)chars == portname_buf_len) {
            portname_buf[portname_buf_len - 1] = '\0';
        }
        state->driver.jack.output_ports[i] = jack_port_register(
            client,
            portname_buf,
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsOutput,
            0
        );
        if (!state->driver.jack.output_ports[i]) {
            bmo_err("no more Jack ports available\n");
            goto fail;
        }
    }
    bmo_debug("created %d client output ports\n", i);
    state->n_playback_ch = i;

    /*start jack processing this client*/
    if (jack_activate(client)) {
        bmo_err("Jack cannot activate client");
        goto fail;
    }

    /* query and connect client outputs to jack inputs*/
    state->driver.jack.ports = jack_get_ports(
        client,
        NULL,
        NULL,
        JackPortIsPhysical | JackPortIsInput
    );
    if (!state->driver.jack.ports) {
        bmo_err("Couldn't get any ports into jack server\n");
        goto fail;
    }

    for (i = 0; state->driver.jack.ports[i]; i++)
        ;

    bmo_debug("Jack has %d physical input ports\n", i);
    if (!i) {
        bmo_err("Jack has no input ports available\n");
        goto fail;
    }
    state->n_playback_ch = i;

    /* connect all client ports to the Jack server */
    i = 0;
    while (state->driver.jack.output_ports[i]) {
        jack_connect(
            client,
            jack_port_name(state->driver.jack.output_ports[i]),
            state->driver.jack.ports[i % state->n_playback_ch]
        );
        i++;
    }
    bmo_debug("Connected %d ports to physical inputs on the jack server\n", i);

    if (i < channels) {
        bmo_info(
            "Couldn't connect all output ports to independent physical outputs;"
            "will connect all extra ports between existing %d channels\n",
            i
        );
        i = 0;
        bmo_debug("jack has these system playback ports available:\n");
        while (state->driver.jack.ports[i]) {
            bmo_debug("%s\n", state->driver.jack.ports[i]);
            i++;
        }
    }

    return state;
fail:
    bmo_jack_stop(state);
    return NULL;
}
#endif
