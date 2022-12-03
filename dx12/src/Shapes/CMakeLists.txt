add_executable(ShapesAPP  WIN32 Shapes.cc FrameResource.cc)
target_compile_definitions(ShapesAPP PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ShapesAPP PRIVATE d3d12lib)