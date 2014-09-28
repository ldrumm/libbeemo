#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
#include "../../src/lua/getline_nowait.h"
int main(void)
{
    lua_State * L = luaL_newstate();
    luaL_openlibs(L);
	bmo_reg_getline_nowait(L, "async");
	printf("repl>>\n");
    luaL_dostring(L,"(function debug.debug()"\
	        "repeat"\
	        "io.write('repl>>')\n"\
	        "line = async.getline()\n"\
	        "if #line > 1 then\n"\
	        "   func = loadstring(line)\n"\
	        "   if func then "\
	        "        local ok, msg = pcall(func)"\
	        "       if not ok then "\
	        "          print(msg)"\
	        "       end "\
	        "   end\n"\
	        "end\n"\
	        "until line == cont)()"
   );
	lua_close(L);
	return 0;
}
