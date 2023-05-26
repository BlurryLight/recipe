add_executable(FirstTriangle FirstTriangle.cc)
target_link_libraries(FirstTriangle base)

set(GLSLShaderDir "${CMAKE_SOURCE_DIR}/resources/shader/glsl/FirstTriangle")
set(HLSLShaderDir "${CMAKE_SOURCE_DIR}/resources/shader/hlsl/FirstTriangle")

add_custom_command(
    TARGET FirstTriangle
    POST_BUILD
    COMMAND cmd.exe /c compile.bat
    WORKING_DIRECTORY ${GLSLShaderDir}
    VERBATIM # VERBATIM will try to escape args to make sure in every platform the tool will receive same args
)

add_custom_command(
    TARGET FirstTriangle
    POST_BUILD
    COMMAND cmd.exe /c compile.bat
    WORKING_DIRECTORY ${HLSLShaderDir}
    VERBATIM
)

