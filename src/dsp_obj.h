#ifndef BMO_DSP_OBJ_H
#define BMO_DSP_OBJ_H
#include "definitions.h"
BMO_dsp_obj_t * 
bmo_dsp_new(uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate);
void 
bmo_dsp_close(BMO_dsp_obj_t * dsp);
uint64_t bmo_uid(void);
#endif
