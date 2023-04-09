add_executable(ex3101 ex3101.cc)
add_library(BitArrayLib SHARED BitArray.cc)
add_dependencies(ex3101 BitArrayLib)
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/ex3101.lua ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ex3101.lua)