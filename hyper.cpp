#include <algorithm>
#include "hyper.hh"
#include "common.hh"
#include "render.hh"
#include "task.hh"
#include "vec.hh"
#include "camera.hh"
#include "commonmath.hh"
#include "gputask.hh"

////////////////////////////////////////////////////////////////////////////////
static std::shared_ptr<ShaderInfo> g_htexShader;
enum HtexUniformLocType {
	HTEXBIND_DensityMap,
	HTEXBIND_EyePosInModel,
	HTEXBIND_Absorption,
	HTEXBIND_PhaseConstants,
	HTEXBIND_TransMap,
	HTEXBIND_DensityMult,
	HTEXBIND_AbsorptionColor,
};

static std::vector<CustomShaderAttr> g_htexUniforms =
{
	{ HTEXBIND_DensityMap, "densityMap" },
	{ HTEXBIND_EyePosInModel, "eyePosInModel" },
	{ HTEXBIND_Absorption, "absorption" },
	{ HTEXBIND_PhaseConstants, "phaseConstants" },
	{ HTEXBIND_TransMap, "transMap" },
	{ HTEXBIND_DensityMult, "densityMult" },
	{ HTEXBIND_AbsorptionColor, "absorptionColor" },
};

static std::shared_ptr<ShaderInfo> g_lightingShader;

enum LightingUniformLocType {
	LBIND_Absorption,
	LBIND_DensityMap,
	LBIND_DensityMult,
	LBIND_ScatteringColor,
};

static std::vector<CustomShaderAttr> g_lightingUniforms =
{
	{ LBIND_Absorption, "absorption" },
	{ LBIND_DensityMap, "densityMap" },
	{ LBIND_DensityMult, "densityMult" },
	{ LBIND_ScatteringColor, "scatteringColor" },
};

static std::shared_ptr<Geom> g_boxGeom;

////////////////////////////////////////////////////////////////////////////////
static std::shared_ptr<Geom> CreateHypertextureBoxGeom();
void hyper_Init()
{
	if(!g_htexShader)
		g_htexShader = render_CompileShader("shaders/hypertexture.glsl", g_htexUniforms);
	if(!g_lightingShader)
		g_lightingShader = render_CompileShader("shaders/computelighting.glsl", g_lightingUniforms);

	g_boxGeom = CreateHypertextureBoxGeom();
}

////////////////////////////////////////////////////////////////////////////////
Hypertexture::Hypertexture(int numCells, GenFuncType genFunc)
	: m_numCells(numCells)
	, m_ready(false)
	, m_genFunc(genFunc)
	, m_tex(0)
	, m_completedSlices(0)
{
	Compute();
}

Hypertexture::~Hypertexture()
{
	glDeleteTextures(1, &m_tex);
}
	
