

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 velocity;

layout(binding = 1) uniform sampler2D spriteTexture;

void main() { outColor = texture(spriteTexture, gl_PointCoord.xy) * vec4(abs(velocity.xyz), 1.0); }
