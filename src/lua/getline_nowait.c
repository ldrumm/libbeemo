#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>

static size_t my_getline(int fd, char * buf, size_t count)
{
    ssize_t len = read(fd, buf, count);
    if( len < 0){
        return 0;
    }
    return (size_t) len;
}

static int bmo_getline_nowait(lua_State * L)
{
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    char * s;
    size_t len = 0;
    int fd = STDIN_FILENO;
    #ifdef __unix__
    int oldflags = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, O_NONBLOCK|O_NDELAY);
    while(1){
        s = luaL_prepbuffer(&b);
        if(!s){
            printf("alloc fail\n");
            fcntl(fd, F_SETFL, oldflags);
            return luaL_error(L, "couldn't assign buffer for input.\n");
        }
        len = my_getline(fd, s,  LUAL_BUFFERSIZE);
        if(!(len > 1)){
            luaL_pushresult(&b);
            fcntl(fd, F_SETFL, oldflags);
            return 1;
        }

        luaL_addsize(&b, len);
    }
    #else
    BMO_NOT_IMPLEMENTED;
    #endif
}

static const struct luaL_Reg funcs[] = {
    {"getline", bmo_getline_nowait},
    {NULL, NULL}
};

void bmo_reg_getline_nowait(lua_State * L, const char *table_name)
{
    luaL_newlib(L, funcs);
	lua_setglobal(L, table_name);
}
