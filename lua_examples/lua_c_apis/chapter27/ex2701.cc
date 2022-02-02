extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include <iostream>
#include <string>

#include <stdarg.h>
void error(lua_State* L,const char* fmt,...)
{
    va_list argp;
    va_start(argp,fmt);
    vfprintf(stderr,fmt,argp);
    va_end(argp);
    lua_close(L);
    exit(EXIT_FAILURE);
}
int main()
{
    int error = 0;
    lua_State* ls = luaL_newstate();
    luaL_openlibs(ls);
    std::string linebuf;
    while(std::getline(std::cin,linebuf))
    {
        error = luaL_loadstring(ls, linebuf.data()) || lua_pcall(ls,0,0,0);
        if(error)
        {
            std::cerr<<lua_tostring(ls, -1);
            lua_pop(ls,1);
        }
    }
    lua_close(ls);
    return 0;
}