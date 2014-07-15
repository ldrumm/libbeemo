#include <assert.h>
#ifdef BMO_HAVE_PORTAUDIO
#include <portaudio.h>
#include "pa.h"
#endif
#ifdef BMO_HAVE_JACK
#include "jack_functions.h"
#endif
#include "dummy_driver.h"
#include "../definitions.h"
#include "../error.h"


BMO_state_t * 
bmo_driver_init(BMO_state_t * state, uint32_t channels, uint32_t rate, uint32_t buf_len, uint32_t flags, void *data)
{
	if(!channels || !rate || !buf_len){	
		bmo_info("using defaults for audio driver connection\n");
		if(!channels)	channels = BMO_DEFAULT_CHANNELS;
		if(!rate)		rate = BMO_DEFAULT_RATE;
		if(!buf_len)	buf_len = BMO_DEFAULT_BUF;
		
		if(((flags & (BMO_PORTAUDIO_DRIVER | BMO_JACK_DRIVER)) == 0)){
			#ifdef BMO_HAVE_PORTAUDIO
			flags |= BMO_PORTAUDIO_DRIVER;
			#elif BMO_HAVE_JACK
			flags |= BMO_JACK_DRIVER;
			#else
			flags |= BMO_DUMMY_DRIVER;
			#endif
		}
	}
	switch(flags &(BMO_PORTAUDIO_DRIVER | BMO_JACK_DRIVER | BMO_DUMMY_DRIVER))
	{
		case BMO_PORTAUDIO_DRIVER: 	{
			#ifdef BMO_HAVE_PORTAUDIO
			bmo_debug("starting Portaudio driver with rate=%d, channels=%d, buf_len=%d\n", \
			    rate, channels, buf_len);
			return bmo_pa_start(state, channels, rate, buf_len, flags);
			#else
			return NULL;
			#endif
		}
		case BMO_JACK_DRIVER:		{		
			#ifdef BMO_HAVE_JACK
			bmo_debug("starting Jack driver with client name '%s'"
			    "rate=%d, channels=%d, buf_len=%d", \
			     (const char *)data, rate, channels, buf_len);
			if(!data)
			    data = (void *)"BMO_client";
			return bmo_jack_start(state, channels, rate, buf_len, flags, (const char *)data);
			#else 
			return NULL;
			#endif
		}
		case BMO_DUMMY_DRIVER: {
		    bmo_debug("starting dummy driver with file name '%s'"
			    "rate=%d, channels=%d, buf_len=%d\n", \
			    (const char *)data, rate, channels, buf_len);
            return bmo_dummy_start(state,  channels, rate, buf_len, flags, (const char *)data);
		}
		default:assert(0);
	}
	return NULL;
}

int bmo_driver_close(BMO_state_t * state)
{
    switch(state->driver_id){
        #ifdef BMO_HAVE_PORTAUDIO
        case BMO_PORTAUDIO_DRIVER:
        ;
            PaError pa_err = Pa_CloseStream(state->driver.pa.stream);
            if(pa_err != paNoError){
                bmo_err("could not terminate portaudio stream:%s\n",  Pa_GetErrorText(pa_err));
                return -1;
            }
            Pa_Terminate();
            break;
        #endif
        #ifdef BMO_HAVE_JACK
        case BMO_JACK_DRIVER:
            ;
            int err = jack_client_close(state->driver.jack.client);
            if(err){
                return -1;
            }
            break;
        #endif
        default:{
            assert(0);
            bmo_err("cannot stop unkown driver:%d\n", state->driver_id);
            return -1;
        }
    }
    state->driver_id = 0;
    return 0;

}
