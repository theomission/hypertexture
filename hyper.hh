#pragma once

#include "render.hh"
#include "commonmath.hh"
#include <functional>

class Camera;
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

////////////////////////////////////////////////////////////////////////////////
// Gpu rendering of density function, allows for possibly real time animation of 
// the function.
class GpuHypertexture
{
public:
	static constexpr int kShadowDim = 512;
	GpuHypertexture(int numCells, const std::shared_ptr<ShaderInfo>& shader, 
		const vec3& scale,
		const std::shared_ptr<ShaderParams>& params = nullptr);

	void Render(const Camera& camera, const vec3& sundir, const Color& sunColor);

	void Update(const vec3& sundir);

	float GetAbsorption() const { return m_absorption; }
	void SetAbsorption(float a) { m_absorption = a; }
	float GetPhaseConstant() const { return m_g; }
	void SetPhaseConstant(float g) { m_g = g; UpdatePhaseConstants(); }
	const Color& GetColor() const { return m_color; }
	void SetColor(const Color& c) { m_color = c; }
	float GetDensityMultiplier() const { return m_densityMult; }
	void SetDensityMultiplier(float f) { m_densityMult = f; }
	const Color& GetScatteringColor() const { return m_scatteringColor; }
	void SetScatteringColor(const Color& c) { m_scatteringColor= c; }
	const Color& GetAbsorptionColor() const { return m_absorptionColor; }
	void SetAbsorptionColor(const Color& c) { m_absorptionColor = c; }

	GLuint GetShadowTexture() const { return m_fboShadow.GetTexture(0); }
	const mat4& GetModel() const { return m_model; }
	const mat4& GetShadowMatrix() const { return m_matShadow; }
private:
	void UpdatePhaseConstants();

	int m_numCells;
	std::shared_ptr<ShaderInfo> m_shader;
	std::shared_ptr<ShaderParams> m_genParams;
	bool m_ready;
	Framebuffer m_fboDensity;
	Framebuffer m_fboTrans;
	Framebuffer m_fboShadow;
	mat4 m_model;
	mat4 m_matShadow;

	float m_absorption;
	float m_g; // phase constant
	float m_phaseConstants[3]; // precomputed stuff for the shader
	Color m_color;
	float m_densityMult;
	Color m_scatteringColor;
	Color m_absorptionColor;
};

