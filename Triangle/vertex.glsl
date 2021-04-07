
#extension GL_ARB_explicit_attrib_location : enable
#if defined(GL_ARB_explicit_attrib_location)
layout(location = 0) in vec3 vertex;
#else
attribute vec3 vertex;
#endif
#if __VERSION__ > 120
smooth out vec2 uv;
#else
varying vec2 uv;
#endif
out gl_PerVertex{
	vec4 gl_Position;
	float gl_PointSize;
	float gl_ClipDistance[];
};
void main(void){
	gl_Position = vec4(vertex,1.0);
	uv = (vertex.xy + vec2(1.0)) / 2.0;
};