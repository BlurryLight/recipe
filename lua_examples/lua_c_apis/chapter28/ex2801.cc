extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include <iostream>
#include <string>

#include <stdarg.h>

constexpr static float kMaxColorf = 255.0f;
#define MAX_COLOR 255
struct ColorTable
{
    const char* name;
    unsigned char red,green,blue;
} colortable[]
{
  {"WHITE", MAX_COLOR, MAX_COLOR, MAX_COLOR}, {"RED", MAX_COLOR, 0, 0},
      {"GREEN", 0, MAX_COLOR, 0}, {"BLUE", 0, 0, MAX_COLOR}, {NULL, 0, 0, 0},
};
void error(lua_State* L,const char* fmt,...)
{
    va_list argp;
    va_start(argp,fmt);
    vfprintf(stderr,fmt,argp);
    va_end(argp);
    lua_close(L);
    exit(EXIT_FAILURE);
}

int GetGlobalInt(lua_State* ls,const char* var)
{
    int isNum = 0;
    lua_getglobal(ls, var);
    int result = (int)lua_tointegerx(ls, -1, &isNum);
    if(!isNum)
    {
        error(ls,"'%s' should be an number!",var);
    }
    lua_pop(ls,1);
    return result;
}

int GetColorField(lua_State* ls,const char* key)
{
    if(lua_getfield(ls, -1, key) != LUA_TNUMBER)
    {
        error(ls,"'%s' invalid component in color",key);
    }
    int result = (int)(lua_tonumber(ls, -1) * kMaxColorf);
    lua_pop(ls,1);
    return result;
}


void Load(lua_State* ls,const char* filename,int *w,int*h)
{
    if(luaL_loadfile(ls, filename) || lua_pcall(ls,0,0,0))
    {
        error(ls,"cannot load config file %s because %s",filename, lua_tostring(ls, -1));
    }
    *w = GetGlobalInt(ls,"width");
    *h = GetGlobalInt(ls,"height");
}

void SetColorField(lua_State* ls,const char* key,int value)
{
    lua_pushnumber(ls, (double)value / MAX_COLOR);
    lua_setfield(ls, -2, key);
}

void SetColor(lua_State* ls,const ColorTable* ct)
{
    // 第一个参数是多少个数组元素，第二个参数是多少个table元素
    lua_createtable(ls, 0, 3);
    SetColorField(ls, "red", ct->red);
    SetColorField(ls, "green", ct->green);
    SetColorField(ls, "blue", ct->blue);
    // 清栈，并把这个变量设置为一个全局变量
    lua_setglobal(ls,ct->name);
}
// case-insensitive str compare
bool ciequals(const char* a,const char* b)
{
    size_t len1 = strlen(a);
    size_t len2 = strlen(b);
    if(len1 != len2) return false;
    for(int i = 0; i < len1;i++)
    {
        if(tolower(a[i]) != tolower(b[i]))
        {
            return false;
        }
    }
    return true;
}

double LuaFFunc(lua_State* ls,double x, double y)
{
    lua_getglobal(ls,"f");
    lua_pushnumber(ls,x);
    lua_pushnumber(ls,y);

    // 2 args, 1 return value
    if(lua_pcall(ls, 2, 1, 0) != LUA_OK)
    {
        error(ls,"error running function 'f', %s",lua_tostring(ls, -1));
    }
    int isNum;
    double z = lua_tonumberx(ls, -1, &isNum);
    if(!isNum)
    {
        error(ls,"function 'f' does return number");
    }
    lua_pop(ls, 1);
    return z;
}

void call_va(lua_State *ls,const char* func,const char* sig,...)
{
    va_list vl;
    int narg,nres;
    va_start(vl,sig);
    lua_getglobal(ls,func);
    for(narg = 0;*sig;narg++)
    {
        luaL_checkstack(ls, 1, "too many args");
        switch(*sig++)
        {
            case 'b':
                lua_pushboolean(ls,va_arg(vl,int));
                break;
            case 'd':
            {
                lua_pushnumber(ls,va_arg(vl,double));
                break;
            }
            case 'i':
            {
                lua_pushinteger(ls,va_arg(vl,int));
                break;
            }
            case 's':
            {
                lua_pushstring(ls,va_arg(vl,char*));
                break;
            }
            case '>':
            {
                goto endargs;
            }
            default:
                error(ls,"invalid option %c",*(sig - 1));
        }
    }
endargs:
    nres = strlen(sig);
    if(lua_pcall(ls,narg,nres,0) != LUA_OK)
    {
        error(ls,"error calling '%s': %s",func,lua_tostring(ls, -1));
    }

    nres = -nres;
    while(*sig)
    {
        switch(*sig++)
        {
            case 'd':
            {
                int isnum;
                double n = lua_tonumberx(ls, nres, &isnum);
                if(!isnum)
                {
                    error(ls,"wrong result type");
                }
                *va_arg(vl,double*) = n;
                break;
            }
            case 'i':
            {
                int isnum;
                int n = lua_tointegerx(ls, nres, &isnum);
                if(!isnum)
                {
                    error(ls,"wrong result type");
                }
                *va_arg(vl,int*) = n;
                break;
            }
            case 's':
            {
                const char* s= lua_tostring(ls, nres);
                if(!s)
                {
                    error(ls,"wrong result type");
                }
                *va_arg(vl,const char**) = s;
                break;
            }
            case 'b':
            {
                int flag = lua_toboolean(ls, nres);
                *va_arg(vl,int*) = flag;
                break;
            }
            default:
                error(ls,"invalid option %c",*(sig - 1));
        }
        nres++;
    }
    va_end(vl);
}

int main()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    int i = 0;
    while(colortable[i].name != NULL)
    {
        SetColor(L, &colortable[i]);
        i++;
    }

    int w,h;
    Load(L,"config.lua",&w,&h);
    std::cout << "width: " << w << " " << "height: " << h << std::endl;

    lua_getglobal(L,"background");
    int redColor = -1,greenColor = -1,blueColor = -1;
    if(lua_isstring(L, -1))
    {
        const char* name = lua_tostring(L,-1);
        int i = 0;
        for(i = 0; colortable[i].name != NULL;i++ )
        {
          if (ciequals(name, colortable[i].name)) {
            break;
          }
        }
        if(colortable[i].name == NULL)
        {
            error(L,"invalid color name '%s'!",name);
        }
        else
        {
            redColor = colortable[i].red;
            greenColor = colortable[i].green;
            blueColor = colortable[i].blue;
        }
    }
    else if(lua_istable(L,-1))
    {
        redColor = GetColorField(L,"red");
        greenColor = GetColorField(L,"green");
        blueColor = GetColorField(L,"blue");
    }
    else
    {
        error(L,"invalid value for 'background'!");
    }
    std::printf("red:%d green:%d blue:%d\n",redColor,greenColor,blueColor);


    // std::cout << "f func(0,0): " << LuaFFunc(L, 0, 0) << std::endl;
    double x =0,y=0,z;
    call_va(L, "f", "dd>d", x,y,&z );
    std::cout << "f func(0,0): " << z << std::endl;

    std::string client{"lua client"};
    const char* res = nullptr;
    call_va(L, "hello", "bs>s", true,client.data(),&res);
    std::cout << "hello() " <<  std::string(res) << std::endl;
    lua_close(L);
    return 0;
}