#ifndef BMO_DRIVERS_H
#define BMO_DRIVERS_H
#include "jack_functions.h"
#include "pa.h"

BMO_state_t *bmo_driver_init(BMO_state_t *state, uint32_t channels,
                             uint32_t rate, uint32_t buf_len, uint32_t flags,
                             void *data);
int bmo_driver_close(BMO_state_t *state);
#endif
