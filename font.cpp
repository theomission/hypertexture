#include <memory>
#include "render.hh"
#include "font.hh"
#include "common.hh"
#include "commonmath.hh"
#include "camera.hh"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.hh"

extern Screen g_screen;

static stbtt_bakedchar g_cdata['z' - ' ' + 1];
static GLuint g_texFont;
static std::shared_ptr<ShaderInfo> g_fontShader;

////////////////////////////////////////////////////////////////////////////////
int font_Init()
{
	// init program
	if(!g_texFont)
	{
		FILE* fp = fopen("Arial.ttf", "rb");
		if(!fp)
			return 0;
		fseek(fp, 0, SEEK_END);
		size_t fileSize = ftell(fp);
		rewind(fp);

		unsigned char *buffer = (unsigned char*)malloc(fileSize);

		size_t numRead = fread(buffer, 1, fileSize, fp);
		fclose(fp);
		if(numRead != fileSize)
		{
			free(buffer);
			return 0;
		}

		unsigned char tempBitmap[512*512];
		stbtt_BakeFontBitmap(buffer, 0, 32.0f, tempBitmap, 512, 512, ' ', 'z' - ' ' + 1, g_cdata);
		glGenTextures(1, &g_texFont);
		glBindTexture(GL_TEXTURE_2D, g_texFont);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512, 512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, tempBitmap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		free(buffer);
	}

	if(!g_fontShader)
		g_fontShader = render_CompileShader("shaders/font.glsl");

	return 1;
}

void font_Print(float x, float y, const char* str, const Color& color, float size)
{
	static GLint s_texLoc ;
	static int first = 1;
	if(first)
	{
		s_texLoc = glGetUniformLocation(g_fontShader->m_program, "fontTex");
		first = 0;
	}
	GLint posLoc = g_fontShader->m_attrs[GEOM_Pos];
	GLint uvLoc = g_fontShader->m_attrs[GEOM_Uv];
	GLint mvpLoc = g_fontShader->m_uniforms[BIND_Mvp] ;
	GLint colorLoc = g_fontShader->m_uniforms[BIND_Color] ;

	glUseProgram(g_fontShader->m_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_texFont);
	glUniform1i(s_texLoc, 0);
	glUniformMatrix4fv(mvpLoc, 1, 0, g_screen.m_proj.m);
	glUniform3fv(colorLoc, 1, &color.r);

	const float scale = size / 32.f;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_TRIANGLES);
	static const float kInvW = 1.f/512.f;
	static const float kInvH = 1.f/512.f;
	while(*str)
	{
		if(*str == ' ')
			x += 10.f*scale;
		else if(*str >= ' ' && *str <= 'z') {
   			stbtt_bakedchar *b = g_cdata + (*str - ' ');

			float pen_x = (x + scale*b->xoff);
			float pen_y = (y + scale*b->yoff);

			float x0 = pen_x;
			float x1 = pen_x + (b->x1 - b->x0) * scale;
			float y0 = pen_y;
			float y1 = pen_y + (b->y1 - b->y0) * scale;

			float s0 = b->x0 * kInvW;
			float t0 = b->y0 * kInvH;
			float s1 = b->x1 * kInvW;
			float t1 = b->y1 * kInvH;
			glVertexAttrib2f(uvLoc, s0, t0); glVertexAttrib2f(posLoc, x0, y0);
			glVertexAttrib2f(uvLoc, s1, t0); glVertexAttrib2f(posLoc, x1, y0);
			glVertexAttrib2f(uvLoc, s1, t1); glVertexAttrib2f(posLoc, x1, y1);

			glVertexAttrib2f(uvLoc, s1, t1); glVertexAttrib2f(posLoc, x1, y1);
			glVertexAttrib2f(uvLoc, s0, t1); glVertexAttrib2f(posLoc, x0, y1);
			glVertexAttrib2f(uvLoc, s0, t0); glVertexAttrib2f(posLoc, x0, y0);
			x += b->xadvance * scale;
		}
		++str;
	}
	glEnd();
	glDisable(GL_BLEND);
	checkGlError("font_Print");
}

void font_GetDims(const char* str, float size, float *width, float *height)
{
	float x = 0.f;
	float y = 0.f;
	float scale = size/32.f;
	while(*str)
	{
		if(*str == ' ')
			x += 10.f * scale;
		else if(*str >= ' ' && *str <= 'z') {
			stbtt_bakedchar *b = &g_cdata[*str - ' '];
			x += b->xadvance * scale;
			float sy = (b->y1 - b->y0) * scale;
			y = Max(y, sy);
		}
		++str;
	}
	if(width) *width = x;
	if(height) *height = y;
}

