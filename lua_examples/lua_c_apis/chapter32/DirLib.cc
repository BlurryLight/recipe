
extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

}

#ifdef WIN32
#include <strsafe.h>
#include <windows.h>

#endif
#include <iostream>

static int l_dir_iter(lua_State *L);
static int l_dir(lua_State *L) {
#ifdef WIN32
  const char *path = luaL_checkstring(L, 1);
  HANDLE *hFind = (HANDLE *)lua_newuserdata(L, sizeof(HANDLE));
  *hFind = nullptr;
  luaL_getmetatable(L, "DirLib");
  lua_setmetatable(L, -2); // set metatable for userdata
  WIN32_FIND_DATA FindFileData{};
  *hFind = FindFirstFileA(path, &FindFileData);
  if (*hFind == INVALID_HANDLE_VALUE) {
    luaL_error(L, "cannot open%s   : %d ", path, GetLastError());
  }

  lua_pushcclosure(L, l_dir_iter, 1); // 把userdata当做upvalue
  return 1;
#else
  std::cout << "not implemented!" << std::endl;
  return 0;
#endif
}
static int l_dir_iter(lua_State *L) {
  HANDLE *hFind = (HANDLE *)lua_touserdata(L, lua_upvalueindex(1));
  WIN32_FIND_DATA FindFileData{};
  int res = FindNextFile(*hFind, &FindFileData);
  if (res != 0) {
    lua_pushstring(L, FindFileData.cFileName);
    return 1;
  } else {
    auto dw = GetLastError();
    if (dw != ERROR_NO_MORE_FILES) {
      LPVOID lpMsgBuf;
      LPVOID lpDisplayBuf;
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&lpMsgBuf, 0, NULL);
      lpDisplayBuf = (LPVOID)LocalAlloc(
          LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + 40) * sizeof(TCHAR));
      StringCchPrintf((LPTSTR)lpDisplayBuf,
                      LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                      TEXT("failed with error %d: %s"), dw, lpMsgBuf);
      MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

      LocalFree(lpMsgBuf);
      LocalFree(lpDisplayBuf);
    }
  }
  return 0;
}
static int l_dir_gc(lua_State *L) {
  // 当__gc触发的时候，第一个参数是触发gc的对象自身，这里也就是userdata
  HANDLE *hFind = (HANDLE *)lua_touserdata(L, 1);
  if(*hFind)
  {
    std::cout << "dir handle gced" << std::endl;
    FindClose(*hFind);
  }
  return 0;
}

static const struct luaL_Reg DirLib_funcs[] = {
    {"open", l_dir}, {NULL, NULL} // sentinel
};

extern "C" {
__declspec(dllexport) int luaopen_DirLib(lua_State *L) {
  luaL_newmetatable(L, "DirLib");
  lua_pushcfunction(L,l_dir_gc);
  lua_setfield(L, -2, "__gc"); 
  luaL_newlib(L, DirLib_funcs);
  return 1;
}
}
