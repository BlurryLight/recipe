set(EXAMPLE_NAME LightWaves)
add_executable(${EXAMPLE_NAME}  WIN32 LightWaves.cc FrameResource.cc)
target_compile_definitions(${EXAMPLE_NAME} PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${EXAMPLE_NAME} PRIVATE d3d12lib)