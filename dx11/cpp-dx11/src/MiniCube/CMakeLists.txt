add_executable(MiniCube GameAPP.cc)
target_link_libraries(MiniCube PRIVATE d3dlib)
add_custom_command(TARGET MiniCube PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/HLSL/ $<TARGET_FILE_DIR:MiniCube>/HLSL/)