add_executable(LightColumnsAPP  WIN32 LightColumns.cc FrameResource.cc)
target_compile_definitions(LightColumnsAPP PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(LightColumnsAPP PRIVATE d3d12lib)