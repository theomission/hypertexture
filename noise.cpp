#include "noise.hh"
#include "vec.hh"
#include <cmath>
#include <cstring>
#include <algorithm>
#include "commonmath.hh"

////////////////////////////////////////////////////////////////////////////////
float bias(float b, float t)
{
	float power = log(b) / log(0.5);
	return pow(t, power);
}

float gain(float g, float t)
{
	if(t > 0.5f)
		return 0.5f * bias(1.f - g, 2.f * t);
	else
		return 1.f - 0.5f * bias(1.f - g, 2.f - 2.f * t);
}

////////////////////////////////////////////////////////////////////////////////
Noise::Noise()
	: m_seed(time(NULL))
	, m_permuteTable(kPermuteSize*2)
	, m_gradients(kPermuteSize)
{
	Init();
}

Noise::Noise(unsigned int seed)
	: m_seed(seed)
	, m_permuteTable(kPermuteSize*2)
	, m_gradients(kPermuteSize)
{
	Init();
}

void Noise::Init()
{
	unsigned int seed = m_seed;
	for(int i = 0, c = kPermuteSize; i < c; ++i)
		m_permuteTable[i] = i;
	for(int i = 0, c = kPermuteSize; i < c; ++i)
	{
		int j = rand_r(&seed) % c;
		std::swap(m_permuteTable[i], m_permuteTable[j]);		
	}
	std::copy(m_permuteTable.begin(), m_permuteTable.begin() + kPermuteSize,
		m_permuteTable.begin() + kPermuteSize);
	for(vec3& v: m_gradients)
	{
		float horizAngle = ((float)rand_r(&seed) / RAND_MAX) * 2.f * M_PI;
		float vertAngle = ((float)rand_r(&seed) / RAND_MAX) * M_PI;
		float cosH = cos(horizAngle),
			sinH = sin(horizAngle);
		float cosV = cos(vertAngle),
			sinV = sin(vertAngle);
		v = {sinV * cosH, sinV * sinH, cosV};
	}
}

float Noise::Grad(int perm, float x, float y, float z) const
{
	//int hash = perm & 0xf;
	//float u = hash < 8 ? x : y;
	//float v = hash < 4 ? y : (hash == 12 || hash == 14) ? x : z;
	//u = (hash&1) ? -u : u;
	//v = (hash&2) ? -v : v;
	//return u + v;

	int hash = perm % kPermuteSize;
	vec3 v = m_gradients[hash];
	return v.x * x + v.y * y + v.z * z;
}

inline int Fold3(const int* p, int i, int j, int k)
{
	return p[p[p[k] + j] + i];
}

float Noise::Sample(float x, float y, float z) const
{
	const int *permute = &m_permuteTable[0];
	int ix = (int)Floor(x);
	int iy = (int)Floor(y);
	int iz = (int)Floor(z);
	x -= (float)ix;
	y -= (float)iy;
	z -= (float)iz;
	ix = ix % kPermuteSize;
	iy = iy % kPermuteSize;
	iz = iz % kPermuteSize;

	float u = spline_c2(x);
	float v = spline_c2(y);
	float w = spline_c2(z);

	int points[8];
	points[0] = Fold3(permute, ix,iy,iz);
	points[1] = Fold3(permute, ix+1,iy,iz);
	points[2] = Fold3(permute, ix+1,iy+1,iz);
	points[3] = Fold3(permute, ix,iy+1,iz);
	points[4] = Fold3(permute, ix,iy,iz+1);
	points[5] = Fold3(permute, ix+1,iy,iz+1);
	points[6] = Fold3(permute, ix+1,iy+1,iz+1);
	points[7] = Fold3(permute, ix,iy+1,iz+1);

	float xlerp[4];
	xlerp[0] = Lerp(u,
		Grad(points[0], x, 	y, 		z), 
		Grad(points[1], x-1, 	y, 		z));
	xlerp[1] = Lerp(u,
		Grad(points[3], x, 	y-1, 	z),
		Grad(points[2], x-1, 	y-1, 	z));
	xlerp[2] = Lerp(u,
		Grad(points[4], x, 	y, 		z-1), 
		Grad(points[5], x-1, 	y, 		z-1));
	xlerp[3] = Lerp(u,
		Grad(points[7], x, 	y-1, 	z-1),
		Grad(points[6], x-1, 	y-1, 	z-1));
	float ylerp[2];
	ylerp[0] = Lerp(v, xlerp[0], xlerp[1]);
	ylerp[1] = Lerp(v, xlerp[2], xlerp[3]);
	return Lerp(w, ylerp[0], ylerp[1]);
}
	
float Noise::FbmSample(const vec3& v, float h, float lacunarity, float octaves)
{
	int numOctaves = (int)octaves;
	float remainder = octaves - (float)numOctaves;

	vec3 pt = v;
	float result = 0.f;
	for(int i = 0; i < numOctaves; ++i)
	{
		result += Sample(pt) * powf(lacunarity, -h * i);
		pt *= lacunarity;
	}

	if(remainder > 0.f) 
	{
		result += remainder * Sample(pt) * powf(lacunarity, -h * numOctaves);
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////////
RidgedMultiFractal::RidgedMultiFractal(Noise& noise, int octaves, float offset,
	float h, float lacunarity, float gain)
	: m_noise(noise)
	, m_exponents(octaves)
	, m_octaves(octaves)
	, m_offset(offset)
	, m_lacunarity(lacunarity)
	, m_gain(gain)
{
	float freq = lacunarity;
	for(float& exponent: m_exponents)
	{
		exponent = powf(freq, -h);
		freq = freq * lacunarity;
	}
}

float RidgedMultiFractal::Sample(float x, float y, float z) const
{
	vec3 pt(x,y,z);
	float signal = m_offset - fabs(m_noise.Sample(x,y,z));
	signal *= signal;

	float weight = 1.f;
	float result = signal;
	for(int i = 0, c = m_octaves; i < c; ++i)
	{
		pt = pt * m_lacunarity;
		weight = Clamp(signal * m_gain, 0.f, 1.f);
	
		signal = m_offset - fabs(m_noise.Sample(pt.x, pt.y, pt.z));

		signal *= signal;
		signal *= weight;

		result += signal * m_exponents[i];
	}
	return result;
}

