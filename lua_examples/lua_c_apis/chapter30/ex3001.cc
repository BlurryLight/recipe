
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

static int l_split(lua_State* L)
{
    const char* s = luaL_checkstring(L, 1);
    // only support one char as seq
    const char* seq = luaL_checkstring(L, 2);

    int i = 1;
    const char* e = nullptr;

    lua_newtable(L);

    while( (e = strchr(s,*seq)) != nullptr)
    {
        lua_pushlstring(L,s, e - s); // with len
        lua_rawseti(L,-2,i++); // table[i++] = str
        s = e + 1;
    }
    // last part
    lua_pushstring(L,s); 
    lua_rawseti(L,-2,i++); 
    // return a table
    return 1;
}

static int l_string_upper(lua_State* L)
{
    size_t l = 0;
    const char* s = luaL_checklstring(L, 1,&l);

    luaL_Buffer b; // similar like vector<char> 
    // vector resize, then return vector.data()
    char* p = luaL_buffinitsize(L, &b, l);
    for(int i = 0;i < l;i++)
    {
        p[i] = std::toupper((unsigned char)(s[i]));
    }
    // convert vector<char> to string
    luaL_pushresultsize(&b, l);
    // now stack top is lua string
    return 1;
}

static int l_string_concat(lua_State* L)
{
    luaL_checktype(L,1,LUA_TTABLE);
    size_t n = luaL_len(L,1); // #table
    luaL_checktype(L, 2, LUA_TSTRING);

    luaL_Buffer b;
    luaL_buffinit(L, &b); // we cannot know how long it will be
    for(int i = 1; i <= n;i++)
    {
        lua_geti(L, 1, i); // take table[i]
        luaL_addvalue(&b); // pop stack top and add to buffer
        if(i != n) // skip last seq
        {
            lua_pushvalue(L, 2); // copy the seq to stack top
            luaL_addvalue(&b); 
        }
    }

    luaL_pushresult(&b);
    // now stack top is lua string
    return 1;
}

int main()
{
    int error = 0;
    {
        lua_State* ls = luaL_newstate();
        luaL_openlibs(ls);
        lua_pushcfunction(ls, l_map);
        lua_setglobal(ls,"my_map");

        lua_pushcfunction(ls, l_split);
        lua_setglobal(ls,"my_split");

        lua_pushcfunction(ls, l_string_upper);
        lua_setglobal(ls,"my_string_upper");

        lua_pushcfunction(ls, l_string_concat);
        lua_setglobal(ls,"my_string_concat");

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
