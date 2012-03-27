uniform float time;
uniform float width = 0.2;

#include "shaders/noise.glsl"

vec3 vecnoise(vec3 pt)
{
	vec3 result = vec3(
		pnoise(pt),
		pnoise(pt + vec3(7.777)),
		pnoise(pt + vec3(13.1313)));
	return result;
}

float density(vec3 pt)
{
	//pt += vecnoise(pt) * 0.5;
	const vec3 normal = vec3(0,0,1);
	const vec3 center = vec3(0.5);

	vec3 velocity = time * vec3(0.2,0.0,0.0);
	float n = 0.3 * abs(fbmNoise(pt + velocity, 0.79, 1.9, 9.0));
	pt += n * normal;

	vec3 toPt = pt - center;
	float dist = dot(toPt, normal);
	float len = abs(dist);
	
	float density = 1.0 - smoothstep(width - 0.01, width + 0.01, len);
	return density;
}

#include "shaders/gen/common.glsl"

