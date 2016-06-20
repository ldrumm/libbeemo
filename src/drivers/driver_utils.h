#include "../definitions.h"

#ifndef DRIVER_HELPER_FUNCTIONS_H
#define DRIVER_HELPER_FUNCTIONS_H
BMO_state_t *bmo_new_state(void);
uint32_t bmo_rate(BMO_state_t *params);
uint32_t bmo_playback_count(BMO_state_t *params);
uint32_t bmo_capture_count(BMO_state_t *params);
uint32_t bmo_bufsize(BMO_state_t *params);
uint32_t bmo_driver_ver(BMO_state_t *params);
uint32_t bmo_bufsize(BMO_state_t *params);
const char *bmo_strdriver(BMO_state_t *params);
void bmo_start(BMO_state_t *params);
void bmo_stop(BMO_state_t *params);
uint32_t bmo_status(BMO_state_t *params);
#endif

