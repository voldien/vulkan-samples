#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertex;
layout(location = 0) out vec3 vVertex;

/**
 *
 */
struct skybox_param_t {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ModelViewProjection;
	vec4 DiffuseColor;
};

layout(set = 0, binding = 0) uniform params { skybox_param_t _params; }

u_pushConstants;


void main() {
	vec4 MVPPos = u_pushConstants._params.ModelViewProjection * vec4(vertex, 1.0);
	gl_Position = MVPPos.xyww;
	vVertex = vertex;
}