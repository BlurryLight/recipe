add_executable(ex3201 ex3201.cc)
add_library(DirLib SHARED DirLib.cc)
 add_dependencies(ex3201 DirLib)
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/ex3201.lua ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ex3201.lua)