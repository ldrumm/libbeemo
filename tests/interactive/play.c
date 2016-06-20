#include <stdlib.h>

#include "../../src/definitions.h"
#include "../../src/sched.h"
#include "../../src/drivers/drivers.h"
#include "../../src/drivers/driver_utils.h"
#include "../../src/error.h"
#include "../../src/lua/lua.h"
#include "../../src/drivers/ringbuffer.h"
#include "../../src/graph.h"
#include "../../src/dsp_obj.h"
#include "../../src/import_export.h"

#include "../lib/test_common.c"

int main(int argc, char ** argv)
{
    bmo_test_setup();
    if (argc < 2) {
        bmo_err("missing required argument (path to audiofile)\n");
        exit(EXIT_FAILURE);
    }
    const char *path = argv[1];

    BMO_state_t *state = bmo_new_state();
    state->ringbuffer = bmo_init_rb(FRAMES, CHANNELS);
    BMO_dsp_obj_t *file = bmo_dsp_bo_new_fopen(path, BMO_BUFFERED_DATA, FRAMES);
    BMO_dsp_obj_t *rb = bmo_dsp_rb_new(
        state->ringbuffer,
        BMO_DSP_TYPE_OUTPUT,
        CHANNELS,
        FRAMES,
        RATE
    );
    bmo_dsp_connect(file, rb, 0);
    bmo_init_ipc(state);
    bmo_start(state);
    bmo_driver_init(state, CHANNELS, RATE, FRAMES, DRIVER, NULL);
    bmo_process_graph(state, rb, 0);
    bmo_driver_close(state);

    exit(EXIT_SUCCESS);
}
