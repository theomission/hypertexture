#ifdef VERTEX_P
#include "shaders/debugtex_common.glsl"
#endif

#ifdef FRAGMENT_P
uniform sampler1D colorTex1d;
uniform sampler2D colorTex2d;
uniform int channel;
uniform int dims;
in vec2 vUV;
out vec4 outColor;
void main()
{
	vec4 sample = vec4(0);
	if(dims == 1)
		sample = texture(colorTex1d, vUV.t);
	else
		sample = texture(colorTex2d, vUV);

	if(channel == 0)
		outColor = vec4(sample.r,0,0,1);
	else if(channel == 1)
		outColor = vec4(0,sample.g,0,1);
	else if(channel == 2)
		outColor = vec4(0,0,sample.b,1);
	else if(channel == 3)
		outColor = vec4(sample.aaa,1);
	else
		outColor = vec4(sample.rgb,1);
}
#endif

