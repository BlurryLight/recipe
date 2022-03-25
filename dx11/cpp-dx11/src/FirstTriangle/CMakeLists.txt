add_executable(MiniTri GameAPP.cc)
target_link_libraries(MiniTri PRIVATE d3dlib)
# configure_file(HLSL HLSL COPY_ONLY) # this is only suitable for single file
# message(${CMAKE_CURRENT_SOURCE_DIR}) # current cmakelists location
# message(${CMAKE_SOURCE_DIR}) # root cmakelists location
add_custom_command(TARGET MiniTri PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/HLSL/ $<TARGET_FILE_DIR:MiniTri>/HLSL/)