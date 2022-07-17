CMAKE_MINIMUM_REQUIRED(VERSION  3.12)
project(EASTLPlayGround)

add_subdirectory(EASTL)
add_custom_target(NatVis SOURCES EASTL/doc/EASTL.natvis)
add_executable(main main.cc)
target_link_libraries(main PRIVATE EASTL)
