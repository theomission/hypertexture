uniform vec4 limits;
uniform float planetRadius;
uniform int octaves;
uniform float offset;
uniform float lacunarity;
uniform float gain;
uniform float h;
uniform vec3 basis0;
uniform vec3 basis1;
uniform vec3 basis2;
uniform float initialFreq;

#ifdef VERTEX_P
in vec2 pos;
out vec2 vUV;
void main()
{
	gl_Position = vec4(pos, 0, 1);
	vUV = 0.5 * pos.xy + vec2(0.5);
}
#endif

#include "shaders/noise.glsl"

#ifdef FRAGMENT_P
in vec2 vUV;
out float outHeight; // channels: R16

void main()
{
	vec2 coord = mix(limits.xy, limits.zw, vUV) * 2.0 - vec2(1.0);
	vec3 v0, v1, v2;
	v0 = coord.x * basis0;
	v1 = coord.y * basis1;
	v2 = basis2;

	vec3 dir = normalize(v0 + v1 + v2);
	vec3 pt = initialFreq * dir;

	float signal = offset - abs(pnoise(pt));
	signal = signal * signal;

	float weight = 1.0;
	float result = signal;
	float freq = lacunarity;
	for(int i = 0; i < octaves; ++i)
	{
		pt = pt * lacunarity;
		weight = clamp(signal * gain, 0.0, 1.0);

		signal = offset - abs(pnoise(pt));
		signal = weight * signal * signal;
		
		result += signal * pow(freq, -h);
		freq = freq * lacunarity;
	}
	outHeight = result;
}

#endif

