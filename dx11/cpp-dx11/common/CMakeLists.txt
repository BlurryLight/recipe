set(SRC
    d3dApp.cc 
    d3dUtils.cc
    vertexLayout.cc
)

add_library(d3dlib ${SRC})
target_link_libraries(d3dlib PUBLIC glfw)
target_link_libraries(d3dlib PUBLIC imguilib)
target_include_directories(d3dlib INTERFACE ${CMAKE_SOURCE_DIR}/common)
target_include_directories(d3dlib PUBLIC ${CMAKE_SOURCE_DIR}/third_party)
target_link_libraries(d3dlib PUBLIC d3d11 dxgi dxguid D3DCompiler winmm)
