#ifdef BMO_HAVE_PORTAUDIO
#include "../../src/definitions.h"
#include "../../src/deleteme_sched.h"
#include "../../src/drivers/driver_utils.h"
#include "../../src/drivers/drivers.h"
#include "../../src/error.h"
#include <portaudio.h>

#define CHANNELS 2
#define FRAMES 2048
#define RATE 48000
#define FREQ 200

double phase = 0;

void * start(void * arg)
{
    bmo_pa_start(arg, CHANNELS, RATE, FRAMES, 0);
    return NULL;
}

int main(void)
{
    bmo_verbosity(BMO_MESSAGE_DEBUG);
    BMO_state_t * state = bmo_new_state();
/*    pthread_t realtime_thread;*/
    bmo_init_ipc(state);
    bmo_info("ipc setup...\n");
    bmo_pa_start(state, CHANNELS, RATE, FRAMES, 0);
/*    pthread_create(&realtime_thread, NULL, start, state);*/
    bmo_info("started realtime thread\n");
    bmo_process_graph(state, 40);
    printf("calling PA_StOP\n");
    Pa_StopStream(state->driver.pa.stream);
    bmo_stop(state);
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
