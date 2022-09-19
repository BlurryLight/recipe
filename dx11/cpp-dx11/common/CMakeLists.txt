set(SRC
    d3dApp.cc
    d3dUtils.cc
    vertexLayout.cc
    shapes.cc
    src/material.cc
    ../utils/resource_path_searcher.cc
)

add_library(d3dlib STATIC ${SRC})
target_link_libraries(d3dlib PUBLIC glfw)
target_link_libraries(d3dlib PUBLIC imguilib)
target_link_libraries(d3dlib PUBLIC assimp::assimp)
target_include_directories(d3dlib INTERFACE ${CMAKE_SOURCE_DIR}/common)
target_include_directories(d3dlib INTERFACE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(d3dlib INTERFACE ${CMAKE_SOURCE_DIR}/utils)
target_include_directories(d3dlib PUBLIC ${CMAKE_SOURCE_DIR}/third_party)
target_link_libraries(d3dlib PUBLIC d3d11 dxgi dxguid D3DCompiler winmm)
target_link_libraries(d3dlib PUBLIC fmt::fmt)
target_link_libraries(d3dlib PUBLIC spdlog::spdlog)

add_custom_command(TARGET d3dlib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:assimp> $<TARGET_FILE_DIR:d3dlib>)
