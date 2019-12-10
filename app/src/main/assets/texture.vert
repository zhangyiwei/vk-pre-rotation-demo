#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (push_constant) uniform PushConstants {
   mat2 scaleAndRotate;
} pushConstants;
layout (location = 0) in vec2 inVertPos;
layout (location = 1) in vec2 inTexPos;
layout (location = 0) out vec2 outTexPos;

void main() {
   outTexPos = inTexPos;
   gl_Position = vec4(pushConstants.scaleAndRotate * inVertPos, 0.0, 1.0);
}