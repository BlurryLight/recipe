extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <math.h>
#include <iostream>
#include <string>

// 给lua调用的函数都是这种签名，参数从栈里取得，计算完结果以后把结果压入栈中,返回值表示有多少个返回值
static int l_sin(lua_State* L)
{
    double d = luaL_checknumber(L, 1);
    lua_pushnumber(L,sin(d));
    return 1; // one return value
}

int main()
{
    int error = 0;
    {
        lua_State* ls = luaL_newstate();
        luaL_openlibs(ls);
        lua_pushcfunction(ls, l_sin);
        lua_setglobal(ls,"mysin");
        std::string filename{"ex29.lua"};
        error = luaL_loadfile(ls, filename.data()) || lua_pcall(ls,0,0,0);
        if(error)
        {
            std::cerr<<lua_tostring(ls, -1);
            lua_pop(ls,1);
        }
        lua_close(ls);
    }

    {
        lua_State* ls = luaL_newstate();
        luaL_openlibs(ls);
        std::string filename{"mylib_test.lua"};
        error = luaL_loadfile(ls, filename.data()) || lua_pcall(ls,0,0,0);
        if(error)
        {
            std::cerr<<lua_tostring(ls, -1);
            lua_pop(ls,1);
        }
        lua_close(ls);
    }
    return 0;
}