void Hypertexture::ComputeSlice(const std::shared_ptr<std::vector<float>> cells,
	const std::shared_ptr<std::vector<unsigned char>> texels, int zStart, int zNum)
{
	const int numCells = m_numCells;
	auto runFunc = [zStart, zNum, cells, texels, numCells, m_genFunc]() {
		float *data = &(*cells)[0];
		float inc = 1.f / numCells;
		vec3 coord(0, 0, zStart*inc);
		int offsetStart = zStart*numCells*numCells;
		int offset = offsetStart;
		for(int z = zStart, zEnd = zStart + zNum; z < zEnd; ++z)
		{
			coord.y = 0.f;
			for(int y = 0; y < numCells; ++y)
			{
				coord.x = 0.f;
				for(int x = 0; x < numCells; ++x, ++offset)
				{
					data[offset] = m_genFunc(coord.x, coord.y, coord.z);
					coord.x += inc;
				}
				coord.y += inc;
			}
			coord.z += inc;
		}
		
		const int offsetEnd = offset;
		auto range = std::minmax_element(cells->begin() + offsetStart, cells->begin() + offsetEnd);
		const float invScale = 1.f/(*range.second - *range.first);
		const float minVal = *range.first;
		std::transform(cells->begin() + offsetStart, cells->begin() + offsetEnd, texels->begin() + offsetStart,
			[minVal, invScale](float density) 
			{
				float val = (density - minVal) * invScale;
				return static_cast<unsigned char>(255 * Clamp(val, 0.f, 1.f));
			});
	};

	auto completeFunc = [cells, texels, numCells, &m_ready, &m_tex, &m_completedSlices, zNum]() {
		m_completedSlices += zNum;
		if(m_completedSlices == numCells)
		{
			// create opengl textures
			glGenTextures(1, &m_tex);
			glBindTexture(GL_TEXTURE_3D, m_tex);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, numCells, numCells, numCells, 0, GL_RED,
					GL_UNSIGNED_BYTE, &(*texels)[0]);
			glGenerateMipmap(GL_TEXTURE_3D);

			checkGlError("Creating hypertexture");
			m_ready = true;
		}
	};

	task_AppendTask(std::make_shared<Task>(nullptr, completeFunc, runFunc));
}

void Hypertexture::Compute()
{
	m_completedSlices = 0;
	const int numCells = m_numCells;
	auto cells = std::make_shared<std::vector<float>>(numCells*numCells*numCells);
	auto texels = std::make_shared<std::vector<unsigned char>>(cells->size());
	for(int z = 0; z < numCells; z += 4)
	{
		int count = Min(4, numCells - z);
		ComputeSlice(cells, texels, z, count);
	}
}

void Hypertexture::Render(const Camera& camera, const vec3& scale, const vec3& sundir, const Color& sunColor)
{
	if(!m_ready) return;
	const ShaderInfo* shader = g_htexShader.get();

	mat4 model = MakeScale(scale) * MakeTranslation(-0.5f, -0.5f, -0.5f);
	mat4 modelInv = AffineInverse(model);
	mat4 mvp = camera.GetProj() * (camera.GetView() * model);

	vec3 eyePos = TransformPoint(modelInv, camera.GetPos());

	GLint mvpLoc = shader->m_uniforms[BIND_Mvp];
	GLint densityMapLoc = shader->m_custom[HTEXBIND_DensityMap];
	GLint eyePosInModelLoc = shader->m_custom[HTEXBIND_EyePosInModel];
	GLint sundirLoc = shader->m_uniforms[BIND_Sundir];
	GLint sunColorLoc = shader->m_uniforms[BIND_SunColor];

	glUseProgram(shader->m_program);
	glUniformMatrix4fv(mvpLoc, 1, 0, mvp.m);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, m_tex);
	glUniform1i(densityMapLoc, 0);
	glUniform3fv(eyePosInModelLoc, 1, &eyePos.x);
	glUniform3fv(sundirLoc, 1, &sundir.x);
	glUniform3fv(sunColorLoc, 1, &sunColor.r);

	glEnable(GL_CULL_FACE);
	g_boxGeom->Render(*shader);
	glDisable(GL_CULL_FACE);
}

