add_executable(InstanceAndCulling  WIN32 InstanceAndCulling.cc FrameResource.cc)
target_compile_definitions(InstanceAndCulling PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(InstanceAndCulling PRIVATE d3d12lib)