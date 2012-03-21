#include "ui.hh"
#include "render.hh"
#include "commonmath.hh"
#include "camera.hh"

////////////////////////////////////////////////////////////////////////////////
// Extern globals
extern Screen g_screen;

////////////////////////////////////////////////////////////////////////////////
// File-scope globals
static std::shared_ptr<ShaderInfo> g_uiShader; // TODO if there's ever more UI, move this

////////////////////////////////////////////////////////////////////////////////
void ui_Init()
{
	if(!g_uiShader)
		g_uiShader = render_CompileShader("shaders/ui.glsl");
}

void ui_DrawColoredQuad(float x, float y, float w, float h, float border, 
	const Color& color, const Color& borderColor)
{
	glUseProgram(g_uiShader->m_program);
	glUniformMatrix4fv(g_uiShader->m_uniforms[BIND_Mvp], 1, 0, g_screen.m_proj.m);
	GLint locPos = g_uiShader->m_attrs[GEOM_Pos];
	GLint locColor = g_uiShader->m_uniforms[BIND_Color];
	if(border > 0.f)
	{
		glUniform3fv(locColor, 1, &borderColor.r);
		glBegin(GL_TRIANGLE_STRIP);
		glVertexAttrib2f(locPos, x, y);
		glVertexAttrib2f(locPos, x, y+h);
		glVertexAttrib2f(locPos, x+w, y);
		glVertexAttrib2f(locPos, x+w, y+h);
		glEnd();
		x+=border;
		w-=2.f*border;
		y+=border;
		h-=2.f*border;
	}
	glUniform3fv(locColor, 1, &color.r);
	glBegin(GL_TRIANGLE_STRIP);
	glVertexAttrib2f(locPos, x, y);
	glVertexAttrib2f(locPos, x, y+h);
	glVertexAttrib2f(locPos, x+w, y);
	glVertexAttrib2f(locPos, x+w, y+h);
	glEnd();

	checkGlError("ui_DrawColoredQuad");
}

void ui_DrawQuad(float x, float y, float w, float h, float border)
{
	static const Color kDefBorder = {0.1f, 0.4f, 0.1f};
	static const Color kDefBack = {0.05f, 0.13f, 0.05f};
	ui_DrawColoredQuad(x,y,w,h,border,kDefBack,kDefBorder);
}
