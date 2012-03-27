
// some common functions 
uniform float rseed = 1.0;
float rand(vec2 coords)
{
	// the idea behind this is to just create a really high frequency sin wave and sample it, and return
	// the fractional part. There's a very common version of this online, but I wasn't sure where it
	// came from or if there was something particularly special about the numbers they chose, so I'm
	// trying my own to see if it works alright.
	float sample = sin(dot(coords, vec2(34.1374, 89.2791)) * (37532.3163 + rseed));
	return fract(sample);
}

////////////////////////////////////////////////////////////////////////////////
// noise function is from:
// https://github.com/ashima/webgl-noise/blob/master/src/classicnoise3D.glsl
// See README for copyright info.

vec3 spline_c2(vec3 t)
{
	vec3 t3 = t*t*t;
	vec3 t4 = t3*t;
	vec3 t5 = t4*t;
	return 6.0 * t5 - 15.0 * t4 + 10.0 * t3;
}

vec3 mod289(vec3 v)
{
	const vec3 inv = vec3(1.0/289.0);
	return v - floor(v * inv) * 289.0;
}

vec4 mod289(vec4 v)
{
	const vec4 inv = vec4(1.0/289.0);
	return v - floor(v * inv) * 289.0;
}

vec4 permute(vec4 x)
{
	return mod289(((x*34.0 + 1.0) * x));
}

vec4 taylorInvSqrt(vec4 r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

float pnoise(vec3 p)
{
	vec3 coordLo = mod289(floor(p));
	vec3 coordHi = mod289(coordLo + vec3(1));

	vec4 xcol = vec4(coordLo.x, coordHi.x, coordLo.x, coordHi.x);
	vec4 ycol = vec4(coordLo.yy, coordHi.yy);
	vec4 zcol0 = coordLo.zzzz;
	vec4 zcol1 = coordHi.zzzz;

	vec4 ixy = permute(permute(xcol) + ycol);
	vec4 ixy0 = permute(ixy + zcol0);
	vec4 ixy1 = permute(ixy + zcol1);

	// gradient computation
	vec4 gx0 = ixy0 * (1.0 / 7.0);
	vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
	gx0 = fract(gx0);
	vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
	vec4 sz0 = step(gz0, vec4(0.0));
	gx0 -= sz0 * (step(0.0, gx0) - 0.5);
	gy0 -= sz0 * (step(0.0, gy0) - 0.5);
	
	vec4 gx1 = ixy1 * (1.0 / 7.0);
	vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
	gx1 = fract(gx1);
	vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
	vec4 sz1 = step(gz1, vec4(0.0));
	gx1 -= sz1 * (step(0.0, gx1) - 0.5);
	gy1 -= sz1 * (step(0.0, gy1) - 0.5);

	// expand the 'stacked' vectors out
	vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
	vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
	vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
	vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
	vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
	vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
	vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
	vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

	vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g100, g100), dot(g010, g010), dot(g110, g110)));	
	vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g101, g101), dot(g011, g011), dot(g111, g111)));	
	g000 *= norm0.x;
	g100 *= norm0.y;
	g010 *= norm0.z;
	g110 *= norm0.w;
	g001 *= norm1.x;
	g101 *= norm1.y;
	g011 *= norm1.z;
	g111 *= norm1.w;

	vec3 fpart0 = fract(p);
	vec3 fpart1 = fpart0 - vec3(1.0);

	float n000 = dot(g000, fpart0);
	float n100 = dot(g100, vec3(fpart1.x, fpart0.yz));
	float n010 = dot(g010, vec3(fpart0.x, fpart1.y, fpart0.z));
	float n110 = dot(g110, vec3(fpart1.xy, fpart0.z));
	float n001 = dot(g001, vec3(fpart0.xy, fpart1.z));
	float n101 = dot(g101, vec3(fpart1.x, fpart0.y, fpart1.z));
	float n011 = dot(g011, vec3(fpart0.x, fpart1.yz));
	float n111 = dot(g111, fpart1);

	vec3 interp = spline_c2(fpart0);

	vec4 zlerp = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), interp.z);
	vec2 ylerp = mix(zlerp.xy, zlerp.zw, interp.y);
	float xlerp = mix(ylerp.x, ylerp.y, interp.x);
	return 2.2 * xlerp;
}

float fbmNoise(vec3 pt, float h, float lacunarity, float octaves)
{
	int numOctaves = int(octaves);
	float result = 0.0;

	for(int i = 0; i < numOctaves; ++i)
	{
		result += pnoise(pt) * pow(lacunarity, -h * i);
		pt *= lacunarity;
	}

	float remainder = octaves - numOctaves;
	if(remainder > 0.0)
	{
		result += remainder * pnoise(pt) * pow(lacunarity, -h * numOctaves);
	}

	return result;
}

