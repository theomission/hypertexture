#pragma once

#include <GL/glew.h>
#include <memory>
#include <string>
#include <vector>

class vec3;
class Color;
class Plane;
class mat4;
class Geom;
class ShaderInfo;
class CustomShaderAttr;

// common attribute bindings
enum GeomElType {
	GEOM_Pos,
	GEOM_Uv,
	GEOM_Normal,
	GEOM_Color,
	GEOM_NUM,
};

// common uniform bindings
enum CommonBindingsType {
	BIND_Mvp,
	BIND_Model,
	BIND_ModelIT,
	BIND_ModelInv,
	BIND_ModelView,
	BIND_ModelViewIT,
	BIND_ModelViewInv,
	BIND_Sundir,
	BIND_Color,
	BIND_Eyepos,
	BIND_SunColor,
	BIND_Shininess,
	BIND_Ambient,
	BIND_Ka,
	BIND_Kd,
	BIND_Ks,
	BIND_NUM,
};

////////////////////////////////////////////////////////////////////////////////
// function decls
void render_Init();
std::shared_ptr<Geom> render_GenerateSphereGeom(int subdivH, int subdivV);
std::shared_ptr<Geom> render_GenerateBoxGeom();
std::shared_ptr<Geom> render_GeneratePlaneGeom();
void checkGlError(const char* str);
// use these functions to add shader to a global list of shaders that can be recompiled.
std::shared_ptr<ShaderInfo> render_CompileShader(const char* filename);
std::shared_ptr<ShaderInfo> render_CompileShader(const char* filename, const std::vector<CustomShaderAttr>& customSpec);
void render_RefreshShaders();
void render_SetTextureParameters(int sWrap = GL_REPEAT, int tWrap = GL_REPEAT,
	int magFilter = GL_LINEAR, int minFilter = GL_LINEAR_MIPMAP_LINEAR);
void render_SaveScreen(const char* filename);
void render_SaveTGA(const char* filename, int w, int h, unsigned char* bytes);
void render_drawDebugTexture(GLuint dbgTex, bool splitChannels);

////////////////////////////////////////////////////////////////////////////////
class GeomBindPair 
{
public:
	GeomBindPair(int attr, int count, int offset) : m_attr(attr), m_count(count), m_offset(offset) {}
	int m_attr;
	int m_count;
	int m_offset;
} ;

////////////////////////////////////////////////////////////////////////////////
class Geom 
{
public:
	Geom(int numVerts, const float* verts, 
		int numIndices, const unsigned short* indices,
		int vertStride, int glPrimType, 
		const std::vector<GeomBindPair>& elements);
	~Geom();

	// one-off render
	void Render(const ShaderInfo& shader);

	// rendering many times
	void Bind(const ShaderInfo& shader);
	void Submit();
	void Unbind(const ShaderInfo& shader);
private:
	GLuint m_buffer[2];
	int m_stride;
	int m_glPrimType;
	int m_numIndices;
	std::vector<GeomBindPair> m_elements;
};

////////////////////////////////////////////////////////////////////////////////
class CustomShaderAttr
{
public:
	CustomShaderAttr(int id, const char* staticStrName, bool optional = false)
		: m_id(id), m_name(staticStrName), m_optional(optional) {}

	int m_id;
	const char* m_name;
	bool m_optional;
};

////////////////////////////////////////////////////////////////////////////////
class ShaderInfo
{
public:
	ShaderInfo(const std::string& filename);
	ShaderInfo(const std::string& filename, const std::vector<CustomShaderAttr>& customSpec);
	~ShaderInfo();
	
	void Recompile();

	GLuint m_program;
	GLint m_uniforms[BIND_NUM];	
	GLint m_attrs[GEOM_NUM];
	std::vector<CustomShaderAttr> m_customSpec;
	std::vector<GLint> m_custom;
private:
	class ShaderChunk {
	public:
		ShaderChunk(const std::string& filename, int interruptedLine, const char* str, int strLen);
		std::string m_header;
		std::string m_source;
		std::string m_footer;
	};
	void CompileShaderChunks(const std::vector<ShaderChunk>& chunks);
	void CompileShaderSources(const std::string& filename, std::vector<ShaderChunk>& chunks);
	void FindCommonShaderLocs();
	void DeleteProgram();
	const std::string m_filename;
};

////////////////////////////////////////////////////////////////////////////////
class ShaderParams
{
public:
	ShaderParams(const std::shared_ptr<ShaderInfo>& shader);

	enum ParamType {
		P_Float1,
		P_Float2,
		P_Float3,
		P_Float4,
		P_Int1,
		P_Int2,
		P_Int3,
		P_Int4,
		P_Matrix4,
	};

	void AddParam(const char* name, int type, const void* data);
	void Submit();
private:
	struct Param {
		Param(const char* name, int idx, int type, const void* data) 
			: m_name(name), m_customIndex(idx), m_type(type), m_data(data) {}
		const char* m_name;
		int m_customIndex;
		int m_type;
		const void *m_data;
	};

	std::shared_ptr<ShaderInfo> m_shader;
	std::vector<Param> m_params;
};

////////////////////////////////////////////////////////////////////////////////
class Framebuffer
{
public:
	Framebuffer(int width, int height, int layers = 0);
	Framebuffer(Framebuffer&& other);
	~Framebuffer();

	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator=(const Framebuffer&) = delete;

	void AddDepth(bool stencil = false);
	void AddTexture(int internalFormat, int format, int dataType);
	void AddTexture3D(int internalFormat, int format, int dataType);
	GLuint GetTexture(int index) const { return m_tbo[index].tex; }
	void Create();

	void Bind() const;
	void BindLayer(int layer) const;

	void CopyTexture(int index, GLuint destTex, int internalFormat) const;
private:
	struct TexInfo {
		TexInfo(int type_, GLuint tex_) : type(type_), tex(tex_) {}
		int type;
		GLuint tex;
	};
	GLuint m_fbo;
	GLuint m_rboDepth;
	bool m_hasStencil;
	int m_width;
	int m_height;
	int m_layers;
	std::vector<TexInfo> m_tbo;
};

////////////////////////////////////////////////////////////////////////////////
class ViewportState
{
public:
	ViewportState(int x, int y, int w, int h);
	~ViewportState();
private:
	int m_oldViewport[4];
};

////////////////////////////////////////////////////////////////////////////////
class ScissorState
{
public:
	ScissorState(int x, int y, int w, int h);
	~ScissorState();
private:
	int m_oldScissor[4];
	GLboolean m_prevEnabled;
};

