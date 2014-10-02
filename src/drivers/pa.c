#ifdef BMO_HAVE_PORTAUDIO
#include <portaudio.h>
#include <stdlib.h>
#include <assert.h>
#include "../definitions.h"
#include "../error.h"
#include "../dsp/simple.h"
#include "ringbuffer.h"
#include "driver_utils.h"

#include "../deleteme_sched.h"

static void
_bmo_pa_finished_cb(void * user_data)
{
   	bmo_info("Portaudio:Stream Finished\n");
	bmo_stop(user_data);
}

static int 
_bmo_pa_process_cb(const void *ins, void *outs, unsigned long frames, const PaStreamCallbackTimeInfo * time_info, PaStreamCallbackFlags status_flags, void * data)
{	
	(void) ins;			//TODO
	(void)time_info; 	//TODO
	(void)status_flags; //TODO
	
	assert(frames < UINT32_MAX);
	BMO_state_t * state = (BMO_state_t *)data;
	if (bmo_status(state) == BMO_DSP_STOPPED){
        bmo_info("Audio process quit detected\n");
		bmo_zero_mb((float **)outs, state->n_playback_ch, frames);
		bmo_driver_callback_done(state, BMO_DSP_STOPPED);
		return paComplete;
	}
	else if (bmo_status(state) == BMO_DSP_RUNNING){
		bmo_read_rb(state->ringbuffer, (float **) outs, (uint32_t)frames);
	}
	state->dsp_load = (float)Pa_GetStreamCpuLoad(state->driver.pa.stream);
	bmo_driver_callback_done(state, BMO_DSP_RUNNING);
	return paContinue;
}


BMO_state_t *
bmo_pa_start(BMO_state_t * state, uint32_t channels, uint32_t rate, uint32_t buf_len, uint32_t flags)
{
	(void)flags; //TODO
	if(!state->ringbuffer){
	    state->ringbuffer = bmo_init_rb(buf_len, channels);
	}
	state->driver_id = BMO_PORTAUDIO_DRIVER;
	state->driver.pa.error_num = Pa_Initialize();
	
	if(state->driver.pa.error_num){
		bmo_err("Cannot initialise PortAudio:\n"
				"%s\n", Pa_GetErrorText(state->driver.pa.error_num));
		return NULL;
	}

    /*TODO should be enumerating available devices, and choosing best 
    option, rather than falling back on given defaults*/
	state->driver.pa.output_params.device = Pa_GetDefaultOutputDevice();
	if (state->driver.pa.output_params.device == paNoDevice){
		bmo_err("No default portaudio output device available.\n");
    }
    
    assert(((int64_t)channels) < INT_MAX); //portaudio uses a signed int here. we are passing an unisgned int
	state->driver.pa.output_params.channelCount = (int)channels;
	state->n_playback_ch = channels;
	state->driver.pa.output_params.sampleFormat = paFloat32 | paNonInterleaved;
	state->driver.pa.output_params.suggestedLatency = Pa_GetDeviceInfo(
	    state->driver.pa.output_params.device
	)->defaultLowOutputLatency;
	state->driver.pa.output_params.hostApiSpecificStreamInfo = NULL;

    /* Open an audio I/O stream. */
    bmo_debug("opening stream\n");
	state->driver.pa.error_num = Pa_OpenStream(
		&(state->driver.pa.stream),
		NULL, 	// input parameters aren't accounted for yet TODO
		&(state->driver.pa.output_params),
		(double)rate,
		(unsigned long)buf_len,
		paClipOff,
		_bmo_pa_process_cb,
		state
	);

	if(state->driver.pa.error_num != paNoError){
		bmo_err("couldn't open portaudio stream:"
			"%s\n", Pa_GetErrorText(state->driver.pa.error_num));
		return NULL;
    }
    
    /* set callbacks */
    bmo_debug("setting callbacks\n");
	state->driver.pa.error_num = Pa_SetStreamFinishedCallback(state->driver.pa.stream, _bmo_pa_finished_cb);

	if(state->driver.pa.error_num != paNoError){
    	bmo_err("couldn't set exit conditions for portaudio stream:"
    		"%s\n", Pa_GetErrorText(state->driver.pa.error_num));
    	return NULL;
    }

	/*  */
	bmo_debug("starting stream\n");
	state->driver.pa.error_num = Pa_StartStream(state->driver.pa.stream);
    if(state->driver.pa.error_num != paNoError){
    	bmo_err("couldn't start portaudio stream:"
    	    "%s\n", Pa_GetErrorText(state->driver.pa.error_num));
    	return NULL;
    }
	return state;
}
#endif
