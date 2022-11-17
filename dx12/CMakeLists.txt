cmake_minimum_required(VERSION 3.12)
# === vcpkg related
set(vcpkg "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

if(NOT CMAKE_TOOLCHAIN_FILE AND EXISTS "${vcpkg}")
    set(CMAKE_TOOLCHAIN_FILE "${vcpkg}"
        CACHE FILEPATH "CMake toolchain file")
    message(STATUS "vcpkg toolchain found: ${CMAKE_TOOLCHAIN_FILE}")
endif()

project(DX12Window)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS on)
include_directories(third_party)

find_package(spdlog REQUIRED)

add_subdirectory(common)
add_subdirectory(src)

