#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#include <lauxlib.h>
#include <lua.h>

#include "../buffer.h"
#include "../definitions.h"
#include "../error.h"
#include "../import_export.h"
#include "../util.h"


#ifdef DEF_BUT_NOT_USED
static int _bmo_lua_fopen_base(lua_State *L)
{
    size_t len;
    const char *static_path = luaL_checklstring(L, 1, &len);
    char *path;
    BMO_buffer_obj_t *buf;

    if (!static_path || !len) {
        bmo_debug("Failed to open audio file\n");
        return luaL_error(L, "argument error; expected filename");
    }
    path = bmo_strdup(static_path);
    buf = bmo_fopen(path, BMO_MAPPED_FILE_DATA);
    if (!buf) {
        bmo_debug("failed to open %s\n", path);
        free(path);
        return luaL_error(L, "failed to open file");
    }
    free(path);
    lua_pushlightuserdata(L, buf);
    return 1;
}
#endif

static int _bmo_lua_fopen(lua_State *L)
{
    size_t len;
    const char *static_path = luaL_checklstring(L, 1, &len);
    char *path;
    BMO_buffer_obj_t *buf;
    if (!static_path || !len) {
        bmo_debug("Lua code failed to open audio file\n");
        return luaL_error(L, "argument error; expected filename");
    }
    path = bmo_strdup(static_path);
    buf = bmo_fopen(path, BMO_BUFFERED_DATA);
    if (!buf) {
        free(path);
        return luaL_error(L, "couldn't open file %s\n", bmo_strerror());
        return 0;
    }
    lua_createtable(L, (int)buf->channels, 0);
    BMO_NOT_IMPLEMENTED;
}

static const struct luaL_Reg import_builtins[] = {
    {"open_audio", _bmo_lua_fopen}, {NULL, NULL}
    // Add new import functions here
};

void _bmo_lua_reg_fio(lua_State *L, const char *name)
{
    luaL_newlib(L, import_builtins);
    lua_setglobal(L, name);
}
