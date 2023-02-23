cmake_minimum_required(VERSION 3.12)
# === vcpkg related
set(vcpkg "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

if(NOT CMAKE_TOOLCHAIN_FILE AND EXISTS "${vcpkg}")
    set(CMAKE_TOOLCHAIN_FILE "${vcpkg}"
        CACHE FILEPATH "CMake toolchain file")
    message(STATUS "vcpkg toolchain found: ${CMAKE_TOOLCHAIN_FILE}")
endif()

project(DX12Window)
include(${CMAKE_SOURCE_DIR}/cmake/generate_header.cmake)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS on)
include_directories(third_party)

find_package(imgui CONFIG REQUIRED)
find_package(spdlog REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_subdirectory(third_party/stb_image)
add_subdirectory(common)
add_subdirectory(src)

