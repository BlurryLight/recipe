add_executable(DynamicCubeMap  WIN32 DynamicCubeMap.cc FrameResource.cc CubeRenderTarget.cc)
target_compile_definitions(DynamicCubeMap PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(DynamicCubeMap PRIVATE d3d12lib)
