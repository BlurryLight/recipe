add_executable(StencilApp  WIN32 StencilApp.cc FrameResource.cc)
target_compile_definitions(StencilApp PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(StencilApp PRIVATE d3d12lib)