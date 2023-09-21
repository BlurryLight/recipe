cmake_minimum_required(VERSION 3.14)

# if ( WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
# 	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT" CACHE STRING "")
# 	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd" CACHE STRING "")
# 	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" CACHE STRING "")
# 	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" CACHE STRING "")
# endif ()

project(PuertsTest)

set (CMAKE_CXX_STANDARD 14)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

set(JS_ENGINE_ROOT ${CMAKE_SOURCE_DIR}/../v8)

include_directories(
    ${JS_ENGINE_ROOT}/Inc
    puerts/include
)

set ( PuertsTestSRC puertsTest.cc 
          puerts/src/CppObjectMapper.cpp
          puerts/src/DataTransfer.cpp
          puerts/src/JSClassRegister.cpp
          )

add_executable(helloworld hello-world.cc)
add_executable(puertsTest ${PuertsTestSRC})
add_executable(v8hellowrold v8helloworld.cc)

add_executable(shell shell.cc)
add_executable(process process.cc)

set (LIB_NAME wee8)
set(target_list helloworld puertsTest v8hellowrold shell process)

foreach(exe ${target_list})
    target_link_libraries(${exe}
        ${JS_ENGINE_ROOT}/Lib/Win64DLL/v8.dll.lib
        ${JS_ENGINE_ROOT}/Lib/Win64DLL/v8_libplatform.dll.lib
        ${LIBS_NEEDED}
    )
    target_compile_definitions (${exe} PRIVATE PLATFORM_WINDOWS)
endforeach()
