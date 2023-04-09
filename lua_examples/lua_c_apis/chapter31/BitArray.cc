
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

// sizeof = 4
struct BitArray {
  int size = 0;
  unsigned int values[];
};

static int l_NewBitArray(lua_State *L) {
  int n = (int)luaL_checkinteger(L, 1);
  luaL_argcheck(L, n >= 1, n, "invalid bitarray size");
  size_t sz = I_WORD_ROUNDUP(n);
  size_t nbytes = sizeof(BitArray) + sz * sizeof(uint32_t);
  BitArray *arr = (BitArray *)lua_newuserdata(L, nbytes);
  arr->size = n;
  memset(arr->values, 0, nbytes - sizeof(BitArray));
  return 1;
}

static int l_SetBit(lua_State *L) {
  BitArray *arr = (BitArray *)lua_touserdata(L, 1);
  luaL_argcheck(L, arr != NULL, 1, "BitArray expected!");
  int index = (int)luaL_checkinteger(L, 2);
  // 0-index based
  if (!(index >= 0 && index < arr->size)) {
    char error_msg[256] = {};
    std::snprintf(error_msg, sizeof(error_msg),
                  "index %d out of range, should be in [0,%d)!", index,
                  arr->size);
    luaL_argerror(L, 2, error_msg);
  }
  luaL_checkany(L, 3);

  if (lua_toboolean(L, 3)) {
    arr->values[I_WORD(index)] |= I_BIT(index);
  } else {
    arr->values[I_WORD(index)] &= ~I_BIT(index);
  }
  return 0;
}

static int l_GetBit(lua_State *L) {
  BitArray *const arr = (BitArray *)lua_touserdata(L, 1);
  luaL_argcheck(L, arr != NULL, 1, "BitArray expected!");
  int index = (int)luaL_checkinteger(L, 2);
  if (!(index >= 0 && index < arr->size)) {
    char error_msg[256] = {};
    std::snprintf(error_msg, sizeof(error_msg),
                  "index %d out of range, should be in [0,%d)!", index,
                  arr->size);
    luaL_argerror(L, 2, error_msg);
  }

  bool res = arr->values[I_WORD(index)] & I_BIT(index);
  lua_pushboolean(L, res);
  return 1;
}

static int l_GetSize(lua_State *L) {
  BitArray *const arr = (BitArray *)lua_touserdata(L, 1);
  luaL_argcheck(L, arr != NULL, 1, "BitArray expected!");
  lua_pushinteger(L, arr->size);
  return 1;
}
static const struct luaL_Reg BitArrayLib[] = {
    {"new", l_NewBitArray},
    {"set", l_SetBit},
    {"get", l_GetBit},
    {"size", l_GetSize},
    {NULL, NULL} // sentinel
};

extern "C" {
__declspec(dllexport) int luaopen_BitArrayLib(lua_State *L) {
  luaL_newlib(L, BitArrayLib);
  return 1;
}
}
