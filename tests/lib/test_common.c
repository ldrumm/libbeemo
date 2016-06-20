#ifndef BMO_TEST_COMMON_C
#define BMO_TEST_COMMON_C
#include <math.h>
#include <stdlib.h>
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
    if (!path) {
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static uint32_t CHANNELS = 2;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static uint32_t FRAMES = 256;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static uint32_t RATE = 44100;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static uint32_t DRIVER = 0;
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

void assert_fequal(float a, float b)
{
    static const double tolerance = 1e-15;
    const double diff = fabs(a) - fabs(b);
    if (diff > tolerance){
        fprintf(
            stderr,
            "assertion failed: %1.15g != %1.15g ±%g (%1.15g)\n",
            a, b, tolerance, diff
        );
    }
    assert((fabs(a) - fabs(b)) < tolerance);
}

void assert_multibuf_equal(float **a, float **b, uint32_t channels, uint32_t frames)
{
    for (uint32_t ch = 0; ch < channels; ch++)
        for (uint32_t f = 0; f < frames; f++)
            assert_fequal(a[ch][f], b[ch][f]);
}

static void _setup_logging(void)
{
    bmo_verbosity(BMO_MESSAGE_DEBUG);
    const char * env = getenv("LOGLEVEL");
    if (env) {
        if (strcasecmp(env, "INFO") == 0) {
            bmo_verbosity(BMO_MESSAGE_INFO);
        }
        if (strcasecmp(env, "DEBUG") == 0) {
            bmo_verbosity(BMO_MESSAGE_DEBUG);
        }
        if (strcasecmp(env, "ERROR") == 0) {
            bmo_verbosity(BMO_MESSAGE_CRIT);
        }
        if (strcasecmp(env, "NONE") == 0) {
            bmo_verbosity(BMO_MESSAGE_NONE);
        }
    }
}

static void _setup_driver(void)
{
    const char * env = getenv("DRIVER");
    if (env) {
        #ifdef BMO_HAVE_JACK
        if (strcasecmp(env, "JACK") == 0) {
            DRIVER = BMO_JACK_DRIVER;
        }
        #endif
        #ifdef BMO_HAVE_PORTAUDIO
        if (strcasecmp(env, "PORTAUDIO") == 0) {
            DRIVER = BMO_PORTAUDIO_DRIVER;
        }
        #endif
    }
}

void bmo_test_setup(void)
{
	#ifdef _WIN32
	winsetup();
	#endif

    const char *env;
    if ((env = getenv("CHANNELS"))) {
        CHANNELS = atoi(env);
        assert(CHANNELS > 0);
    }
    if ((env = getenv("RATE"))) {
        RATE = atoi(env);
        assert(RATE > 0);
    }
    if ((env = getenv("FRAMES"))) {
        FRAMES = atoi(env);
        assert(FRAMES > 0);
    }
    _setup_driver();
    _setup_logging();
}
#endif
