add_executable(Crate  WIN32 Crate.cc FrameResource.cc)
target_compile_definitions(Crate PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(Crate PRIVATE d3d12lib)