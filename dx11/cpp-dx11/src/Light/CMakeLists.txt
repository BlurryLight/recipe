add_executable(Light GameAPP.cc)
target_compile_definitions(Light PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(Light PRIVATE d3dlib)

# add_custom_command(TARGET Light POST_BUILD
# COMMAND ${CMAKE_COMMAND} -E copy_if_different
# $<TARGET_RUNTIME_DLLS:Light> $<TARGET_FILE_DIR:Light>
# COMMAND_EXPAND_LISTS
# )

# for lower version cmake
add_custom_command(TARGET Light POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:assimp> $<TARGET_FILE_DIR:Light>)