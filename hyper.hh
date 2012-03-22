#pragma once

#include "render.hh"
#include <functional>

class Camera;
class Color;
class vec3;

void hyper_Init();

class Hypertexture
{
public:
	typedef std::function<float(float,float,float)> GenFuncType;
	Hypertexture(int numCells, GenFuncType genFunc);
	~Hypertexture();
	void Render(const Camera& camera, const vec3& scale, const vec3& sundir, const Color& sunColor);
private:
	void Compute();
	void ComputeSlice(const std::shared_ptr<std::vector<float>> cells,
		const std::shared_ptr<std::vector<unsigned char>> texels, int z, int num);

	int m_numCells;
	bool m_ready;
	GenFuncType m_genFunc;
	GLuint m_tex;
	int m_completedSlices;
};

