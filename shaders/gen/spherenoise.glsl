
#include "shaders/noise.glsl"

float density(vec3 pt)
{
	const vec3 center = vec3(0.5);
	const float radius = 0.5;
	const float innerRadius = 0.3;
	const float invdiff = 1.0 / ( radius - innerRadius );

	vec3 diff = pt - center;
	float len = length(diff);
	float n = 0.2 * fbmNoise(pt, 0.7, 2.0, 10.0);
	len += n;

	float outerDensity = 1.0 - smoothstep(radius - 0.01, radius + 0.01, len);
	float innerDensity = smoothstep(innerRadius - 0.01, innerRadius + 0.01, len);

	float t = clamp( (len - innerRadius) * invdiff, 0, 1);

	return mix(innerDensity, outerDensity, t);
}

#include "shaders/gen/common.glsl"

