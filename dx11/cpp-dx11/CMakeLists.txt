cmake_minimum_required(VERSION 3.12)

# === vcpkg related
set(vcpkg "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

if(NOT CMAKE_TOOLCHAIN_FILE AND EXISTS "${vcpkg}")
    set(CMAKE_TOOLCHAIN_FILE "${vcpkg}"
        CACHE FILEPATH "CMake toolchain file")
    message(STATUS "vcpkg toolchain found: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# toolchain file settings should be ahead of any project
project("DiDx11Lab")
include(${CMAKE_SOURCE_DIR}/cmake/generate_header.cmake)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS on)
set(CMAKE_CXX_STANDARD 17)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8;/W3>")
include_directories(${PROJECT_SOURCE_DIR})

find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(third_party)
add_subdirectory(common)
add_subdirectory(src)
