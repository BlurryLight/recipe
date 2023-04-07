
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <math.h>
#include <iostream>
#include <string>
#include <filesystem>

static int l_map(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int n = luaL_len(L, 1); //  #table
    for(int i = 1; i <= n; i++)
    {
        lua_pushvalue(L, 2); //copy the function and put it on the stacktop
        lua_geti(L,1,i); // get table[i] and push on the table
        lua_call(L,1,1); // call func(table[i]) without any protection, args are poped and result are pushed
        lua_seti(L,1,i); // This function pops the value from the stack.
    }
    return 0;
}

int main()
{
    int error = 0;
    {
        lua_State* ls = luaL_newstate();
        luaL_openlibs(ls);
        lua_pushcfunction(ls, l_map);
        lua_setglobal(ls,"my_map");
        std::string filename{"ex3001.lua"};
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


// int main(int argc, char *argv[]) {
//     lua_State *L = luaL_newstate();
//     luaL_openlibs(L);

//     // Load Lua code
//     luaL_dostring(L, "function make_adder(x) return function(y) return x + y end end");

//     // Get the closure
//     lua_getglobal(L, "make_adder");
//     lua_pushnumber(L, 2);
//     lua_call(L, 1, 1);

//     // Get the closure and push arguments onto the stack
//     lua_pushnumber(L, 3);

//     // Call the closure with 1 argument and 1 return value
//     lua_call(L, 1, 1);

//     // Get the return value from the stack
//     double result = lua_tonumber(L, -1);

//     printf("Result: %f\n", result);

//     lua_close(L);
//     return 0;
// }
