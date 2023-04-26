add_executable(FirstTriangle FirstTriangle.cc)
target_link_libraries(FirstTriangle base)

set(ShaderDir "${CMAKE_SOURCE_DIR}/resources/shader/glsl/FirstTriangle")
file(GLOB SHADERS "${ShaderDir}/*" )
foreach(Shader ${SHADERS})
 add_custom_command(
    TARGET FirstTriangle
    POST_BUILD
    COMMAND ${VK_GLSLC_COMPILER} ${Shader} -o ${Shader}.spirv
    WORKING_DIRECTORY ${ShaderDir}
    VERBATIM # VERBATIM will try to escape args to make sure in every platform the tool will receive same args
 )

endforeach()