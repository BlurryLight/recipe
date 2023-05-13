cmake_minimum_required(VERSION 3.14)
project(Fix64)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == Windows related
if(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS on)
  message("MSVC")
  if(DEFINED ENV{SWIG_EXECUTABLE})
    message("Read Swig Executable From Env: " $ENV{SWIG_EXECUTABLE})
    set(SWIG_EXECUTABLE "$ENV{SWIG_EXECUTABLE}")
  endif()
endif(MSVC)
#

set (UseSWIG_TARGET_NAME_PREFERENCE STANDARD)
find_package(SWIG 4.0 COMPONENTS python)
if(SWIG_FOUND)
  message("SWIG found: ${SWIG_EXECUTABLE}")
endif()
include(${SWIG_USE_FILE})
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_library(Fix64Lib SHARED Fix64.cc)
target_compile_options(Fix64Lib PUBLIC "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-pedantic>")
message(${CMAKE_SOURCE_DIR})
target_include_directories(Fix64Lib PUBLIC ${CMAKE_SOURCE_DIR})


#== swig python begin
find_package (Python COMPONENTS Interpreter Development)
# 生成swig需要的wrapper文件
set_property(SOURCE PythonLib/Fix64Python.i PROPERTY DEPENDS Fix64.h)
set_property(SOURCE PythonLib/Fix64Python.i PROPERTY CPLUSPLUS ON)
swig_add_library(Fix64Py LANGUAGE python SOURCES PythonLib/Fix64Python.i 
  OUTPUT_DIR ${CMAKE_BINARY_DIR}/PythonLib
)
# ignore swig wrapper warning
target_compile_options(Fix64Py PRIVATE "$<$<CXX_COMPILER_ID:GNU,Clang>:-w>")
set_property(TARGET Fix64Py PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE)
set_property(TARGET Fix64Py PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/PythonLib")
target_link_libraries(Fix64Py PRIVATE Python::Python Fix64Lib)
# swig还会为python生成一个同名的py文件，从这个py里引用
add_custom_command(TARGET Fix64Py POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/PythonLib/Fix64.py ${CMAKE_SOURCE_DIR}/PythonLib/Fix64.py
)

if(WIN32)
# windows 还需要拷贝dll
add_custom_command(TARGET Fix64Py POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Fix64Lib> ${CMAKE_SOURCE_DIR}/PythonLib/$<TARGET_FILE_NAME:Fix64Lib>
)
endif()
#== swig python end

enable_testing()
if(NOT "${Python_EXECUTABLE}" STREQUAL "")
  add_test(NAME pythonTest COMMAND ${Python_EXECUTABLE} PyTest.py WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/PythonLib)
endif()