uniform float time;
uniform float radius = 0.5;
uniform float innerRadius = 0.3;

#include "shaders/noise.glsl"

float density(vec3 pt)
{
	const vec3 startpt = vec3(0.5,0.5,0);
	const vec3 dir = vec3(0,0,1);
	vec3 toPt = pt - startpt;
	vec3 closestVec = toPt - dir * dot(dir, toPt);
	float len = length(closestVec);

	vec3 velocity = vec3(0,0,-0.3) * time;
	float n = 0.3 * abs(fbmNoise(pt + velocity, 0.64, 2.0, 11.0));
	float n2 = 0.1 * abs(fbmNoise(pt + vec3(3.33) + 1.3 * velocity, 0.7, 2.0, 11.0));

	float falloff = 1.0;
	falloff *= exp( -3.f * max(pt.z - 0.1,0) );

	float r = max(radius * falloff + n,0) ;
	float ir = max(innerRadius * falloff + n2,0) ;

	float outerDensity = 1.0 - smoothstep(r - 0.01, r + 0.01, len);
	float innerDensity = 1.0 - smoothstep(ir - 0.01, ir + 0.01, len);

	float density = max(0, outerDensity - innerDensity);

//	return smoothstep(0,0.1,density);
	return density;
}

#include "shaders/gen/common.glsl"

