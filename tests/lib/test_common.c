#ifndef BMO_TEST_COMMON_C
#define BMO_TEST_COMMON_C
#include <math.h>
#include <strings.h>

#include "../../src/error.h"
#include "../../src/definitions.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#ifndef _DEBUG
#define _DEBUG
#endif
#ifdef _WIN32

#include <stdio.h>
#include <fcntl.h>
int mkstemp(char *template)
{
    char *path = mktemp(template);
    if (!path){
        return -1;
    }
    return open(path, O_RDWR|O_CREAT, 0600);
}

/*
When an assertion fails on windows, an annoying dialogue box pops up to inform the user that:
"   
    XXX has stopped working

    A problem caused the program to stop working correctly.  
    Windows will close the program and notify you if a solution is available.
"
It then waits a while as it hits the network to find out if Microsoft know how to fix it.
We're testing software.  We called assert. Avoid all that.

 */

int report_hook(int type, char *message, int *ret_val)
{
    (void)type;
	fprintf(stderr, "ASSERTION FAILURE:%s", message);
	return *ret_val;
}



void winsetup(void)
{
    // TODO The above is moot.  MinGW does not have the correct definition
    //	_CrtSetReportHook(report_hook);
}

#endif

static uint32_t CHANNELS = 1;
static uint32_t FRAMES = 512;
static uint32_t RATE = 44100;
static uint32_t DRIVER = 0;

void assert_fequal(float a, float b)
{

    assert(fabs(a) - fabs(b) < 1e-15);
    (void) a;
    (void) b;
}

void assert_multibuf_equal(float **a, float **b, uint32_t channels, uint32_t frames)
{
    for(uint32_t ch=0; ch < channels; ch++)
        for(uint32_t f=0; f < frames; f++)
            assert_fequal(a[ch][f], b[ch][f]);
}


static void _setup_logging(void)
{
    bmo_verbosity(BMO_MESSAGE_DEBUG);
    const char * env = getenv("LOGGING");
    if(env){
        if(strcasecmp(env, "INFO")==0){
            bmo_verbosity(BMO_MESSAGE_INFO);
        }
        if(strcasecmp(env, "DEBUG")==0){
            bmo_verbosity(BMO_MESSAGE_DEBUG);
        }
        if(strcasecmp(env, "ERROR")==0){
            bmo_verbosity(BMO_MESSAGE_CRIT);
        }
        if(strcasecmp(env, "NONE")==0){
            bmo_verbosity(BMO_MESSAGE_NONE);
        }
    }
}

static void _setup_driver(void)
{
    #ifdef BMO_HAVE_JACK
    DRIVER = getenv("JACK") ? BMO_JACK_DRIVER : 0;
    #endif
    #ifdef BMO_HAVE_PORTAUDIO
    DRIVER = getenv("PORTAUDIO") ? BMO_PORTAUDIO_DRIVER : DRIVER;
    #endif
}

void bmo_test_setup(void)
{
	#ifdef _WIN32
	winsetup();
	#endif

	const char * env;
	env = getenv("CHANNELS");
	if(env){
		CHANNELS = atoi(env);
		assert(CHANNELS > 0);
	}
	env = getenv("RATE");
	if(env){
		RATE = atoi(env);
		assert(RATE > 0);
	}
	env = getenv("FRAMES");
	if(env){
		FRAMES = atoi(env);
		assert(FRAMES > 0);
	}
	_setup_driver();
	_setup_logging();
}

#endif