#include "definitions.h"
#ifndef BMO_BUFFER_OBJS_H
#define BMO_BUFFER_OBJS_H

float ** bmo_mb_new(uint32_t channels, uint32_t frames);
void bmo_mb_free( float ** buffer, uint32_t channels);
BMO_buffer_obj_t * bmo_bo_new(uint32_t flags, uint32_t channels, size_t frames,  uint32_t rate, size_t offset, size_t file_len, void * data);
BMO_buffer_obj_t * bmo_bo_alias(BMO_buffer_obj_t * obj);
BMO_buffer_obj_t * bmo_bo_cpy(BMO_buffer_obj_t * obj);
void bmo_bo_free(BMO_buffer_obj_t * obj);
#endif
