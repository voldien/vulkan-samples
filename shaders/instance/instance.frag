#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 instanceColor;

layout(set = 0, binding = 2) uniform sampler2D DiffuseTexture;

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	vec4 specularColor;
	vec4 viewPos;
	float shininess;
}
ubo;

void main() {

	const vec3 viewDir = normalize(ubo.viewPos.xyz - vertex);
	const vec3 N = normalize(normal);

	const vec3 halfwayDir = normalize(ubo.direction.xyz + viewDir);
	const float spec = pow(max(dot(N, halfwayDir), 0.0), ubo.shininess);

	const float contribution = max(dot(N, normalize(ubo.direction.xyz)), 0.0);

	const vec4 LightSpecular = ubo.specularColor * spec;
	const vec4 LightColors = contribution * ubo.lightColor;

	fragColor = (ubo.ambientColor + LightColors + LightSpecular) * texture(DiffuseTexture, uv) * instanceColor;
}