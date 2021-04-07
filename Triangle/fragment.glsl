 
#extension GL_ARB_explicit_attrib_location : enable
#if defined(GL_ARB_explicit_attrib_location)
layout(location = 0) out vec4 fragColor;
#else
out vec4 fragColor;
#endif
uniform sampler2D tex0;
#if __VERSION__ > 120
smooth in vec2 uv;
#else
varying vec2 uv;
#endif
void main(void){
#if defined(GL_ARB_explicit_attrib_location)
	#if __VERSION__ > 120
	fragColor = texture(tex0, uv);
	#else
	fragColor = texture2D(tex0, uv);
	#endif
#else
	#if __VERSION__ > 120
	fragColor = texture(tex0, uv);
	#else
	gl_FragColor = texture2D(tex0, uv);
	#endif
#endif
}