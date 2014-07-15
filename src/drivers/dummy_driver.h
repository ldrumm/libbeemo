#include "../definitions.h"
#ifndef BMO_DUMMY_DRIVER_H
#define BMO_DUMMY_DRIVER_H
BMO_state_t * bmo_dummy_start(BMO_state_t * params,  uint32_t channels, uint32_t rate, uint32_t buf_len, uint32_t flags, const char * path);
#endif 
