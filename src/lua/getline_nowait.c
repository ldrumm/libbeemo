#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#ifdef _WIN32
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include <conio.h>
#else
#include <fcntl.h>
#endif

#include <lua.h>
#include <lauxlib.h>

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
    // This function makes some pretty big sacrifices on performance and clarity to be able to
    // query the console without blocking.  This involves a lot of syscalls, but as 
    // it is intended for debug a repl, it won't affect the general performance of the
    // lib too much.  win32 doesn't provide a way to query if the console has a line available
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
    #define UTF8CP 65001 //UTF-8
    static wint_t line_buf[BMO_TEXT_BUF];
    static int n_read;
    while(_kbhit()){
        wint_t ch = _getwch();
        //avoid control sequences ending up in the buffer
        if(ch == 0 || ch == 0xe0){
            //above indicates first half of a FN/arrow key escape sequence
            //http://msdn.microsoft.com/en-us/library/078sfkak.aspx
            (void)_getwch();
            goto fail;
        }
        _putwch(ch);
        if(n_read >= count || n_read >= BMO_TEXT_BUF){
            printf(
                "line too long for buffer."
                "Input line ignored.count:%d, n_read:%d, BMO_TEXT_BUF:%d\n", 
                n_read, count, BMO_TEXT_BUF
            );
            goto fail;
        }
        if(ch == L'\b'){
            //backspace key was pressed
            if(n_read){
                line_buf[--n_read] = L'\0';
                wprintf(L" \b \b");
            }
            return 0;
        }
        line_buf[n_read] = ch;
        n_read++;
        if(ch == L'\n' || ch == L'\r'){
            //for some reason _newlines aren't properly echoed by _putwch()
             _putwch(L'\n');
            //get required length for UTF-8 string.
            int required = WideCharToMultiByte(
                UTF8CP,     //code page
                0,          //flags
                line_buf,   //input string
                n_read,     //input length
                NULL,       //output string (optional)
                0,          //when zero return the resultant length with no-op
                NULL,       //unrepresentable placeholder
                NULL        //pointer to bool indicating default happened 
            );
            if(required >= count - 1){
                puts("UTF-8 string conversion failed;line too long...");
                goto fail;
            }
            if(required == 0){
                //empty line
                goto fail;
            }
            WideCharToMultiByte(
                UTF8CP,
                0,
                line_buf,
                n_read,
                buf,
                count,
                NULL,
                NULL
            );
            wmemset(line_buf, 0, BMO_TEXT_BUF);
            n_read = 0;
            assert(required >= 0);
            return required;
        }
    }
    return 0;
fail:
    wmemset(line_buf, 0, BMO_TEXT_BUF);
    n_read = 0;
    return 0;
    //end win32
    #else
    (void)buf;
    (void)count;
    BMO_NOT_IMPLEMENTED;
    #endif
}

static int getline_nowait(lua_State *L)
{
    char buf[BMO_TEXT_BUF];
    memset(buf, '\0', BMO_TEXT_BUF);
    lua_settop(L, 0);
    size_t len;
    if(!current_buff){
        current_buff = "";
    }
    len = try_getline(buf, BMO_TEXT_BUF);
    if(!len){
        return 0;
    }
    
    buf[len < BMO_TEXT_BUF ? len : BMO_TEXT_BUF - 1] = '\0';
    lua_pushstring(L, current_buff);
    lua_pushstring(L, buf);
    lua_concat(L, 2);
    current_buff = bmo_strdup(lua_tostring(L, -1));
    
    return 1;
}

static int try_interpret(lua_State *L)
{
    int loaded_line = getline_nowait(L);
    if(!loaded_line){
        return 0;
    }
    int status =  luaL_loadbuffer(L, current_buff, strlen(current_buff), "@repl");

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
    try_interpret(L);
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
