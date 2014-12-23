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
#include "graph.h"

#ifdef _WIN32
#include <io.h>
#define pipe(x) _pipe((x), BMO_DEFAULT_BUF, _O_BINARY)
#endif
#define bmo_driver_callback_fail(state) bmo_driver_callback_done((state), BMO_DSP_STOPPED)
#define bmo_driver_callback_success(state) bmo_driver_callback_done((state), BMO_DSP_RUNNING)

uint32_t
bmo_wait_ipc_driver(BMO_state_t* state)
{
    /** Blocks until the realtime callback completes and signals via IPC.
        This can be implemented as a write/read pipe or blocking on a semaphore etc.
        and this just wraps that platform specific functionality.
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
    return 0;
}

int bmo_init_ipc(BMO_state_t * state)
{
    if(!pipe(state->ipc.driver_pipefd) == 0){
        bmo_err("ipc setup failed - couldn't create pipe.:%s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int 
bmo_process_graph(BMO_state_t * state, BMO_dsp_obj_t * graph, size_t count)
{
    size_t inc = count ? 1 : 0;
    count = count ? count : 1;
    while(bmo_wait_ipc_driver(state) != BMO_DSP_STOPPED && count){
//        bmo_debug("upating process graph; CPU load is %f\n", state->dsp_load);
        bmo_update_dsp_tree(graph, state->n_ticks, 0);
        state->n_ticks++;
        count -= inc;
    }
    bmo_info("engine terminated\n");
    return 0;
}

int bmo_driver_callback_done(BMO_state_t * state, uint32_t status)
{
    ssize_t written = write(state->ipc.driver_pipefd[1], &status, sizeof(status));
    if(written != sizeof(status)){
        bmo_err("couldn't write to pipe\n");
        return -1;
    }
    return 0;
}


