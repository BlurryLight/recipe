set(SRC 
    d3dApp.cc
    d3dUtils.cc
    GameTimer.cc
    EulerCamera.cc
    MeshGeometry.cc
    ../third_party/MathHelper.cpp
    ../third_party/GeometryGenerator.cpp
    ../third_party/SimpleMath.cpp
    ../utils/resource_path_searcher.cc
)

add_library(d3d12lib STATIC ${SRC})
target_include_directories(d3d12lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(d3d12lib PUBLIC ${CMAKE_SOURCE_DIR}/third_party)
target_include_directories(d3d12lib PUBLIC ${CMAKE_SOURCE_DIR}/utils)
target_compile_options(d3d12lib PUBLIC /utf-8 /W3)
target_compile_definitions(d3d12lib PUBLIC _CRT_SECURE_NO_WARNINGS NOMINMAX)
target_link_libraries(d3d12lib PUBLIC winmm.lib d3dcompiler.lib D3D12.lib dxgi.lib)

# https://github.com/microsoft/DirectX-Graphics-Samples/issues/567
# for d3dx12.h
target_link_libraries(d3d12lib PUBLIC dxguid.lib)
target_link_libraries(d3d12lib PUBLIC spdlog::spdlog)
target_link_libraries(d3d12lib PUBLIC imgui::imgui)
target_link_libraries(d3d12lib PUBLIC assimp::assimp)

# msvc warning
target_compile_options(d3d12lib PUBLIC /wd4267)