cmake_minimum_required(VERSION 3.12)
project("DiDx11Lab")
set(CMAKE_CXX_STANDARD 17)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
find_package(glfw3 REQUIRED)
link_libraries(d3d11 dxgi dxguid D3DCompiler winmm)

add_subdirectory(common)
add_subdirectory(src)


