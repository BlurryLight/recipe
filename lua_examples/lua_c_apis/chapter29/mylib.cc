
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include <math.h>
#include <iostream>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

static int l_dir(lua_State* L)
{
    const char* path_str = luaL_checkstring(L,1);
    fs::path path(path_str);
    if(!fs::exists(path))
    {
        lua_pushnil(L);
        lua_pushstring(L,"path not exists");
        return 2;
    }
    lua_newtable(L);
    int i = 1;
    for (const auto & entry : fs::directory_iterator(path))
    {
        lua_pushinteger(L,i++);
        lua_pushstring(L,entry.path().u8string().c_str());
        lua_settable(L,-3);
    }
    return 1;
}

static int l_summation(lua_State* L)
{
    double res = 0.0;
    int top = lua_gettop(L);
    for(auto index = top;index > 0;index--)
    {
        double v = luaL_checknumber(L, index);
        res += v;
    }
    lua_pushnumber(L,res);
    return 1;
}

static const struct luaL_Reg mylib[] = 
{
    {"mydir",l_dir},
    {"summation",l_summation},
    {NULL,NULL} // sentinel
};

extern "C" {
    __declspec(dllexport) int luaopen_mylib(lua_State *L)
    {
        luaL_newlib(L, mylib);
        return 1;
    }
}