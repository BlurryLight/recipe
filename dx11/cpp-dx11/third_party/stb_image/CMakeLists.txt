add_library(stb_image_lib STATIC stb_image.cc)
message("stb_image dir: " ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(stb_image_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})