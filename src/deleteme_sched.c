#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "definitions.h"
#include "error.h"
#include "buffer.h"
#include "drivers/ringbuffer.h"
#include "dsp/simple.h"
#define BUF 8192

#ifdef _WIN32
#include <io.h>
#define pipe(x) _pipe((x), BMO_DEFAULT_BUF, _O_BINARY)
#endif



double phase = 90.;
float pitch = 1000.;
#define bmo_driver_callback_fail(state) bmo_driver_callback_done((state), BMO_DSP_STOPPED)
#define bmo_driver_callback_success(state) bmo_driver_callback_done((state), BMO_DSP_RUNNING)

uint32_t bmo_wait_update(BMO_state_t* state)
{
    /** bmo_wait_update() blocks until the realtime callback (audio driver) completes and signals via IPC.
        This is implemented as a write/read pipe, and this just wraps that.
        @return: -1 on error. 0 on success
    */
    uint32_t msg;
    ssize_t status = read(state->ipc.driver_pipefd[0], &msg, sizeof(msg));
    if (status == 0){
        bmo_info("engine terminated\n");
        return BMO_DSP_STOPPED;
    }
    if(status < 0){
        bmo_err("could not read from pipe:%s", strerror(errno));
        return BMO_DSP_STOPPED;
    }
    return BMO_DSP_STOPPED;
}

int bmo_init_ipc(BMO_state_t * state)
{
    if(!pipe(state->ipc.driver_pipefd) == 0){
        bmo_err("ipc setup failed - couldn't create pipe.:%s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static void 
garbage(float ** buf, uint32_t len, uint32_t channels)
{
    bmo_zero_mb(buf, channels, len);
    for(uint32_t i = 0; i<channels;i++){
        buf[i][0] = rand() / (float)RAND_MAX;    
        buf[i][len/2] = rand() / (float)RAND_MAX;           
        bmo_osc_sine_mix_sb(buf[i], pitch, phase, 1., 44100, len);
//        bmo_osc_saw_sb(buf[i], pitch, phase, 10., 44100, len);
//        bmo_osc_sq_mix_sb(buf[i], pitch, phase, 10., 44100, len);
    }
    pitch += (pitch * 0.5);
}

int 
bmo_process_graph(BMO_state_t * state, int count)
{
    float ** tmp = bmo_mb_new(2, BUF);
    assert(tmp);
    int i =0;
    while(bmo_wait_update(state) != BMO_DSP_STOPPED){
//        bmo_err("upating process graph; CPU load is %f\n", state->dsp_load);
        garbage(tmp, BUF, 2);
        bmo_write_rb(state->ringbuffer, tmp, BUF);
        i++;
        if(i == count){
            break;
        }
    }
    bmo_mb_free(tmp, 2);
    bmo_info("engine terminated\n");
    return 0;
}

int bmo_driver_callback_done(BMO_state_t * state, uint32_t status)
{
    uint32_t  status_= status;
    ssize_t written = write(state->ipc.driver_pipefd[1], &status_, sizeof(status_));
    if(written != sizeof(status)){
        bmo_err("couldn't write to pipe\n");
        return -1;
    }
    return 0;
}


