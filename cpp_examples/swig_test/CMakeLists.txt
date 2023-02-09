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

find_package(SWIG 4.0 COMPONENTS python lua)
if(SWIG_FOUND)
  message("SWIG found: ${SWIG_EXECUTABLE}")
endif()
include(${SWIG_USE_FILE})

add_library(testLib SHARED testLib.cc)
target_compile_options(testLib PUBLIC "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-pedantic>")


set_property(SOURCE testLib.i PROPERTY CPLUSPLUS ON)
#== swig python begin
find_package (Python COMPONENTS Interpreter Development)
swig_add_library(testLibPy LANGUAGE python SOURCES testLib.i OUTPUT_DIR ${CMAKE_BINARY_DIR}/PythonLib)
target_link_libraries(testLibPy PRIVATE Python::Python testLib)
set_property(TARGET testLibPy PROPERTY SWIG_USE_LIBRARY_INCLUDE_DIRECTORIES TRUE)
set_property(TARGET testLibPy PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/PythonLib")
#== swig python end

#== swig python begin
find_package (Lua 5.3 REQUIRED)
message("Lua Found ${LUA_VERSION_STRING}")
swig_add_library(testLibLua LANGUAGE lua SOURCES testLib.i OUTPUT_DIR ${CMAKE_BINARY_DIR}/LuaLib)
target_link_libraries(testLibLua PRIVATE ${LUA_LIBRARIES} testLib)
target_include_directories(testLibLua PRIVATE ${LUA_INCLUDE_DIR} )
set_property(TARGET testLibLua PROPERTY SWIG_USE_LIBRARY_INCLUDE_DIRECTORIES TRUE)
set_property(TARGET testLibLua PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/LuaLib")
#== swig python end

add_executable(${PROJECT_NAME} main.cc)
target_link_libraries(${PROJECT_NAME} testLib)
