add_executable(Light GameAPP.cc)
target_link_libraries(Light PRIVATE d3dlib)
add_custom_command(TARGET Light PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/HLSL/ $<TARGET_FILE_DIR:Light>/HLSL/)