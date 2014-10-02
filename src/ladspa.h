#ifndef BMO_LADSPA_H
#define BMO_LADSPA_H
#include <ladspa.h>
#include "definitions.h"

typedef struct BMO_LADSPA_dsp_state
{
	LADSPA_Handle * lh;
	LADSPA_Descriptor * ld;
}BMO_LADSPA_dsp_state;

void bmo_ladspa_info(const LADSPA_Descriptor * ld);
BMO_dsp_obj_t * bmo_dsp_ladspa_new(const char * path, uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate);

#endif
