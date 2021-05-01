#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
}
ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 uv;

layout(push_constant) uniform va { layout(offset = 0) mat4 model; }
u_pushConstants;

void main() {
	gl_Position = ubo.proj * ubo.view * u_pushConstants.model * vec4(inPosition, 1.0);
	uv = inUV;
}
