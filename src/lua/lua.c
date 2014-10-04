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

#if 0
typedef struct
{
    const void * data;
    size_t len;
    size_t i;
}BMO_lua_reader_t;


static const char * 
_bmo_lua_reader_cb(lua_State *L, void *data, size_t *size)
{
    (void) L;
    BMO_lua_reader_t * reader = (BMO_lua_reader_t *) data;
    bmo_debug("reader->i==%zu\n", reader->i);
    *size = reader->len - reader->i;
    reader->i+=reader->len;
    return reader->data + reader->i;
}

static int 
_bmo_lua_loadbytecode(lua_State * L, const char * bytecode,  size_t len, const char * chunkname)
{
    BMO_lua_reader_t * reader = malloc(sizeof(BMO_lua_reader_t));
    if(!reader){
        bmo_err("couldn't compile Lua source:%s", strerror(errno));
        return -1;
    }
    reader->len = len;
    reader->i = 0;
    reader->data=bytecode;
    return lua_load(L, _bmo_lua_reader_cb, reader, chunkname, NULL);
}
#endif



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
	
	int status = luaL_dostring(L, state->init_script);
	if(status){
		bmo_err("could not create DSP object:%s\n", lua_tostring(L, -1));
		
		lua_close(L);
		free(state);
		return -1;
	}
	state->L = L;
	return 0;
}

static int
_bmo_dsp_update_lua(void * in, uint32_t flags)
{
	(void) flags;
	int status = 0;
	BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *) in;
	BMO_lua_context_t * state = (BMO_lua_context_t *)dsp->handle;
	if(!dsp->flags){
	    status = luaL_dostring(state->L, state->user_script);
	    if(status){
	        //If the user script fails to parse we should zero the output
	        bmo_err("user script failed to compile:%s\n", lua_tostring(state->L, -1));
	        bmo_zero_mb(dsp->out_buffers, dsp->channels, dsp->frames);
	    }
	    dsp->flags = 1;
	}
	status = luaL_dostring(state->L, "do dspmain() end");
	if(status){
		bmo_err("could not update DSP object:%s\n", lua_tostring(state->L, -1));
		lua_close(state->L);
		//not zeroing the buffers may cause earache on failure.
		bmo_zero_mb(dsp->out_buffers, dsp->channels, dsp->frames);
		return -1;
	}
	status = luaL_dostring(state->L, "\ndo dsp.schedule:update() end");
	if (status){
	    bmo_err("scheduler failed to run:%s\n", lua_tostring(state->L, -1));
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
bmo_lua_new(uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate, const char * user_script)
{
    #include "lua_default_text.h" //definesBMO_LUA_INIT_SCRIPT[];
	BMO_dsp_obj_t * dsp;
	size_t script_len = 0;
	char * script = NULL;
	if(user_script){
	    script_len = strlen(user_script)+1; //TODO support loading bytecode strings
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
	state->user_script_len = script_len; //TODO support bytecode loading
	dsp->type = BMO_DSP_OBJ_LUA;
	dsp->handle = state;
	dsp->_init = _bmo_dsp_init_lua;
	dsp->_update = _bmo_dsp_update_lua;
	dsp->_close = _bmo_dsp_close_lua;
	dsp->flags = flags;
	return dsp;
}

#endif
