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

static void LuaStackDump(lua_State* L)
{ 
    int i = 1; //lua index is from 1
    int top = lua_gettop(L);
    for(;i <= top;i++)
    {
        int t = lua_type(L,i);
        switch (t)
        {
            case LUA_TSTRING:{
                printf("%s",lua_tostring(L,i)); // \0 in string?
                break;
            }
            case LUA_TBOOLEAN: {
                printf(lua_toboolean(L, i) ? "true" : "false");
                break;
            }
            case LUA_TNUMBER:
            {
                if(lua_isinteger(L,i))
                    printf("%lld",lua_tointeger(L,i));
                else
                    printf("%g",lua_tonumber(L,i));
                break;
            }
            default:
            {
                printf("%s",lua_typename(L,t));
            }
        }
        printf("  ");
    }
    printf("\n");
}
int main()
{
    int error = 0;
    lua_State* L = luaL_newstate();

    lua_pushboolean(L, 1);
    LuaStackDump(L);
    lua_pushnumber(L, 10.00);
    lua_pushnil(L); 
    lua_pushstring(L,"helloworld");
    LuaStackDump(L);
    //true  10  nil  helloworld

    lua_pushvalue(L,-1); LuaStackDump(L);// copy stack top value 
    //true  10  nil  helloworld hello world


    lua_replace(L,1); LuaStackDump(L);// 弹出栈顶元素，并把index所在的元素设置为刚弹出的栈顶的值
    //hellowlrd  10 nil  helloworld 

    lua_settop(L,3); lua_settop(L,6); LuaStackDump(L);// 先缩容后扩容，缩容丢弃元素，扩容填nil,支持负数，负数的话从栈顶往下数
    //hellowlrd  10 nil nil nil nil 

    lua_rotate(L,2,-1); LuaStackDump(L);// 旋转栈顶到index之间的所有元素，正数向栈顶旋转，负数向栈底
    // 可以看到10被交换到了栈顶，所以lua_rorate/lua_pop可以定义出lua_remove
    //helloworld nil nil nil nil 10

    lua_remove(L,-1); LuaStackDump(L);// 移除index所在元素
    //helloworld nil nil nil nil 
    lua_settop(L,-5); LuaStackDump(L);// 负数的settop,等于gettop + (-5) = 6 -5 = 1

   
    lua_close(L);
    return 0;
}