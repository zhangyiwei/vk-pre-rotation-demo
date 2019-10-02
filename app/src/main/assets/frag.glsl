#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 uFragColor;
layout (push_constant) uniform ColorData {
    vec3 colors;
} colorData;
void main() {
    uFragColor = vec4(colorData.colors, 1.0);
}