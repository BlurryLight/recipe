add_executable(ex2901 ex2901.cc)
add_library(mylib SHARED mylib.cc)
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/ex29.lua 
${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ex29.lua)

file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/mylib_test.lua 
${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mylib_test.lua)