#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inVelocity;

layout(location = 0) out vec4 velocity;

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	vec4 diffuse;
	float deltaTime;
}
ubo;

void main() {
	gl_PointSize = 10;
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	velocity = inVelocity;
}