////////////////////////////////////////////////////////////////////////////////
GpuHypertexture::GpuHypertexture(int numCells, const std::shared_ptr<ShaderInfo>& shader,
	const std::shared_ptr<ShaderParams>& params)
	: m_numCells(numCells)
	, m_shader(shader)
	, m_genParams(params)
	, m_ready(true)
	, m_fboDensity{numCells, numCells, numCells}
	, m_fboTrans{numCells, numCells, numCells}
	, m_completedSlices(0)
	, m_absorption(0.1)
	, m_g(0.1)
	, m_phaseConstants{}
	, m_color(1,1,1)
	, m_densityMult(1.0)
	, m_scatteringColor(1,1,1)
	, m_absorptionColor(1,1,1)
{
	m_fboDensity.AddTexture3D(GL_R8, GL_RED, GL_UNSIGNED_BYTE);
	m_fboDensity.Create();
	
	m_fboTrans.AddTexture3D(GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
	m_fboTrans.Create();

	UpdatePhaseConstants();
}
	
void GpuHypertexture::UpdatePhaseConstants()
{
	float g = m_g;
	float g2 = g*g;
	m_phaseConstants[0] = (3.0 * (1.f - g2)) / (2.f * (2.f + g2));
	m_phaseConstants[1] = 1 + g2;
	m_phaseConstants[2] = -2*g;
}
	
void GpuHypertexture::Update(const vec3& sundir)
{
	if(!m_ready) return;
	m_ready = false;
	m_completedSlices = 0;

	auto submit = [this, sundir]() {
		const int numCells = m_numCells;
		glEnable(GL_CULL_FACE);
		ViewportState vpState(0, 0, numCells, numCells);

		m_fboDensity.Bind();
			
		const ShaderInfo* shader = m_shader.get();
		GLint posLoc = shader->m_attrs[GEOM_Pos];
		glUseProgram(shader->m_program);
		if(m_genParams) m_genParams->Submit();
	
		float zCoord = -1.f;
		const float zInc = 2.f / numCells;
		for(int z = 0; z < numCells; ++z, zCoord += zInc)
		{
			m_fboDensity.BindLayer(z);

			glBegin(GL_TRIANGLE_STRIP);
			glVertexAttrib3f(posLoc, -1.f, -1.f, zCoord);
			glVertexAttrib3f(posLoc, 1.f, -1.f, zCoord);
			glVertexAttrib3f(posLoc, -1.f, 1.f, zCoord);
			glVertexAttrib3f(posLoc, 1.f, 1.f, zCoord);
			glEnd();
			checkGlError("GpuHypertexture::Update - submit");
		}

		m_fboTrans.Bind();

		shader = g_lightingShader.get();
		glUseProgram(shader->m_program);
		posLoc = shader->m_attrs[GEOM_Pos];
		GLint sundirLoc = shader->m_uniforms[BIND_Sundir];
		GLint absorptionLoc = shader->m_custom[LBIND_Absorption];
		GLint densityMapLoc = shader->m_custom[LBIND_DensityMap];
		GLint densityMultLoc = shader->m_custom[LBIND_DensityMult];
		GLint scatteringColor = shader->m_custom[LBIND_ScatteringColor];

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, m_fboDensity.GetTexture(0));
		glUniform1i(densityMapLoc, 0);
		glUniform3fv(sundirLoc, 1, &sundir.x);
		glUniform1f(absorptionLoc, m_absorption);
		glUniform1f(densityMultLoc, m_densityMult);
		glUniform3fv(scatteringColor, 1, &m_scatteringColor.r);

		zCoord = -1.f;
		for(int z = 0; z < numCells; ++z, zCoord += zInc)
		{
			m_fboTrans.BindLayer(z);

			glUseProgram(shader->m_program);

			glBegin(GL_TRIANGLE_STRIP);
			glVertexAttrib3f(posLoc, -1.f, -1.f, zCoord);
			glVertexAttrib3f(posLoc, 1.f, -1.f, zCoord);
			glVertexAttrib3f(posLoc, -1.f, 1.f, zCoord);
			glVertexAttrib3f(posLoc, 1.f, 1.f, zCoord);
			glEnd();
			checkGlError("GpuHypertexture::Update - submit lighting");
		}

		glDisable(GL_CULL_FACE);

		m_ready = true;
	};

	gputask_Append(std::make_shared<GpuTask>(submit, nullptr));
}

