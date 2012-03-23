uniform float time;
uniform float radius = 0.5;
uniform float innerRadius = 0.3;

#include "shaders/noise.glsl"

float density(vec3 pt)
{
	const vec3 center = vec3(0.5);
	vec3 diff = pt - center;
	float len = length(diff);

	float n = 0.2 * time * abs(fbmNoise(pt, 0.6, 2.0, 11.0));
	float n2 = 0.1 * time * abs(fbmNoise(pt + vec3(3.33), 0.9, 2.0, 11.0));

	float r = max(radius + n,0) ;
	float ir = max(innerRadius + n2,0) ;

	float outerDensity = 1.0 - smoothstep(r - 0.01, r + 0.01, len);
	float innerDensity = smoothstep(ir - 0.01, ir + 0.01, len);

	float invdiff = 1.0 / ( r - ir );
	float t = clamp( (len - ir) * invdiff, 0, 1);

	float density = mix(innerDensity, outerDensity, t);
	return density;
}

#include "shaders/gen/common.glsl"

