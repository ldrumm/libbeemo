#ifndef BMO_ringbuffer_t_H
#define BMO_ringbuffer_t_H
#include "../definitions.h"
BMO_ringbuffer_t * bmo_init_rb(uint32_t len, uint32_t channels);
void bmo_rb_free(BMO_ringbuffer_t * rb);
uint32_t bmo_read_rb(BMO_ringbuffer_t * rb, float ** out, uint32_t frames);
uint32_t bmo_write_rb(BMO_ringbuffer_t * rb, float ** in, uint32_t frames);
#endif
