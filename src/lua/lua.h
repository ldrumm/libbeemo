#ifndef BMO_LUA_DSP_H
#define BMO_LUA_DSP_H
#include "../definitions.h"

#ifdef BMO_HAVE_LUA
#include <lua.h>

typedef struct
{
    lua_State *L;
    const char *init_script;
    const char *user_script;
    size_t init_script_len;
    size_t user_script_len;
    int user_script_ref;    // Lua reference for the user's script file.
    int dspmain_ref;
    #ifndef NDEBUG
    lua_Hook *debug_hook;
    #endif
    lua_Alloc *allocator;
    ssize_t max_mem;
    BMO_dsp_obj_t *dsp;     // Pointer back to the parent object.
} BMO_lua_context_t;
#endif


#define BMO_DSP_METATABLE ("beemo.dsp")
#define BMO_BUFFER_METATABLE ("beemo.dsp.buffer")

#define CHECKBUFFER(L, idx) (luaL_checkudata((L), (idx), BMO_BUFFER_METATABLE))
#define CHECKDSP(L, idx) (luaL_checkudata((L), (idx), BMO_DSP_METATABLE)

BMO_dsp_obj_t *bmo_dsp_lua_new(
    uint32_t flags,
    uint32_t channels,
    uint32_t frames,
    uint32_t rate,
    const char * user_script,
    size_t script_len
);

#endif
