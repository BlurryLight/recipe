cmake_minimum_required(VERSION 3.12)
project(DXWindow)

set(CMAKE_CXX_STANDARD 17)


set(SOURCE_FILES InitDirect3DApp.cpp d3dApp.cc d3dApp.hh d3dUtils.cc d3dUtils.hh)
add_executable(d3d12test WIN32 ${SOURCE_FILES})
target_link_libraries(d3d12test winmm.lib d3dcompiler.lib D3D12.lib dxgi.lib)
target_compile_options(d3d12test PRIVATE /utf-8)
target_compile_definitions(d3d12test PRIVATE _CRT_SECURE_NO_WARNINGS)

