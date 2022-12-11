add_executable(LandAndWavesAPP  WIN32 LandAndWaves.cc FrameResource.cc)
target_compile_definitions(LandAndWavesAPP PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(LandAndWavesAPP PRIVATE d3d12lib)