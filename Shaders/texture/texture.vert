#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 UV;

layout(location = 0) out vec2 uv;

layout(binding = 0) uniform UniformBufferBlock {

	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
}
ubo;

void main() {
	gl_Position = ubo.proj * ubo.modelView * ubo.model * vec4(inPosition, 1.0);
	uv = UV.xy;
}
