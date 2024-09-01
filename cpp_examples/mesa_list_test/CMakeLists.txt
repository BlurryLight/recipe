cmake_minimum_required(VERSION 3.14)

project(TestProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == Windows related
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS on)
#

add_executable(${PROJECT_NAME} main.cc)
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:-D_CRT_SECURE_NO_WARNINGS>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/W3>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-pedantic>")

