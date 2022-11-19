add_executable(MiniCube WIN32 BoxApp.cc)
target_compile_definitions(MiniCube PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(MiniCube PRIVATE d3d12lib)