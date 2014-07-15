#ifdef BMO_HAVE_LADSPA
#include <assert.h>
#include <ladspa.h>
#include "dsp_obj.h"
#include "definitions.h"
#include "error.h"
#include "memory/map.h"
#include "ladspa.h"


void 
bmo_ladspa_info(const LADSPA_Descriptor * ld)
{
	LADSPA_Properties properties = ld->Properties;
	if(!LADSPA_IS_REALTIME(properties)){
		bmo_debug("LADSPA Plugin \'%s\' does not need support for realtime input.  \n", ld->Name);
	}
	if(LADSPA_IS_INPLACE_BROKEN(properties)){
		bmo_info("LADSPA Plugin \'%s\' does not support in place buffer writes.\n", ld->Name);
	}
	if(LADSPA_IS_HARD_RT_CAPABLE(properties)){
		bmo_debug("LADSPA Plugin \'%s\' claims to support Hard-Realtime limits\n", ld->Name);
	}
	bmo_debug("LADSPA Plugin \'%s\' has %lu ports available.\n", ld->Name, ld->PortCount);
	for(unsigned long i = 0; i < ld->PortCount; i++){
		bmo_debug("LADSPA Plugin \'%s\' port%lu:%s :: %s%s %s%s::%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s>=%1.1f<=%1.1f\n", \
			ld->Name, i, ld->PortNames[i],
			LADSPA_IS_PORT_AUDIO(ld->PortDescriptors[i])?"(audio)":"",
			LADSPA_IS_PORT_CONTROL(ld->PortDescriptors[i])?"(control)":"",
			LADSPA_IS_PORT_INPUT(ld->PortDescriptors[i])?"(input)":"",
			LADSPA_IS_PORT_OUTPUT(ld->PortDescriptors[i])?"(output)":"" ,
			LADSPA_IS_HINT_BOUNDED_BELOW(ld->PortRangeHints[i].HintDescriptor)?"(bounded below)":"",
			LADSPA_IS_HINT_BOUNDED_ABOVE(ld->PortRangeHints[i].HintDescriptor)?"(bounded above)":"",
			LADSPA_IS_HINT_TOGGLED(ld->PortRangeHints[i].HintDescriptor)?"(toggled)":"",
			LADSPA_IS_HINT_SAMPLE_RATE(ld->PortRangeHints[i].HintDescriptor)?"(samplerate)":"",
			LADSPA_IS_HINT_LOGARITHMIC(ld->PortRangeHints[i].HintDescriptor)?"(logarithmic)":"",
			LADSPA_IS_HINT_INTEGER(ld->PortRangeHints[i].HintDescriptor)?"(integer)":"",
			LADSPA_IS_HINT_HAS_DEFAULT(ld->PortRangeHints[i].HintDescriptor)?"(has default)":"",
			LADSPA_IS_HINT_DEFAULT_MINIMUM(ld->PortRangeHints[i].HintDescriptor)?"(has default min)":"",
			LADSPA_IS_HINT_DEFAULT_LOW(ld->PortRangeHints[i].HintDescriptor)?"(default low)":"",
			LADSPA_IS_HINT_DEFAULT_MIDDLE(ld->PortRangeHints[i].HintDescriptor)?"(default mid)":"",
			LADSPA_IS_HINT_DEFAULT_HIGH(ld->PortRangeHints[i].HintDescriptor)?"(default high)":"",
			LADSPA_IS_HINT_DEFAULT_MAXIMUM(ld->PortRangeHints[i].HintDescriptor)?"(default max)":"",
			LADSPA_IS_HINT_DEFAULT_0(ld->PortRangeHints[i].HintDescriptor)?"(default 0)":"",
			LADSPA_IS_HINT_DEFAULT_1(ld->PortRangeHints[i].HintDescriptor)?"(default 1)":"",
			LADSPA_IS_HINT_DEFAULT_100(ld->PortRangeHints[i].HintDescriptor)?"(default 100 )":"",
			LADSPA_IS_HINT_DEFAULT_440(ld->PortRangeHints[i].HintDescriptor)?"(default 440)":"",
			ld->PortRangeHints[i].LowerBound,
			ld->PortRangeHints[i].UpperBound);
	}
	return;
}




static const LADSPA_Descriptor * 
bmo_load_ladspa(const char * path)
{
	LADSPA_Descriptor_Function ld;
	ld = bmo_loadlib(path, "ladspa_descriptor");
	
	if(ld){
		bmo_debug("loaded %s at %p\n", path, ld);
		return ld(0);
	}
	else{
		bmo_debug("invalid or corrupt ladspa plugin %s\n", path);
		return NULL;
	}
}


static int 
_bmo_update_dsp_ladspa(void * in, uint32_t flags)
{
	assert(in);
	assert(flags);
	BMO_dsp_obj_t * dsp = in;
	BMO_LADSPA_dsp_state * state = (BMO_LADSPA_dsp_state *)dsp->handle;
	state->ld->run(state->lh, flags);
	bmo_debug("\n");
	return 0;
}


static int 
_bmo_init_dsp_ladspa(void * in, uint32_t flags)
{
	(void) in;
	(void) flags;
	return -1;
}


static int 
_bmo_close_dsp_ladspa(void * in, uint32_t flags)
{
	(void) in;
	(void) flags;
	return -1;
}


BMO_dsp_obj_t *
bmo_ladspa_new(uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate, const char * path)
{
	(void) flags;
	const LADSPA_Descriptor * ld = NULL;
	LADSPA_Handle * lh = NULL;
	BMO_dsp_obj_t * dsp = bmo_dsp_new(flags, channels, frames, rate);
	if(!dsp)
		return NULL;
	dsp->type = BMO_DSP_OBJ_LADSPA; //FIXME
	dsp->handle = calloc(1, sizeof(BMO_LADSPA_dsp_state));
	if(!dsp->handle)
		return NULL;
		
	ld = bmo_load_ladspa(path);
	lh = ld->instantiate(ld, rate);
	
	if(!ld)
		return NULL;
	if(!lh)
		return NULL;

	BMO_LADSPA_dsp_state * state = dsp->handle;

	//connect ports/register with plugin etc in a dumb way TODO make smart guy be.
	unsigned j = 0;
	for(unsigned i = 0; i < ld->PortCount; i++){
		if(LADSPA_IS_PORT_AUDIO(ld->PortDescriptors[i]) && LADSPA_IS_PORT_INPUT(ld->PortDescriptors[i]) ){
			ld->connect_port(lh, i, dsp->in_buffers[j]);
			j++;
		}
	}
	bmo_debug("connected %d input ports\n", j);
	j = 0;
	for(unsigned i = 0; i < ld->PortCount; i++){
		if(LADSPA_IS_PORT_AUDIO(ld->PortDescriptors[i]) && LADSPA_IS_PORT_OUTPUT(ld->PortDescriptors[i]) ){
			ld->connect_port(lh, i, dsp->out_buffers[j]);
			j++;
		}
	}
	dsp->_init = _bmo_init_dsp_ladspa;
	dsp->_update = _bmo_update_dsp_ladspa;
	dsp->_close = _bmo_close_dsp_ladspa;
	ld->activate(lh);
	state->lh = lh;
	state->ld = (void *) ld;
	bmo_debug("connected %d output ports\n", j);

	return dsp;
}
#endif
