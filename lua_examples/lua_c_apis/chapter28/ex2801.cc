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
    int isNum;
    // 需要先将查询的key放在栈上，然后调用gettable 
    lua_pushstring(ls, key);
    lua_gettable(ls, -2);
    int result = (int)(lua_tonumberx(ls, -1, &isNum) * kMaxColorf);
    if(!isNum)
    {
        error(ls,"'%s' invalid component in color",key);
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
    *w = GetGlobalInt(ls,"width");
    *h = GetGlobalInt(ls,"height");
}

void SetColorField(lua_State* ls,const char* key,int value)
{
    lua_pushstring(ls,key);
    lua_pushnumber(ls, (double)value / MAX_COLOR);
    lua_settable(ls, -3);
}

void SetColor(lua_State* ls,const ColorTable* ct)
{
    lua_newtable(ls);
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


    lua_close(L);
    return 0;
}