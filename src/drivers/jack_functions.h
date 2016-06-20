#ifndef BMO_JACK_FUNCTIONS_H
#define BMO_JACK_FUNCTIONS_H
#ifdef BMO_HAVE_JACK
#include <jack/jack.h>
#endif
#include "../definitions.h"

BMO_state_t *bmo_jack_start(BMO_state_t *state, uint32_t channels,
                            uint32_t rate, uint32_t buf_len, uint32_t flags,
                            const char *client_name);
void bmo_jack_stop(BMO_state_t *params);
#endif

