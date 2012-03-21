#include <algorithm>
#include "hyper.hh"
#include "common.hh"
#include "render.hh"
#include "task.hh"
#include "vec.hh"
#include "camera.hh"

////////////////////////////////////////////////////////////////////////////////
static std::shared_ptr<ShaderInfo> g_htexShader;
enum HtexUniformLocType {
	HTEXBIND_DensityMap,
};

static std::vector<CustomShaderAttr> g_htexUniforms =
{
	{ HTEXBIND_DensityMap, "densityMap" },
};

static std::shared_ptr<Geom> g_boxGeom;

////////////////////////////////////////////////////////////////////////////////
static std::shared_ptr<Geom> CreateHypertextureBoxGeom();
void hyper_Init()
{
	if(!g_htexShader)
		g_htexShader = render_CompileShader("shaders/hypertexture.glsl", g_htexUniforms);
	g_boxGeom = CreateHypertextureBoxGeom();
}

////////////////////////////////////////////////////////////////////////////////
Hypertexture::Hypertexture(int numCells, GenFuncType genFunc)
	: m_numCells(numCells)
	, m_ready(false)
	, m_genFunc(genFunc)
	, m_tex(0)
{
	Compute();
}

Hypertexture::~Hypertexture()
{
	glDeleteTextures(1, &m_tex);
}

void Hypertexture::Compute()
{
	const int numCells = m_numCells;
	auto cells = std::make_shared<std::vector<float>>(numCells*numCells*numCells);
	auto runFunc = [cells, numCells, m_genFunc]() {
		float *data = &(*cells)[0];
		float inc = 1.f / numCells;
		vec3 coord(0,0,0);
		for(int z = 0, offset = 0; z < numCells; ++z)
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
	};
	
	auto completeFunc = [cells, numCells, &m_ready, &m_tex]() {
		auto range = std::minmax_element(cells->begin(), cells->end());
		float invScale = 1.f/(*range.second - *range.first);
		float minVal = *range.first;
		std::vector<unsigned char> texels(cells->size());
		std::transform(cells->begin(), cells->end(), texels.begin(),
			[minVal, invScale](float density) 
		{
			float val = (density - minVal) * invScale;
			return static_cast<unsigned char>(255 * Clamp(val, 0.f, 1.f));
		});
		// create opengl textures
		glGenTextures(1, &m_tex);
		glBindTexture(GL_TEXTURE_3D, m_tex);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, numCells, numCells, numCells, 0, GL_RED,
			GL_UNSIGNED_BYTE, &texels[0]);

		checkGlError("Creating hypertexture");
		m_ready = true;

	};

	task_AppendTask(std::make_shared<Task>(nullptr, completeFunc, runFunc));
}

void Hypertexture::Render(const Camera& camera, float scale)
{
	if(!m_ready) return;
	const ShaderInfo* shader = g_htexShader.get();

	mat4 model = MakeScale(scale,scale,scale) * MakeTranslation(-0.5f, -0.5f, -0.5f);
	mat4 mvp = camera.GetProj() * (camera.GetView() * model);

	GLint mvpLoc = shader->m_uniforms[BIND_Mvp];
	GLint densityMapLoc = shader->m_custom[HTEXBIND_DensityMap];

	glUseProgram(shader->m_program);
	glUniformMatrix4fv(mvpLoc, 1, 0, mvp.m);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, m_tex);
	glUniform1i(densityMapLoc, 0);

	g_boxGeom->Render(*shader);
}

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
		0, 1, 2,
		0, 2, 3,
		// quad 4,7,6,5
		4, 7, 6,
		4, 6, 5,
		// quad 0,3,7,4
		0, 3, 7,
		0, 7, 4,
		// quad 1,5,6,2
		1, 5, 6,
		1, 6, 2,
		// quad 2,6,7,3
		2, 6, 7,
		2, 7, 3,
		// quad 0,4,5,1
		0, 4, 5,
		0, 5, 1,
	};

	return std::make_shared<Geom>(
		ARRAY_SIZE(verts)/6, &verts[0],
		ARRAY_SIZE(indices), &indices[0],
		vtxStride, GL_TRIANGLES,
		std::vector<GeomBindPair>{ {GEOM_Pos, 3, 0}, {GEOM_Uv, 3, 3 * sizeof(float)} }
	);
}

