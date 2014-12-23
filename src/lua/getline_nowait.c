#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#ifdef _WIN32
#include <conio.h>
#endif
#include "../definitions.h"
#include "../error.h"
#include "../util.h"
#define BMO_TEXT_BUF 8192
#if !defined(MIN)
#define MIN(x, y) ((x) < (y) ? (x):(y))
#endif


static int block_closed(lua_State *L, int status) {
    // This seems to be about the only hackish thing in Lua.
    // We have to parse the errorstring for an "<eof>" marker to check if
    // the string ended prematurely, and continue only in that case.
    // Otherwise, the error message will never reference EOF
    if(status == LUA_ERRSYNTAX) {
        size_t len;
        const char *errstring = lua_tolstring(L, -1, &len);
        if(strstr(errstring, "<eof>") == errstring + len - 5){
            lua_pop(L, -1); //pop the error message, because we've still got work to do
            return 0;
        }
    }
    return 1; // The block is closed
}


static const char * current_buff = NULL;;
static size_t try_getline(char * buf, size_t count)
{
    #define PRINTABLE(x) (((x) > 32) && ((x) < 126))    //Sorry people of the world...
    // This function makes some pretty big sacrifices on performance and clarity to be able to
    // query the console without blocking.  This involves a lot of syscalls, but as 
    // it is intended for debug a repl, it won't affect the general performance of the
    // lib too much.  win32 doesn't provide a way to query if the console has a line avaiulable
    // without bringing in a lot of crud, so we use the most basic of state machines and ignore
    // non-ascii scripts.
    #ifdef __unix__
    ssize_t len;
    int oldflags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK|O_NDELAY);
    len = read(STDIN_FILENO, buf, count);
    fcntl(STDIN_FILENO, F_SETFL, oldflags);
    if( len < 0){
        return 0;
    }
    count = len;

    #elif _WIN32
    static char line_buf[BMO_TEXT_BUF];
    static size_t n_read;
    while(_kbhit()){
        int ch = _getch();
        if(PRINTABLE(ch)){
            _putch(ch);     
        }
        if(n_read >= count || n_read >= BMO_TEXT_BUF){
            bmo_err("line too long for buffer.  Input line ignored.");
            memset(line_buf, 0, BMO_TEXT_BUF);
            n_read = 0;
            return 0;
        }
        if(ch != '\b'){
            line_buf[n_read] = ch;
            n_read++;
            count = n_read;
        }
        else{
            if(n_read){
                n_read--;
                _putch('\b');
                _putch(' ');
                _putch('\b');
                line_buf[n_read] = '\0';
            } 
            return 0;
        }

        if(ch == '\n' || ch == '\r'){
            //for some reason _newlines aren't properly echoed by _putch()
            _putch('\n');
            //we don't NUL terminate because getline_nowait is expected to handle
            //binary strings and take this precaution itself.
            strncpy(buf, line_buf, n_read -1);
            memset(line_buf, 0, BMO_TEXT_BUF);
            n_read = 0;
            return count;
        }
    }
    return 0;

    #else
    (void)buf;
    (void)count;
    BMO_NOT_IMPLEMENTED;
    #endif

    return count;
}

static int getline_nowait(lua_State * L, int * loaded)
{
    char buf[BMO_TEXT_BUF];
    memset(buf, '\0', BMO_TEXT_BUF);
    lua_settop(L, 0);
    size_t len;
    if(!current_buff){
        current_buff = "";
    }
    int status;
    while(1){
        len = try_getline(buf, BMO_TEXT_BUF);
        if(!len){
            *loaded = 0;
            return 0;
        }
        *loaded = 1;
        buf[len < BMO_TEXT_BUF ? len : BMO_TEXT_BUF - 1] = '\0';

        lua_pushstring(L, current_buff);
        lua_pushstring(L, buf);
        lua_concat(L, 2);
        current_buff = bmo_strdup(lua_tostring(L, -1));
        status =  luaL_loadbuffer(L, current_buff, strlen(current_buff), "@repl");
        return status;
    }
    return 0;
}

static int tryinterpret(lua_State *L)
{
    int loaded_line;
    int status = getline_nowait(L, &loaded_line);

    if(!loaded_line){
        return 0;
    }

    if(block_closed(L, status)){
        if(lua_type(L, -1) == LUA_TFUNCTION){
            status = lua_pcall(L, 0, 0, 0);
        }
        if(status){
            fprintf(stderr, "error:%s\n", lua_tostring(L, -1));
        }
        free((void *)current_buff);
        current_buff = NULL;
        printf("[~]");
    }else {
        printf(">> ");
    }
    lua_settop(L, 0);
    fflush(stdout);
    return status;
}


static int repl_nowait(lua_State *L){
    if(!isatty(STDIN_FILENO)){  //EOFs are currently ignored.
        return luaL_error(L, "not a typewriter");
    }
    tryinterpret(L);
    lua_settop(L, 0);
    return 0;
}

static const struct luaL_Reg funcs[] = {
    {"repl", repl_nowait},
    {NULL, NULL}
};

void bmo_lua_reg_getline_nowait(lua_State * L, const char *table_name)
{
    luaL_newlib(L, funcs);
	lua_setglobal(L, table_name);
}