void GpuHypertexture::Render(const Camera& camera, const vec3& scale, const vec3& sundir, const Color& sunColor)
{
	const ShaderInfo* shader = g_htexShader.get();

	mat4 model = MakeScale(scale) ;
	mat4 modelInv = AffineInverse(model);
	mat4 mvp = camera.GetProj() * (camera.GetView() * model);

	vec3 eyePos = TransformPoint(modelInv, camera.GetPos());

	GLint mvpLoc = shader->m_uniforms[BIND_Mvp];
	GLint densityMapLoc = shader->m_custom[HTEXBIND_DensityMap];
	GLint eyePosInModelLoc = shader->m_custom[HTEXBIND_EyePosInModel];
	GLint absorptionLoc = shader->m_custom[HTEXBIND_Absorption];
	GLint sundirLoc = shader->m_uniforms[BIND_Sundir];
	GLint sunColorLoc = shader->m_uniforms[BIND_SunColor];
	GLint colorLoc = shader->m_uniforms[BIND_Color];
	GLint transMapLoc = shader->m_custom[HTEXBIND_TransMap];
	GLint phaseConstantsLoc = shader->m_custom[HTEXBIND_PhaseConstants];
	GLint densityMultLoc = shader->m_custom[HTEXBIND_DensityMult];
	GLint absorptionColorLoc = shader->m_custom[HTEXBIND_AbsorptionColor];

	glUseProgram(shader->m_program);
	glUniformMatrix4fv(mvpLoc, 1, 0, mvp.m);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, m_fboDensity.GetTexture(0));
	glUniform1i(densityMapLoc, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, m_fboTrans.GetTexture(0));
	glUniform1i(transMapLoc, 1);
	glUniform3fv(eyePosInModelLoc, 1, &eyePos.x);
	glUniform3fv(sundirLoc, 1, &sundir.x);
	glUniform3fv(sunColorLoc, 1, &sunColor.r);
	glUniform1f(absorptionLoc, m_absorption);
	glUniform3fv(phaseConstantsLoc, 1, m_phaseConstants);
	glUniform3fv(colorLoc, 1, &m_color.r);
	glUniform1f(densityMultLoc, m_densityMult);
	glUniform3fv(absorptionColorLoc, 1, &m_absorptionColor.r);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	
	glEnable(GL_CULL_FACE);
	g_boxGeom->Render(*shader);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

////////////////////////////////////////////////////////////////////////////////
static std::shared_ptr<Geom> CreateHypertextureBoxGeom()
{
	static float verts[] = {
		1.f, 1.f, -1.f,
		1.f, 1.f, 0.f,

		-1.f, 1.f, -1.f,
		0.f, 1.f, 0.f,

		-1.f, -1.f, -1.f,
		0.f, 0.f, 0.f,

		1.f, -1.f, -1.f,
		1.f, 0.f, 0.f,

		1.f, 1.f, 1.f,
		1.f, 1.f, 1.f,

		-1.f, 1.f, 1.f,
		0.f, 1.f, 1.f,

		-1.f, -1.f, 1.f,
		0.f, 0.f, 1.f,

		1.f, -1.f, 1.f,
		1.f, 0.f, 1.f,
	};

	const int vtxStride = sizeof(float) * 6;
	static unsigned short indices[] = {
		// quad 0,1,2,3
		0, 2, 1,
		0, 3, 2,
		// quad 4,7,6,5
		4, 6, 7,
		4, 5, 6,
		// quad 0,3,7,4
		0, 7, 3,
		0, 4, 7,
		// quad 1,5,6,2
		1, 6, 5,
		1, 2, 6,
		// quad 2,6,7,3
		2, 7, 6,
		2, 3, 7,
		// quad 0,4,5,1
		0, 5, 4,
		0, 1, 5,
	};

	return std::make_shared<Geom>(
		ARRAY_SIZE(verts)/6, &verts[0],
		ARRAY_SIZE(indices), &indices[0],
		vtxStride, GL_TRIANGLES,
		std::vector<GeomBindPair>{ {GEOM_Pos, 3, 0}, {GEOM_Uv, 3, 3 * sizeof(float)} }
	);
}

