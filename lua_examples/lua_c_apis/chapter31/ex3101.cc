
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <math.h>
#include <iostream>
#include <string>


int main()
{
    int error = 0;
    {
        lua_State* ls = luaL_newstate();
        luaL_openlibs(ls);

        std::string filename{"ex3101.lua"};
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
