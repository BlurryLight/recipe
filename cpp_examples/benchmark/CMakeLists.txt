CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
project(Benchmark_Test)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(sort_bench sort_bench.cc)