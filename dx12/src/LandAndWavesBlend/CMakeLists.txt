add_executable(LandAndWavesBlendApp  WIN32 LandAndWavesBlend.cc FrameResource.cc BlurFilter.cc)
target_compile_definitions(LandAndWavesBlendApp PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(LandAndWavesBlendApp PRIVATE d3d12lib)