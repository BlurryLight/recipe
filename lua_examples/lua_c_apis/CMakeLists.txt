cmake_minimum_required(VERSION 3.11)
project(lua_examples)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # lib and a
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # so and lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # .exe and .dll

add_subdirectory(Lua-with-cmake)
set(LUA_SUPPORT_DL ON CACHE BOOL "" FORCE)
set(LUA_BINARY_LINK_DYNAMIC ON CACHE BOOL "" FORCE) # to support dynamic load C Module
set(LUA_BUILD_BINARY ON CACHE BOOL "" FORCE)
link_libraries(lua_dynamic)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

add_subdirectory(chapter27)
add_subdirectory(chapter28)
add_subdirectory(chapter29)
