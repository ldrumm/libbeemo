#ifndef DELETEME_H
#define DELETEME_H
#include "definitions.h"
#define bmo_driver_callback_fail(state)                                        \
    bmo_driver_callback_done((state), BMO_DSP_STOPPED)
#define bmo_driver_callback_success(state)                                     \
    bmo_driver_callback_done((state), BMO_DSP_RUNNING)
int bmo_process_graph(BMO_state_t *state, BMO_dsp_obj_t *graph, size_t count);
int bmo_driver_callback_done(BMO_state_t *state, uint32_t code);
int bmo_init_ipc(BMO_state_t *state);
#endif
