#pragma once

#include <memory>
#include <string>
#include <vector>

#include "vec.hh"
#include "commonmath.hh"

class GpuHypertexture;
class ShaderInfo;
class ShaderParams;
class SubmenuMenuItem;

// just a collection of stuff needed to control and render a GpuHypertexture
class AnimatedHypertexture
{
public:
	AnimatedHypertexture();
	std::shared_ptr<SubmenuMenuItem> CreateMenu();

	// Creates GpuHypertexture
	void Create();

	// destroys GpuHypertexture
	void Destroy();

	// true if a GpuHypertexture can be created with the creation data
	bool Valid() const;

	void Update(const vec3& sundir);

	////////////////////////////////////////////////////////////////////////////////	
	std::shared_ptr<GpuHypertexture> m_gpuhtex;
	std::shared_ptr<ShaderInfo> m_shader;
	std::shared_ptr<ShaderParams> m_params;

	// Creation parameters
	int m_numCells;
	std::string m_name;
	std::string m_shaderName;
	float m_absorption;
	float m_g;
	float m_time;
	float m_lastUpdateTime;
	Color m_color;
	float m_densityMult;
	Color m_scatterColor;
	Color m_absorbColor;

	// other variables for shaders that are specific to the shader
	float m_radius;
	float m_innerRadius;
};

void htexdb_Init();
// hurray C++11!
std::vector<std::shared_ptr<AnimatedHypertexture>> ParseHtexFile(const char* filename);
void SaveHtexFile(const char* filename, const std::vector<std::shared_ptr<AnimatedHypertexture>>& descriptions);

