#version 450

layout (push_constant) uniform PushConstants {
   mat4 mvp;
   mat2 preRotate;
} pushConstants;
layout (location = 0) in vec2 inVertPos;
layout (location = 1) in vec2 inTexPos;
layout (location = 0) out vec2 outTexPos;

void main() {
   outTexPos = inTexPos;
   vec4 clip = pushConstants.mvp * vec4(inVertPos, 0.0, 1.0);
   gl_Position = vec4(pushConstants.preRotate * vec2(clip.x, clip.y), clip.z, clip.w);
}