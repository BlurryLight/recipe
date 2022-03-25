add_library(d3dlib d3dApp.cc)
target_link_libraries(d3dlib PUBLIC glfw)
target_include_directories(d3dlib INTERFACE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(d3dlib PUBLIC d3d11 dxgi dxguid D3DCompiler winmm)
