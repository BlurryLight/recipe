add_executable(LightColumnsDynamicIndex  WIN32 LightColumns.cc FrameResource.cc)
target_compile_definitions(LightColumnsDynamicIndex PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(LightColumnsDynamicIndex PRIVATE d3d12lib)