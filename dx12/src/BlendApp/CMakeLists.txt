add_executable(BlendApp  WIN32 BlendApp.cc FrameResource.cc)
target_compile_definitions(BlendApp PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(BlendApp PRIVATE d3d12lib)