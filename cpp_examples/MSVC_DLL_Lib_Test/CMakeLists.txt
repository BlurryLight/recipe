CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
project(MSVC_TEST)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

Option(BuildShared "Shared Lib Test" On)

if(BuildShared)
    add_library(foo1 SHARED foo.cc)
    target_compile_definitions(foo1 PRIVATE -DSIMPLE_DLL_EXPORT)
    add_library(foo2 SHARED foo_2.cc)
    target_compile_definitions(foo2 PRIVATE -DSIMPLE_DLL_EXPORT)
else(BuildShared)
    add_library(foo1 STATIC foo.cc)
    add_library(foo2 STATIC foo_2.cc)
endif()

add_executable(main main.cc)
target_link_libraries(main PRIVATE foo2 foo1)
