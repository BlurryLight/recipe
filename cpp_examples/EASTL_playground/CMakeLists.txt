CMAKE_MINIMUM_REQUIRED(VERSION  3.12)
project(EASTLPlayGround)

add_subdirectory(EASTL)
add_custom_target(NatVis SOURCES EASTL/doc/EASTL.natvis)
add_executable(${PROJECT_NAME} main.cc)
target_link_libraries(${PROJECT_NAME} PRIVATE EASTL)
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:-D_CRT_SECURE_NO_WARNINGS>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/W3>")