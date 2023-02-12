cmake_minimum_required(VERSION 3.14)
project(TestSwig)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == Windows related
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS on)
#

set (UseSWIG_TARGET_NAME_PREFERENCE STANDARD)
find_package(SWIG 4.0 COMPONENTS python lua)
if(SWIG_FOUND)
  message("SWIG found: ${SWIG_EXECUTABLE}")
endif()
include(${SWIG_USE_FILE})

add_library(testLib SHARED testLib.cc)
target_compile_options(testLib PUBLIC "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-pedantic>")
message(${CMAKE_SOURCE_DIR})
target_include_directories(testLib PUBLIC ${CMAKE_SOURCE_DIR})


#== swig python begin
find_package (Python COMPONENTS Interpreter Development)
# 生成swig需要的wrapper文件
set_property(SOURCE PythonLib/testLibPython.i PROPERTY CPLUSPLUS ON)
swig_add_library(testLibPy LANGUAGE python SOURCES PythonLib/testLibPython.i 
  OUTPUT_DIR ${CMAKE_BINARY_DIR}/PythonLib
)
# ignore swig wrapper warning
target_compile_options(testLibPy PRIVATE "$<$<CXX_COMPILER_ID:GNU,Clang>:-w>")
set_property(TARGET testLibPy PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE)
set_property(TARGET testLibPy PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/PythonLib")
target_link_libraries(testLibPy PRIVATE Python::Python testLib)
# swig还会为python生成一个同名的py文件，从这个py里引用
add_custom_command(TARGET testLibPy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/PythonLib/testLib.py ${CMAKE_SOURCE_DIR}/PythonLib/testLib.py
)
#== swig python end

#== swig lua begin
find_package (Lua 5.3 REQUIRED)
message("Lua Found ${LUA_VERSION_STRING}")
set_property(SOURCE LuaLib/testLibLua.i PROPERTY CPLUSPLUS ON)
swig_add_library(testLibLua LANGUAGE lua SOURCES LuaLib/testLibLua.i OUTPUT_DIR ${CMAKE_BINARY_DIR}/LuaLib)
target_link_libraries(testLibLua PRIVATE ${LUA_LIBRARIES} testLib)
target_include_directories(testLibLua PRIVATE ${LUA_INCLUDE_DIR} )
set_property(TARGET testLibLua PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE)
set_property(TARGET testLibLua PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/LuaLib")
set_target_properties(testLibLua PROPERTIES OUTPUT_NAME "testLib")
#== swig lua end

add_executable(${PROJECT_NAME} main.cc)
target_link_libraries(${PROJECT_NAME} testLib)

# test
enable_testing()
# 假设系统里有lua在环境变量里
add_test(NAME luaTest COMMAND /usr/bin/env lua LuaTest.lua WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/LuaLib)
add_test(NAME pythonTest COMMAND /usr/bin/env python3 PyTest.py WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/PythonLib)