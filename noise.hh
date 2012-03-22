#pragma once

#include "vec.hh"
#include <vector>

class Noise
{
public:
	Noise();
	Noise(unsigned int seed);

	float Sample(const vec3& v) const { return Sample(v.x, v.y, v.z); }
	float Sample(float x, float y, float z) const;

	float FbmSample(const vec3& v, float h, float lacunarity, float octaves);
private:
	void Init();
	float Grad(int perm, float x, float y, float z) const;
	unsigned int m_seed;
	std::vector<int> m_permuteTable;
	std::vector<vec3> m_gradients;

	constexpr static int kPermuteSize = 256;
};

class RidgedMultiFractal
{
public:
	RidgedMultiFractal(Noise& noise, int octaves, float offset,
		float h, float lacunarity, float gain);

	float Sample(float x, float y, float z) const;
private:

	Noise& m_noise;
	std::vector<float> m_exponents;
	int m_octaves;
	float m_offset;
	float m_lacunarity;
	float m_gain;
};

float bias(float b, float t); // ref impl
float gain(float g, float t); // ref impl

