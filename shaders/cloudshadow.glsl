uniform mat4 mvp;
uniform vec3 sundir;
uniform sampler3D densityMap;
uniform float densityMult = 1.0;
uniform float absorption;
uniform vec3 absorptionColor = vec3(1);

#ifdef VERTEX_P
in vec3 pos;
in vec3 uv;
out vec3 vRay;
out vec3 vCoord;
void main()
{
	vec3 dir = -sundir;
	gl_Position = mvp * vec4(pos, 1);
	vCoord = uv;
	vRay = dir;
}
#endif

#define NUM_STEPS 16

#include "shaders/raymarch_common.glsl"

#ifdef FRAGMENT_P
in vec3 vCoord;
in vec3 vRay;
out float outShadow;

void main()
{
	vec3 ray = normalize(vRay);
	vec3 position = vCoord;
	vec3 exitPt = GetExitPoint(position, ray);
	vec3 step = (exitPt - position)/NUM_STEPS;
	float len = length(step);

	vec3 Tfactor = -absorption * len * absorptionColor * densityMult;

	float sampleSum = 0.0;
	for(int i = 0; i < NUM_STEPS; ++i)
	{
		float sample = texture(densityMap, position).r;
		sampleSum += sample;
		position += step;
	}

	vec3 T = exp(Tfactor * sampleSum);
	float avgT = (T.x + T.y + T.z) / 3.0;
	float alpha = 1.0 - avgT;

	outShadow = alpha;
}
#endif

