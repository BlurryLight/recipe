cmake_minimum_required(VERSION 3.12)
# === vcpkg related
set(vcpkg "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

if(NOT CMAKE_TOOLCHAIN_FILE AND EXISTS "${vcpkg}")
    set(CMAKE_TOOLCHAIN_FILE "${vcpkg}"
        CACHE FILEPATH "CMake toolchain file")
    message(STATUS "vcpkg toolchain found: ${CMAKE_TOOLCHAIN_FILE}")
endif()

project(VulkanExamples)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS on)
find_package(Vulkan)
IF (NOT Vulkan_FOUND)
    set(VK_SDK_PATH $ENV{VK_SDK_PATH})
    message(${VK_SDK_PATH})
    if("${VK_SDK_PATH}" STREQUAL "")
        message(FATAL_ERROR "Vulkan Not Found")
    endif()
    set(Vulkan_INCLUDE_DIRS ${VK_SDK_PATH}/Include)
    set(Vulkan_LIBRARY ${VK_SDK_PATH}/Lib/vulkan-1.lib)
    set(VK_SDK_BIN ${VK_SDK_PATH}/Bin)
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

include_directories(${Vulkan_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /W3")
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

find_package(spdlog REQUIRED)
find_package(glfw3)

add_subdirectory(base)

add_executable(main main.cc)
target_link_libraries(main base)
