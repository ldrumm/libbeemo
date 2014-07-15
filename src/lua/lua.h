#ifndef BMO_LUA_DSP_H
#define BMO_LUA_DSP_H
#include "../definitions.h"

#ifdef BMO_HAVE_LUA
typedef struct 
{
	lua_State * L;
	const char * init_script;
	char * user_script;
	size_t init_script_len;
	size_t user_script_len;
	
} BMO_lua_context_t; 
#endif

BMO_dsp_obj_t *bmo_lua_new(uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate, const char * user_script);

int BMO_runtests(void * dsp_obj, char * test_file, const char * test_string, void * test_state);
#endif
