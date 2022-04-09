add_executable(Light GameAPP.cc)
target_compile_definitions(Light PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(Light PRIVATE d3dlib)