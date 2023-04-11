
#include <cstdio>
#include <limits.h>
#include <stdint.h>
#include <string.h>
extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

constexpr uint32_t kBitsPerWord = (CHAR_BIT) * sizeof(uint32_t);
#define I_WORD(i) ((uint32_t)i / kBitsPerWord)
#define I_WORD_ROUNDUP(i) ((uint32_t)(i + kBitsPerWord - 1) / kBitsPerWord)
#define I_BIT(i) (1 << ((uint32_t)i % kBitsPerWord))

constexpr const char *kBitArrayKey = "BitArrayLib.BitArray";

// sizeof = 4
struct BitArray {
  int size = 0;
  unsigned int values[0];
};

static void GetParams(lua_State *L,
                      /*out*/ uint32_t **entry,
                      /*out*/ uint32_t *mask) {

  auto arr = (BitArray *)luaL_checkudata(L, 1, kBitArrayKey);

  int index = (int)luaL_checkinteger(L, 2) - 1;
  if (!(index >= 0 && index < arr->size)) {
    char error_msg[256] = {};
    std::snprintf(error_msg, sizeof(error_msg),
                  "index %d out of range, should be in [1,%d]!", index + 1,
                  arr->size);
    luaL_argerror(L, 2, error_msg);
  }
  *entry = &arr->values[I_WORD(index)];
  *mask = I_BIT(index);
}

static int l_NewBitArray(lua_State *L) {
  int n = (int)luaL_checkinteger(L, 1);
  luaL_argcheck(L, n >= 1, n, "invalid bitarray size");
  size_t sz = I_WORD_ROUNDUP(n);
  size_t nbytes = sizeof(BitArray) + sz * sizeof(uint32_t);
  BitArray *arr = (BitArray *)lua_newuserdata(L, nbytes);
  arr->size = n;
  memset(arr->values, 0, nbytes - sizeof(BitArray));

  luaL_setmetatable(L, kBitArrayKey);
  return 1;
}

static int l_SetBit(lua_State *L) {
  luaL_checkany(L, 3);

  uint32_t *entry = nullptr;
  uint32_t mask = 0;
  GetParams(L, &entry, &mask);
  if (lua_toboolean(L, 3)) {
    *entry |= mask;
  } else {
    *entry &= ~mask;
  }
  return 0;
}

static int l_GetBit(lua_State *L) {

  uint32_t *entry = nullptr;
  uint32_t mask = 0;
  GetParams(L, &entry, &mask);

  bool res = *entry & mask;
  lua_pushboolean(L, res);
  return 1;
}

static int l_GetSize(lua_State *L) {
  BitArray *const arr = (BitArray *)luaL_checkudata(L, 1, kBitArrayKey);
  lua_pushinteger(L, arr->size);
  return 1;
}

static int l_array2string(lua_State *L) {
  BitArray *const arr = (BitArray *)luaL_checkudata(L, 1, kBitArrayKey);
  lua_pushfstring(L, "BitArray(%d)",arr->size);
  return 1;
}

static const struct luaL_Reg BitArrayLib_funcs[] = {
    {"new", l_NewBitArray},
    {"set", l_SetBit},
    {"get", l_GetBit},
    {"size", l_GetSize},
    {NULL, NULL} // sentinel
};

static const struct luaL_Reg BitArrayLib_methods[] = {
    {"__tostring", l_array2string},
    {"__len", l_GetSize},
    {"__newindex", l_SetBit},

    {"set", l_SetBit},
    {"get", l_GetBit},
    {"size", l_GetSize},
    {NULL, NULL} // sentinel
};

extern "C" {
__declspec(dllexport) int luaopen_BitArrayLib(lua_State *L) {
  luaL_newmetatable(L, kBitArrayKey);
  lua_pushvalue(L, -1); // copy metatable
  lua_setfield(L, -2, "__index");// meta.__index = meta
  luaL_setfuncs(L, BitArrayLib_methods, 0); // register metatable funcs
  luaL_newlib(L, BitArrayLib_funcs);
  return 1;
}
}
