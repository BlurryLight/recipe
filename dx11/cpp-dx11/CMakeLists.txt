cmake_minimum_required(VERSION 3.12)
project("DiDx11Lab")
include(${CMAKE_SOURCE_DIR}/cmake/generate_header.cmake)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS on)
set(CMAKE_CXX_STANDARD 17)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
find_package(glfw3 REQUIRED)

add_subdirectory(third_party)
add_subdirectory(common)
add_subdirectory(src)


