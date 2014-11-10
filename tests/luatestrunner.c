#if defined(_WIN32)
    #include <windows.h>
    #define setenv(k, v, c) SetEnvironmentVariable((k), (v));
#else
    #define _POSIX_C_SOURCE 201112L
#endif
#include <assert.h>

#include <string.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../src/definitions.h"
#include "../src/error.h"
#include "../src/lua/lua.h"
#include "../src/error.h"
#include "lib/stopwatch.c"


#include <stdlib.h>
#define ITERATIONS 10
#define FRAMES 1024
#define RATE 44100
#define CHANNELS 1
#define ENV_BUF_LEN 1024
int 
bmo_runtests(void * dsp_obj, char * test_file, const char * test_string, void * test_state)
{
    (void) test_state; //TODO define behaviour of extra parameters
    BMO_dsp_obj_t * dsp = (BMO_dsp_obj_t *) dsp_obj;
    bmo_debug("%s\n", test_file);
    BMO_lua_context_t * state = (BMO_lua_context_t *)dsp->handle;
    assert(state);
    int err = 0;
    if(test_file){
        err = luaL_dofile(state->L, test_file);
        if(err){
            goto fail;
        }
    }
    if(test_string){
	    err = luaL_loadstring(state->L, test_string);
	    if(err || lua_pcall(state->L, 0, 1, 0)){
	        goto fail;
	    }
//	    printf("pcall returned %d argument(s)\n", lua_gettop(state->L));
	    err = luaL_checkinteger(state->L, -1);
//	    printf("tests returned %d\n", err);
	    return err;
	}
	return 0;
fail:
		bmo_err("could not run tests:%s\n", lua_tostring(state->L, -1));
//		free(state); //leave the state in place for debugging.
//		lua_close(L);
		return -1;
}

int main(int argc, char **argv)
{
	bmo_verbosity(BMO_MESSAGE_DEBUG);
	int ret = 0;
	char * test_directory = NULL;
	char * test_string = NULL;
	char * replacement = NULL;
	char env_buf[ENV_BUF_LEN] = {'\0'};
	const char * test_proto = "return runtests(find_test_files({'$'}, 1, \"*.lua\"))";
	if(argc != 2){
		test_directory = "./lua";
	}
	else{
	    test_directory = argv[1];
	}
    test_string = calloc(strlen(test_directory) + strlen(test_proto) + 1, sizeof(char));
    if(!test_string){
        bmo_err("couldn't allocate string.");
        assert(0);
    }
    replacement = strchr(test_proto, '$');
    assert(replacement);
    strncpy(test_string, test_proto, replacement - test_proto);
    strcat(test_string, test_directory);
    strcat(test_string, replacement + 1);
	
	//some tests rely on sanity testing values by comparing against environment variables
	snprintf(env_buf, ENV_BUF_LEN, "%d", FRAMES);
	setenv("BMO_FRAMES", env_buf, 1);
	snprintf(env_buf, ENV_BUF_LEN, "%d", CHANNELS);
	setenv("BMO_CHANNELS", env_buf, 1);
	snprintf(env_buf, ENV_BUF_LEN, "%d", RATE);
	setenv("BMO_RATE", env_buf, 1);

	stopwatch_start();
	BMO_dsp_obj_t *dsp = bmo_dsp_lua_new(0, CHANNELS, FRAMES, RATE, NULL, 0);
	dsp->_init(dsp, 0);
	ret = bmo_runtests(dsp, "testingunit.lua", test_string, NULL);
	bmo_info("tests ran in %fs\n", stopwatch_stop());
	if(ret != 0){
		bmo_err("test failure:%d\n", ret);
	}
	dsp->_close(dsp, 0);
	free(dsp);
	free(test_string);
	return ret;
}
