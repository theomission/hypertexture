uniform vec3 sundir;
uniform float absorption;
uniform sampler3D densityMap;
uniform float densityMult = 1.0;
uniform vec3 scatteringColor = vec3(1,1,1);

#ifdef VERTEX_P
in vec3 pos;
out vec3 vCoord;
void main()
{
	vCoord = 0.5 * pos + vec3(0.5);
	gl_Position = vec4(pos.xy,0,1);
}
#endif

#define NUM_LIGHTING_STEPS 32 

#include "shaders/raymarch_common.glsl"

vec3 computeLightingTransmittance(vec3 pos)
{
	vec3 exitPt = GetExitPoint(pos, sundir);
	vec3 step = (exitPt - pos) / NUM_LIGHTING_STEPS;
	float len = length(step);
	vec3 T = vec3(1.0);
	vec3 factor = -absorption * scatteringColor * len * densityMult;
	for(int i = 0; i < NUM_LIGHTING_STEPS; ++i)
	{
		float rho = texture(densityMap, pos).x ;
		vec3 localT = exp(factor * rho);
		T = T * localT;

		pos += step;
	}
	return T;	
}

#ifdef FRAGMENT_P
in vec3 vCoord;
out vec3 outT;
void main()
{
	outT = computeLightingTransmittance(vCoord);
}
#endif

