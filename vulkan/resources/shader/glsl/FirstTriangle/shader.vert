#version 450

layout(location = 0) out vec3 fragColor;
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

// vec2 positions[3] = vec2[](
//     vec2(0.0, -0.5),
//     vec2(0.5, 0.5),
//     vec2(-0.5, 0.5)
// );

// vec3 colors[3] = vec3[](
//     vec3(1.0, 0.0, 0.0),
//     vec3(0.0, 1.0, 0.0),
//     vec3(0.0, 0.0, 1.0)
// );

void main() {
    // gl_VertexIndex is availble only in vulkan
    // 对应的OpenGL的变量是gl_VertexID
    // see https://stackoverflow.com/questions/57704761/using-spirv-with-opengl-gl-vertexindex-is-always-0
    // gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    // fragColor = colors[gl_VertexIndex];

    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}