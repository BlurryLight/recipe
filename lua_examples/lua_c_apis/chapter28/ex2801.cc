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

int getglobint(lua_State* ls,const char* var)
{
    int isNum = false;
    lua_getglobal(ls, var);
    int result = (int)lua_tointegerx(ls, -1, &isNum);
    if(!isNum)
    {
        error(ls,"'%s' should be an number!",var);
    }
    lua_pop(ls,1);
    return result;
}

void Load(lua_State* ls,const char* filename,int *w,int*h)
{
    if(luaL_loadfile(ls, filename) || lua_pcall(ls,0,0,0))
    {
        error(ls,"cannot load config file %s because %s",filename, lua_tostring(ls, -1));
    }
    *w = getglobint(ls,"width");
    *h = getglobint(ls,"height");
}

int main()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    int w,h;
    Load(L,"config.lua",&w,&h);
    lua_close(L);
    std::cout << "width: " << w << " " << "height: " << h << std::endl;
    return 0;
}