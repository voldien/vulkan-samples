#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inVelocity;

layout(location = 0) out vec4 velocity;

struct particle_setting {
	float speed;
	float lifetime;
	float gravity;
};

layout(set = 0, binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	*/
	float deltaTime;

	particle_setting setting;
}
ubo;

void main() {

	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition.xyz, 1.0);
	gl_PointSize = (10000.0f / gl_Position.w) * inPosition.w;
	velocity = inVelocity;
}
