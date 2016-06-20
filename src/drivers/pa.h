#ifndef PORTAUDIO_FUNCTIONS_H
#define PORTAUDIO_FUNCTIONS_H
#include <portaudio.h>
#include "../definitions.h"

BMO_state_t *bmo_pa_start(BMO_state_t *params, uint32_t channels, uint32_t rate,
                          uint32_t buf_len, uint32_t flags);
#endif
