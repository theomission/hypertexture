#pragma once

#include "render.hh"
#include <functional>

class Camera;

void hyper_Init();

class Hypertexture
{
public:
	typedef std::function<float(float,float,float)> GenFuncType;
	Hypertexture(int numCells, GenFuncType genFunc);
	~Hypertexture();
	void Render(const Camera& camera, float scale);
private:
	void Compute();

	int m_numCells;
	bool m_ready;
	GenFuncType m_genFunc;
	GLuint m_tex;
};

