#include "htexdb.hh"
#include "render.hh"
#include "hyper.hh"
#include "tokparser.hh"
#include "menu.hh"
#include <iostream>
#include <fstream>

////////////////////////////////////////////////////////////////////////////////
// shaders

static std::shared_ptr<ShaderInfo> g_sphereNoiseShader;

enum AnimatedHtexBindType {
	HTEXBIND_Time,
	HTEXBIND_Radius,
	HTEXBIND_InnerRadius,
	HTEXBIND_Absorption,
};

static std::vector<CustomShaderAttr> g_animatedHtexUniforms =
{
	{ HTEXBIND_Time, "time" },
	{ HTEXBIND_Radius, "radius", true },
	{ HTEXBIND_InnerRadius, "innerRadius", true },
};

////////////////////////////////////////////////////////////////////////////////
void htexdb_Init()
{
	if(!g_sphereNoiseShader)
		g_sphereNoiseShader = render_CompileShader("shaders/gen/spherenoise.glsl", g_animatedHtexUniforms);
}

static std::shared_ptr<ShaderInfo> GetShaderFromName(const char* name)
{
	if(strcasecmp(name, "spherenoise") == 0)
		return g_sphereNoiseShader;

	std::cerr << "couldn't find shader for label \"" << name << "\"" << std::endl;
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
AnimatedHypertexture::AnimatedHypertexture()
	: m_numCells(64)
	, m_absorption(0.7)
	, m_g(-0.1)
	, m_time(0.f)
	, m_lastUpdateTime(0.f)
	, m_color(1,1,1)
	, m_densityMult(10)
	, m_scatterColor(1,1,1)
	, m_absorbColor(1,1,1)
	, m_radius(0.3f)
	, m_innerRadius(0.1f)
{	
}

void AnimatedHypertexture::Create()
{
	m_gpuhtex = std::make_shared<GpuHypertexture>(m_numCells, m_shader, vec3(100.0), m_params);
}

void AnimatedHypertexture::Destroy()
{
	m_gpuhtex.reset();
}

bool AnimatedHypertexture::Valid() const
{
	return m_numCells > 0 &&
		IsPower2(m_numCells) &&
		m_shader ;
}
	
void AnimatedHypertexture::UpdateVariables()
{
	if(!m_gpuhtex) return;
	// keep the saved values in sync with the actual object
	m_gpuhtex->SetAbsorption(m_absorption);
	m_gpuhtex->SetPhaseConstant(m_g);
	m_gpuhtex->SetColor(m_color);
	m_gpuhtex->SetDensityMultiplier(m_densityMult);
	m_gpuhtex->SetScatteringColor(m_scatterColor);
	m_gpuhtex->SetAbsorptionColor(m_absorbColor);
}

void AnimatedHypertexture::Update(const vec3& sundir)
{
	UpdateVariables();
	m_gpuhtex->Update(sundir);
}

std::shared_ptr<SubmenuMenuItem> AnimatedHypertexture::CreateMenu()
{
	auto menu = std::make_shared<SubmenuMenuItem>(m_name.c_str(), 
		SubmenuMenuItem::ChildListType{
			std::make_shared<ButtonMenuItem>("reload shader",
				[this]() { m_shader->Recompile(); }),
			std::make_shared<ButtonMenuItem>("reset time", 
				[this]() { 
					m_time = 0.f; 
					m_lastUpdateTime = 0.f; 
				}),
			std::make_shared<FloatSliderMenuItem>("time slider", &m_time),
			std::make_shared<FloatSliderMenuItem>("absorption", 
				[this]() { return m_absorption; },
				[this](float v) { m_absorption = v; UpdateVariables(); }, 0.1f),
			std::make_shared<FloatSliderMenuItem>("g", 
				[this]() { return m_g; },
				[this](float v) { m_g = v; UpdateVariables(); }, 0.1f, Limits<float>{-1.f, 1.f}),
			std::make_shared<ColorSliderMenuItem>("color", 
				[this]() { return m_color; },
				[this](const Color& c) { m_color = c; UpdateVariables(); }),
			std::make_shared<FloatSliderMenuItem>("density multiplier", 
				[this]() { return m_densityMult; },
				[this](float v) { m_densityMult = v; UpdateVariables(); }),
			std::make_shared<ColorSliderMenuItem>("absorption color", 
				[this]() { return m_absorbColor; },
				[this](const Color& c) { m_absorbColor = c; UpdateVariables(); }),
			std::make_shared<ColorSliderMenuItem>("scattering color", 
				[this]() { return m_scatterColor; },
				[this](const Color& c) { m_scatterColor = c; }),
		});

	if(m_shader == g_sphereNoiseShader)
	{
		menu->AppendChild(std::make_shared<FloatSliderMenuItem>("outer radius", &m_radius, 0.1f));
		menu->AppendChild(std::make_shared<FloatSliderMenuItem>("inner radius", &m_innerRadius, 0.1f));
	}

	return menu;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<std::shared_ptr<AnimatedHypertexture>> ParseHtexFile(const char* filename)
{
	std::vector<std::shared_ptr<AnimatedHypertexture>> result;
	
	std::ifstream file(filename, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
	if(!file) {
		std::cerr << "failed to open " << filename << std::endl;
		return result;
	}
	size_t fileSize = file.tellg();
	file.seekg(0);
	std::vector<char> data(fileSize);
	file.read(&data[0], fileSize);

	if(file.fail()) {
		std::cerr << "failed to read " << filename << std::endl;
		return result;
	}

	TokParser parser(&data[0], fileSize);

	while(parser)
	{
		if(!parser.ExpectTok("{"))
		{
			std::cerr << "missing opening '{'" << std::endl;
			break;
		}

		auto htex = std::make_shared<AnimatedHypertexture>();

		while(parser && !parser.IsTok("}"))
		{
			char bufName[256] = {};
			parser.GetString(bufName, sizeof(bufName));
			parser.ExpectTok("=");
			if(!parser) {
				std::cerr << "bad volume spec";
				break;
			}

			char str[256] = {};
			if(strcasecmp(bufName, "name") == 0) {
				parser.GetString(str, sizeof(str));
				htex->m_name = str;
			} else if(strcasecmp(bufName, "shader") == 0) {
				parser.GetString(str, sizeof(str));
				htex->m_shader = GetShaderFromName(str);
				htex->m_shaderName = str;
				if(htex->m_shader)
				{
					htex->m_params = std::make_shared<ShaderParams>(htex->m_shader);
					htex->m_params->AddParam("time", ShaderParams::P_Float1, &htex->m_time);
					htex->m_params->AddParam("radius", ShaderParams::P_Float1, &htex->m_radius);
					htex->m_params->AddParam("innerRadius", ShaderParams::P_Float1, &htex->m_innerRadius);
				}
			} else if(strcasecmp(bufName, "dim") == 0) {
				htex->m_numCells = parser.GetInt();
			} else if(strcasecmp(bufName, "time") == 0) {
				htex->m_lastUpdateTime = htex->m_time = parser.GetFloat();
			} else if(strcasecmp(bufName, "radius") == 0) {
				htex->m_radius = parser.GetFloat();
			} else if(strcasecmp(bufName, "innerRadius") == 0) {
				htex->m_innerRadius = parser.GetFloat();
			} else if(strcasecmp(bufName, "absorption") == 0) {
				htex->m_absorption = parser.GetFloat();
			} else if(strcasecmp(bufName, "g") == 0) {
				htex->m_g = parser.GetFloat();
			} else if(strcasecmp(bufName, "color") == 0) {
				float r = parser.GetFloat();
				float g = parser.GetFloat();
				float b = parser.GetFloat();
				htex->m_color = Color(r,g,b);
			} else if(strcasecmp(bufName, "densityMult") == 0) {
				htex->m_densityMult = parser.GetFloat();
			} else if(strcasecmp(bufName, "scatterColor") == 0) {
				float r = parser.GetFloat();
				float g = parser.GetFloat();
				float b = parser.GetFloat();
				htex->m_scatterColor = Color(r,g,b);
			} else if(strcasecmp(bufName, "absorbColor") == 0) {
				float r = parser.GetFloat();
				float g = parser.GetFloat();
				float b = parser.GetFloat();
				htex->m_absorbColor = Color(r,g,b);
			}
			else {
				std::cerr << "Unrecognized label " << bufName << std::endl;
			}
		}

		if(!parser) {
			std::cerr << "error while parsing " << htex->m_name << std::endl;
			break;
		}

		if(!parser.ExpectTok("}"))
		{
			std::cerr << "missing closing '}'" << std::endl;
			break;
		}

		if(!htex->Valid()) {
			std::cerr << "htex \"" << htex->m_name << "\" isn't valid, ignoring..." << std::endl;
		} else {
			result.push_back(htex);
		}
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
void SaveHtexFile(const char* filename, const std::vector<std::shared_ptr<AnimatedHypertexture>>& descriptions)
{
	TokWriter w(filename);

	for(const auto& animHtex: descriptions)
	{
		w.Token("{"); w.Nl();
		w.String("name"); w.Token("="); w.String(animHtex->m_name.c_str()); w.Nl();
		w.String("shader"); w.Token("="); w.String(animHtex->m_shaderName.c_str()); w.Nl();
		w.String("dim"); w.Token("="); w.Int(animHtex->m_numCells); w.Nl();
		w.String("time"); w.Token("="); w.Float(animHtex->m_time); w.Nl();
		w.String("absorption"); w.Token("="); w.Float(animHtex->m_absorption); w.Nl();
		w.String("g"); w.Token("="); w.Float(animHtex->m_g); w.Nl();
		w.String("color"); w.Token("="); w.Float(animHtex->m_color.r); 
			w.Float(animHtex->m_color.g); w.Float(animHtex->m_color.b); w.Nl();
		w.String("densityMult"); w.Token("="); w.Float(animHtex->m_densityMult); w.Nl();
		w.String("scatterColor"); w.Token("="); w.Float(animHtex->m_scatterColor.r); 
			w.Float(animHtex->m_scatterColor.g); w.Float(animHtex->m_scatterColor.b); w.Nl();
		w.String("absorbColor"); w.Token("="); w.Float(animHtex->m_absorbColor.r); 
			w.Float(animHtex->m_absorbColor.g); w.Float(animHtex->m_absorbColor.b); w.Nl();
	
		// variables for shader types
		w.String("radius"); w.Token("="); w.Float(animHtex->m_radius); w.Nl();
		w.String("innerRadius"); w.Token("="); w.Float(animHtex->m_innerRadius); w.Nl();
		w.Token("}"); w.Nl();
	}
}

