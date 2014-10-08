#ifdef BMO_HAVE_LUA
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>

#include "lua_builtins.h"
#include "lua.h"
#include "import.h"

#include "../dsp_obj.h"
#include "../definitions.h"
#include "../error.h"
#include "../memory/map.h"
#include "../util.h"
#include "../dsp/simple.h"
#include "getline_nowait.h"

static int
_bmo_dsp_init_lua(void * in, uint32_t flags)
{
    const char * dsp_name = "dsp";
	(void) flags;
	BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *) in;
	BMO_lua_context_t * state = (BMO_lua_context_t *)dsp->handle;
	lua_State * L = luaL_newstate();
	luaL_openlibs(L);
	_bmo_lua_reg_builtin(L, dsp, dsp_name, "_dsp");
	bmo_lua_reg_getline_nowait(L, "async");
	
	int status = luaL_loadbuffer(L,
	    state->init_script,
	    state->init_script_len,
	    "bmo_init_script"
	) || lua_pcall(L, 0, LUA_MULTRET, 0);
	if(status){
		goto err;
	}

	if(state->user_script){
	    status = luaL_loadbuffer(L,
	        state->user_script,
	        state->user_script_len,
	        "bmo_user_script"
	    );
        state->user_script_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        assert(state->user_script_ref != LUA_REFNIL && state->user_script_ref != LUA_NOREF);
        lua_rawgeti(L, LUA_REGISTRYINDEX, state->user_script_ref);
        lua_pcall(L, 0, LUA_MULTRET, 0);
	    if(status){
	        goto err;
	    }
    }
    status = luaL_loadstring(L, "do dspmain() end;do dsp.schedule:update() end");
    if(status){
		goto err;
    }
    state->dspmain_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	assert(state->dspmain_ref != LUA_REFNIL && state->dspmain_ref != LUA_NOREF);
	state->L = L;
	return 0;

err:
	bmo_err("could not create DSP object:%s\n", lua_tostring(L, -1));
	lua_close(L);
	free(state);
	return -1;
}

static int
_bmo_dsp_update_lua(void * in, uint32_t flags)
{
	(void) flags;
	BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *) in;
	BMO_lua_context_t * state = (BMO_lua_context_t *)dsp->handle;

	lua_rawgeti(state->L, LUA_REGISTRYINDEX, state->dspmain_ref);
	int status = lua_pcall(state->L, 0, LUA_MULTRET, 0);
	if(status){
	        bmo_err("user script failed:%s\n", lua_tostring(state->L, -1));
	        //not zeroing the buffers may cause earache on failure.
	        bmo_zero_mb(dsp->out_buffers, dsp->channels, dsp->frames);
		return -1;
	}
	return status;
}

static int
_bmo_dsp_close_lua(void * in, uint32_t flags)
{
	(void)flags;
	BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *) in;
	BMO_lua_context_t * state = (BMO_lua_context_t *)dsp->handle;
	lua_close(state->L);
	free(state->user_script);
	free(state);
	bmo_dsp_close(in);

	return 0;
}

BMO_dsp_obj_t *
bmo_dsp_lua_new(uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate, const char * user_script, size_t script_len)
{
    //defines BMO_LUA_INIT_SCRIPT[] and BMO_LUA_INIT_SCRIPT_LEN;
    #include "lua_default_text.h"
	BMO_dsp_obj_t * dsp;
	char * script = NULL;
	if(user_script){
	    if(!script_len){
	        script_len = strlen(user_script);
	    }
        script = bmo_strdupb(user_script, script_len);
        if(!script){
		    bmo_err("Lua script could not be loaded:%s\n", errno ? strerror(errno):"");
		    return NULL;
	    }
	}

	BMO_lua_context_t * state = malloc(sizeof(BMO_lua_context_t));
	if(!state){
		bmo_err("could not create lua state:%s\n", strerror(errno));
		return NULL;
	}

	dsp = bmo_dsp_new(flags, channels, frames, rate);
	if(!dsp){
		bmo_err("could not create new DSP object:\n");
		free(state);
		free(script);
		return NULL;
	}

	state->L = NULL;
	state->init_script = BMO_LUA_INIT_SCRIPT;
	state->init_script_len = BMO_LUA_INIT_SCRIPT_LEN;
	state->user_script = script;
	state->user_script_len = script_len;
	state->user_script_ref = LUA_NOREF;
	state->dspmain_ref = LUA_NOREF;
	dsp->type = BMO_DSP_OBJ_LUA;
	dsp->handle = state;
	dsp->_init = _bmo_dsp_init_lua;
	dsp->_update = _bmo_dsp_update_lua;
	dsp->_close = _bmo_dsp_close_lua;
	dsp->flags = flags;
	return dsp;
}

#endif
