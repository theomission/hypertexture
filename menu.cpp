#include <SDL/SDL.h>
#include <cmath>
#include <algorithm>
#include <sstream>
#include "menu.hh"
#include "render.hh"
#include "common.hh"
#include "font.hh"
#include "commonmath.hh"
#include "matrix.hh"
#include "camera.hh"

////////////////////////////////////////////////////////////////////////////////
static std::shared_ptr<TopMenuItem> g_top;
static std::vector<std::shared_ptr<MenuItem>> g_itemStack;
static std::vector<std::shared_ptr<MenuItem>> g_updating;

constexpr static float kFontSize = 18.f;
constexpr static float kHSpace = 10.f;
constexpr static float kVSpace = 15.f;
static const Color kTextColor = {0.2,0.6f,0.2f};
static const Color kTextSelColor = {0.4f,0.6f,0.2f};
static const Color kTextActiveColor = {0.8f,0.8f,0.2f};
static const Color kColWhite = {1,1,1};
	
constexpr static float kInc = 1.f;
constexpr static float kLargeInc = 10.f;
constexpr static float kSmallInc = 0.1f;

static std::shared_ptr<ShaderInfo> g_menuShader;
static std::shared_ptr<ShaderInfo> g_normalViewShader;

enum NormalViewUniformType {
	NORMALBIND_Stipple,
	NORMALBIND_NUM
};
static std::vector<CustomShaderAttr> g_normalViewUniformNames =
{
	{ NORMALBIND_Stipple, "stipple"}
};

////////////////////////////////////////////////////////////////////////////////
static void menu_ActivateMenuItem(const std::shared_ptr<MenuItem>& item);
static void menu_DeactivateMenuItem();

////////////////////////////////////////////////////////////////////////////////
// Utility drawing functions
static void menu_DrawColoredQuad(float x, float y, float w, float h, float border, 
	const Color& color, const Color& borderColor)
{
	glUseProgram(g_menuShader->m_program);
	glUniformMatrix4fv(g_menuShader->m_uniforms[BIND_Mvp], 1, 0, g_screen.m_proj.m);
	GLint locPos = g_menuShader->m_attrs[GEOM_Pos];
	GLint locColor = g_menuShader->m_uniforms[BIND_Color];
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

	checkGlError("menu_DrawColoredQuad");
}

static void menu_DrawQuad(float x, float y, float w, float h, float border)
{
	static const Color kDefBorder = {0.1f, 0.4f, 0.1f};
	static const Color kDefBack = {0.05f, 0.13f, 0.05f};
	menu_DrawColoredQuad(x,y,w,h,border,kDefBack,kDefBorder);
}


////////////////////////////////////////////////////////////////////////////////
// MenuItem
MenuItem::MenuItem(const std::string& name)
	: m_name(name)
	, m_state(0)
	, m_titleColor {0.2, 0.6, 0.2}
	, m_x(0), m_y(0)
	, m_w(0), m_h(0)
{
}
	
void MenuItem::GetTitleDims(float &w, float &h) 
{
	font_GetDims(m_name.c_str(), kFontSize, &w, &h);
}

void MenuItem::DrawTitle(float x, float y, const Color& color) 
{
	font_Print(x, y, m_name.c_str(), color, kFontSize);
}
	
////////////////////////////////////////////////////////////////////////////////
// ButtonMenuItem
ButtonMenuItem::ButtonMenuItem(const std::string& name, std::function<void()> activate)
	: MenuItem(name)
	, m_activate(activate)
	, m_activeTimer(0.f)
{
}

bool ButtonMenuItem::CanActivate() 
{
	return m_activeTimer <= 0.f;
}
	
bool ButtonMenuItem::RequiresUpdate() {
	return (m_activeTimer > 0.f);
}

void ButtonMenuItem::OnActivate()
{
	if(m_activeTimer > 0.f) return;
	if(m_activate) m_activate();
	m_activeTimer = 0.25f;
	menu_DeactivateMenuItem();
}

void ButtonMenuItem::OnUpdate(float dt)
{
	if(m_activeTimer > 0.f)
		m_activeTimer -= dt;
}
	
void ButtonMenuItem::DrawTitle(float x, float y, const Color& color) 
{
	static const Color kPushedCol = {0.8f, 0.8f, 0.2f};
	const Color& curCol = (m_activeTimer > 0.f) ? kPushedCol : color;
	MenuItem::DrawTitle(x,y,curCol);
}

////////////////////////////////////////////////////////////////////////////////
// BoolMenuItem
BoolMenuItem::BoolMenuItem(const std::string& name, int *ival)
	: MenuItem(name)
	, m_get([=](){ return bool(*ival); })
	, m_set([=](bool value) { *ival = value ? 1 : 0; })
	, m_activeTimer(0.f)
{
}

BoolMenuItem::BoolMenuItem(const std::string& name, bool *bval)
	: MenuItem(name)
	, m_get([=](){ return (*bval); })
	, m_set([=](bool value) { *bval = value; })
	, m_activeTimer(0.f)
{
}

BoolMenuItem::BoolMenuItem(const std::string& name, std::function<bool()> get, std::function<void(bool)> set)
	: MenuItem(name)
	, m_get(get)
	, m_set(set)
	, m_activeTimer(0.f)
{
}
	
bool BoolMenuItem::CanActivate() 
{
	return m_activeTimer <= 0.f;
}
	
bool BoolMenuItem::RequiresUpdate() {
	return (m_activeTimer > 0.f);
}

void BoolMenuItem::OnActivate()
{
	if(m_activeTimer > 0.f) return;
	bool val = m_get();
	val = !val;
	m_set(val);
	m_activeTimer = 0.25f;
	menu_DeactivateMenuItem();
}

void BoolMenuItem::OnUpdate(float dt) 
{
	if(m_activeTimer > 0.f)
		m_activeTimer -= dt;
}

void BoolMenuItem::GetTitleDims(float &w, float &h) 
{
	float exw, exh;
	font_GetDims(" [X]", kFontSize, &exw, &exh);

	float dimx, dimy;
	font_GetDims(GetName().c_str(), kFontSize, &dimx, &dimy);

	w = dimx + exw;
	h = Max(dimy, exh);
}

void BoolMenuItem::DrawTitle(float x, float y, const Color& col) 
{
	static const Color kPushedCol = {0.8f, 0.8f, 0.2f};
	const Color& curCol = (m_activeTimer > 0.f) ? kPushedCol : col;
	float dimx, dimy;
	font_GetDims(GetName().c_str(), kFontSize, &dimx, &dimy);
	font_Print(x, y, GetName().c_str(), curCol, kFontSize);
	x += dimx;
	if(m_get()) font_Print(x, y, " [X]", curCol, kFontSize);
	else font_Print(x, y, " [ ]", curCol, kFontSize);
}

////////////////////////////////////////////////////////////////////////////////
SubmenuMenuItem::SubmenuMenuItem(const std::string& name)
	: MenuItem(name)
	, m_pos(0)
{
}
SubmenuMenuItem::SubmenuMenuItem(const std::string& name,
	const std::vector<std::shared_ptr<MenuItem>>& children)
	: MenuItem(name)
	, m_children(children)
	, m_pos(0)
{
}
SubmenuMenuItem::SubmenuMenuItem(const std::string& name,
	std::vector<std::shared_ptr<MenuItem>>&& children)
	: MenuItem(name)
	, m_pos(0)
{
	m_children.swap(children);
}

void SubmenuMenuItem::InsertChild(int index, const std::shared_ptr<MenuItem>& item)
{
	m_children.insert(m_children.begin() + index, item);
}

void SubmenuMenuItem::AppendChild(const std::shared_ptr<MenuItem>& item)
{
	m_children.push_back(item);
}

void SubmenuMenuItem::SetSelection(int idx)
{
	m_pos = Clamp(idx, 0, int(m_children.size() - 1));
}
	
void SubmenuMenuItem::OnActivate()
{
	m_w = 0.f; m_h = 0.f;
	for(auto child: m_children)
	{
		float titlew, titleh;
		child->GetTitleDims(titlew, titleh);
		titlew += 2*kHSpace;
		m_w = Max(m_w, titlew);
		m_h += titleh + kVSpace;
	}
	m_h += kVSpace;

	// in a scrolling environment, this needs to be updated on scroll
	float penx = m_x + m_w - 8;
	float peny = m_y + 4;
	for(auto child: m_children)
	{
		child->m_x = penx;
		child->m_y = peny;

		float titlew,titleh;
		child->GetTitleDims(titlew, titleh);
		peny += titleh + kVSpace;
	}
}

void SubmenuMenuItem::Render(const Camera& curCamera)
{
	menu_DrawQuad(m_x, m_y, m_w, m_h, 1.f);

	float penx = m_x + kHSpace;
	float peny = m_y + kVSpace;
	for(int i = 0, c = m_children.size(); i < c; ++i)
	{
		auto child = m_children[i];

		float titlew,titleh;
		child->GetTitleDims(titlew, titleh);
		peny += titleh;
		const Color* col = (i == m_pos) ? &kTextSelColor : &kTextColor;
		if(child->HasFlag(MENUSTATE_Active)) col = &kTextActiveColor;
		child->DrawTitle(penx, peny, *col);
		peny += kVSpace;
	}

	for(auto child: m_children)
		if(child->HasFlag(MENUSTATE_Active))
			child->Render(curCamera);
}

bool SubmenuMenuItem::OnKey(int key, int mod)
{
	auto selected = GetSelected();
	// TODO scrolling
	switch(key)
	{
		case SDLK_UP:
			{
				if(HasFlag(MENUSTATE_BaseItem) && m_pos == 0)
				{
					menu_DeactivateMenuItem();
					break;
				}

				SetSelection(m_pos - 1);
			}
			break;
		case SDLK_DOWN:
			{
				SetSelection(m_pos + 1);
			}
			break;
		case SDLK_PAGEUP:
			{
				SetSelection(m_pos - g_screen.m_height / 20.f);
			}
			break;
		case SDLK_PAGEDOWN:
			{
				SetSelection(m_pos + g_screen.m_height / 20.f);
			}
			break;
		case SDLK_LEFT:
			menu_DeactivateMenuItem();
			break;
		case SDLK_RIGHT:
			menu_ActivateMenuItem(selected);
			break;
		case SDLK_RETURN:
			menu_ActivateMenuItem(selected);
			break;
		case SDLK_BACKSPACE:
			menu_DeactivateMenuItem();
			break;
		default:
			break;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
TopMenuItem::TopMenuItem()
	: SubmenuMenuItem("top")
{}

TopMenuItem::TopMenuItem(const std::vector<std::shared_ptr<MenuItem>>& children)
	: SubmenuMenuItem("top", children)
{}

TopMenuItem::TopMenuItem(std::vector<std::shared_ptr<MenuItem>>&& children)
	: SubmenuMenuItem("top", children)
{}

bool TopMenuItem::OnKey(int key, int mod)
{
	auto selected = GetSelected();
	switch(key)
	{
		case SDLK_UP:
			break;
		case SDLK_DOWN:
			menu_ActivateMenuItem(selected);
			break;
		case SDLK_LEFT:
			SetSelection(GetPos() - 1);
			break;
		case SDLK_RIGHT:
			SetSelection(GetPos() + 1);
			break;
		case SDLK_RETURN:
			menu_ActivateMenuItem(selected);
			break;
		case SDLK_BACKSPACE:
			menu_DeactivateMenuItem();
			break;
		default:
			break;
	}
	return true;
}
	
void TopMenuItem::OnActivate()
{
	m_w = g_screen.m_width;
	m_h = 0.f;
	for(auto child: GetChildren())
	{
		// draw each item across the top of the screen.
		float titlew,titleh;
		child->GetTitleDims(titlew, titleh);
		m_h = Max(titleh, m_h);
	}
	m_h += 2.f * kVSpace;
	
	float x = kHSpace, y = m_h-8.f;
	for(auto child: GetChildren())
	{
		float titlew, titleh;
		child->GetTitleDims(titlew, titleh);
		child->m_x = x - kHSpace + 8.f;
		child->m_y = y;
		x += titlew;
	}
}

void TopMenuItem::Render(const Camera& curCamera)
{
	menu_DrawQuad(0.f, 0.f, m_w, m_h, 1.f);
	float x =  kHSpace; 
	float y = m_h - kVSpace;

	for(int i = 0, c = GetChildren().size(); i < c; ++i)	
	{
		auto child = GetChildren()[i];
		float titlew,titleh;
		child->GetTitleDims(titlew, titleh);
		const Color* col = (GetPos() == i) ? &kTextSelColor : &kTextColor;
		if(child->HasFlag(MENUSTATE_Active)) col = &kTextActiveColor;
		child->DrawTitle(x, y, *col);
		x += 2.f * kHSpace + titlew;
	}

	for(auto child: GetChildren())
		if(child->HasFlag(MENUSTATE_Active))
			child->Render(curCamera);
}

////////////////////////////////////////////////////////////////////////////////
IntSliderMenuItem::IntSliderMenuItem(const std::string& name,
	std::function<int()> get,
	std::function<void(int)> set,
	int scale,
	const Limits<int>& lm)
	: MenuItem(name)
	, m_get(get)
	, m_set(set) 
	, m_limits(lm)
	, m_scale(Max(scale,1))
{
	UpdateData();
}
	
IntSliderMenuItem::IntSliderMenuItem(const std::string& name,
	int* ival,
	int scale,
	const Limits<int>& lm)
	: MenuItem(name)
	, m_get([=](){ return *ival; })
	, m_set([=](int val){ *ival = val; }) 
	, m_limits(lm)
	, m_scale(Max(scale,1))
{
	UpdateData();
}
	
void IntSliderMenuItem::UpdateData()
{
	std::stringstream str;
	str << m_get();
	m_str = str.str();

	float w,h;
	font_GetDims(m_str.c_str(), kFontSize, &w, &h);
	w+= 2*kHSpace;
	h+= 2*kVSpace;

	m_w = w;
	m_h = h;
}

void IntSliderMenuItem::Render(const Camera& curCamera)
{
	menu_DrawQuad(m_x, m_y, m_w, m_h, 1.f);
	font_Print(m_x + kHSpace, m_y + m_h - kVSpace, m_str.c_str(), kColWhite, kFontSize);
}

bool IntSliderMenuItem::OnKey(int key, int mod) 
{
	switch(key)
	{
		case SDLK_UP:
			m_set(m_limits(m_get() + m_scale));
			UpdateData();
			break;
		case SDLK_DOWN:
			m_set(m_limits(m_get() - m_scale));
			UpdateData();
			break;
		case SDLK_BACKSPACE:
		case SDLK_LEFT:
			menu_DeactivateMenuItem();
			break;
		default:
			break;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
FloatSliderMenuItem::FloatSliderMenuItem(
	const std::string& name,
	std::function<float()> get,
	std::function<void(float)> set, 
	float scale,
	const Limits<float>& lm)
	: MenuItem(name)
	, m_get(get)
	, m_set(set)
	, m_limits(lm)
	, m_scale(scale)
{
	UpdateData();
}

FloatSliderMenuItem::FloatSliderMenuItem(
	const std::string& name,
	float* fval,
	float scale,
	const Limits<float>& lm)
	: MenuItem(name)
	, m_get([=](){ return *fval; })
	, m_set([=](float val) { *fval = val; })
	, m_limits(lm)
	, m_scale(scale)
{
	UpdateData();
}
	
void FloatSliderMenuItem::UpdateData()
{
	std::stringstream str;
	str << m_get();
	m_str = str.str();
	float w,h;
	font_GetDims(m_str.c_str(), kFontSize, &w, &h);
	w+= 2*kHSpace;
	h+= 2*kVSpace;
	m_w = w;
	m_h = h;
}

void FloatSliderMenuItem::Render(const Camera& curCamera)
{
	menu_DrawQuad(m_x, m_y, m_w, m_h, 1.f);
	font_Print(m_x + kHSpace, m_y + m_h - kVSpace, m_str.c_str(), kColWhite, kFontSize);
}

bool FloatSliderMenuItem::OnKey(int key, int mod)
{
	float amt = kInc * m_scale;
	if(mod & KMOD_SHIFT) amt = kLargeInc * m_scale;
	else if(mod & KMOD_CTRL) amt = kSmallInc * m_scale;

	switch(key)
	{
		case SDLK_UP:
			m_set(m_limits(m_get() + amt));
			UpdateData();
			break;
		case SDLK_DOWN:
			m_set(m_limits(m_get() - amt));
			UpdateData();
			break;
		case SDLK_BACKSPACE:
		case SDLK_LEFT:
			menu_DeactivateMenuItem();
			break;
		default:
			break;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
ColorSliderMenuItem::ColorSliderMenuItem(const std::string& name,
	std::function<Color()> get,
	std::function<void(const Color&)> set)
	: MenuItem(name)
	, m_get(get)
	, m_set(set)
	, m_pos(0)
{
	UpdateData();
}

ColorSliderMenuItem::ColorSliderMenuItem(const std::string& name, Color* val)
	: MenuItem(name)
	, m_get([=]() { return *val; })
	, m_set([=](const Color& c) { *val = c; })
	, m_pos(0)
{
	UpdateData();
}

void ColorSliderMenuItem::UpdateData()
{
	float w = kDisplayW;
	float h = kDisplayH + kVSpace;
	float curw,curh;
	font_GetDims("Green: 10000.10000", kFontSize, &curw, &curh);
	w = Max(w, curw);
	h += 3*(curh + kVSpace);

	m_w = w;
	m_h = h;

	Color color = m_get();
	{
		std::stringstream str;
		str << "Red: " << color.r;
		m_strRed = str.str();
	}
	{
		std::stringstream str;
		str << "Green: " << color.g;
		m_strGreen = str.str();
	}
	{
		std::stringstream str;
		str << "Blue: " << color.b;
		m_strBlue = str.str();
	}
}

void ColorSliderMenuItem::Render(const Camera& curCamera)
{
	menu_DrawQuad(m_x, m_y, m_w, m_h, 1.f);

	// draw the color itself
	static const Color kDefBorder = {0.2f,0.2f,0.2f};
	Color color = m_get();
	menu_DrawColoredQuad(m_x+1.f, m_y+1.f, kDisplayW - 2.f, kDisplayH - 2.f, 1.f, color, kDefBorder);
	float x = m_x + kHSpace;
	float y = m_y + kDisplayH ;

	// draw the text values
	float curw, curh;
	font_GetDims(m_strRed.c_str(), kFontSize, &curw, &curh);
	y += kVSpace + curh;
	font_Print(x, y, m_strRed.c_str(), COLSLIDE_Red == m_pos ? kColWhite : kTextColor, kFontSize);
	
	font_GetDims(m_strGreen.c_str(), kFontSize, &curw, &curh);
	y += kVSpace + curh;
	font_Print(x, y, m_strGreen.c_str(), COLSLIDE_Green == m_pos ? kColWhite : kTextColor, kFontSize);
	
	font_GetDims(m_strBlue.c_str(), kFontSize, &curw, &curh);
	y += kVSpace + curh;
	font_Print(x, y, m_strBlue.c_str(), COLSLIDE_Blue == m_pos ? kColWhite : kTextColor, kFontSize);
}

bool ColorSliderMenuItem::OnKey(int key, int mod)
{
	static const float kInc = 0.05f;
	static const float kSmallInc = 0.001f;
	static const float kLargeInc = 0.1f;

	float inc = kInc;
	if(mod & KMOD_SHIFT) inc = kLargeInc;
	else if(mod & KMOD_CTRL) inc = kSmallInc;

	float incAmt = 0.f;
	switch(key)
	{
		case SDLK_UP:
			m_pos = Max(0, m_pos - 1);
			break;
		case SDLK_DOWN:
			m_pos = Min(COLSLIDE_NUM-1, m_pos + 1);
			break;
		case SDLK_LEFT:
			incAmt = -inc;
			break;
		case SDLK_RIGHT:
			incAmt = inc;
			break;
		case SDLK_BACKSPACE:
			menu_DeactivateMenuItem();
			break;
		default:
			break;
	}

	Color cur = m_get();
	Color old = cur;
	switch(m_pos)
	{
		case COLSLIDE_Red:
			cur.r = Clamp(cur.r + incAmt, 0.f, 1.f);
			break;
		case COLSLIDE_Green:
			cur.g = Clamp(cur.g + incAmt, 0.f, 1.f);
			break;
		case COLSLIDE_Blue:
			cur.b = Clamp(cur.b + incAmt, 0.f, 1.f);
			break;
		default:break;
	}
	if(old != cur)
		m_set(cur);
	UpdateData();
	return true;
}

////////////////////////////////////////////////////////////////////////////////
VecSliderMenuItem::VecSliderMenuItem(const std::string& name,
	std::function<vec3()> get,
	std::function<void(const vec3&)> set,
	float scale,
	const Limits<vec3>& lm)
	: MenuItem(name)
	, m_get(get)
	, m_set(set)
	, m_limits(lm)
	, m_scale(scale)
	, m_pos(0)
{
	UpdateData();
}

VecSliderMenuItem::VecSliderMenuItem(const std::string& name, vec3* val,
	float scale,
	const Limits<vec3>& lm)
	: MenuItem(name)
	, m_get([=]() { return *val; })
	, m_set([=](const vec3& v) { *val = v; })
	, m_limits(lm)
	, m_scale(scale)
	, m_pos(0)
{
	UpdateData();
}
	
void VecSliderMenuItem::DrawNormalVecView(const Camera& curCamera, 
	float x, float y, float w, float h, const vec3& normal)
{
	GLint mvpLoc = g_normalViewShader->m_uniforms[BIND_Mvp];
	GLint colorLoc = g_normalViewShader->m_attrs[GEOM_Color];
	GLint posLoc = g_normalViewShader->m_attrs[GEOM_Pos];
	GLint coordLoc = g_normalViewShader->m_attrs[GEOM_Uv];
	GLint stippleLoc = g_normalViewShader->m_custom[NORMALBIND_Stipple];

	ViewportState vpState(x, g_screen.m_height-y-h, w, h);
	ScissorState scState(x, g_screen.m_height-y-h, w, h);

	glClearColor(0.05f,0.05f,0.05f,1.f);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	Camera localcamera = curCamera;
	localcamera.SetPos(-4.f * curCamera.GetViewframe().m_fwd);
	localcamera.Compute();
	mat4 proj = Compute3DProj(30.f, w/h, 0.1, 5);
	mat4 mvp = proj * localcamera.GetView();

	glUseProgram(g_normalViewShader->m_program);
	glUniformMatrix4fv(mvpLoc, 1, 0, mvp.m);
	glUniform1f(stippleLoc, 0);

	glBegin(GL_LINES);
	// axes
	glVertexAttrib3f(colorLoc, 1, 0, 0);
	glVertexAttrib1f(coordLoc, 0); glVertexAttrib3f(posLoc, 0, 0, 0);
	glVertexAttrib1f(coordLoc, 1); glVertexAttrib3f(posLoc, 1, 0, 0);
	
	glVertexAttrib3f(colorLoc, 0.2, 0, 0);
	glVertexAttrib1f(coordLoc, 0); glVertexAttrib3f(posLoc, 0, 0, 0);
	glVertexAttrib1f(coordLoc, 1); glVertexAttrib3f(posLoc, -1, 0, 0);
	
	glVertexAttrib3f(colorLoc, 0, 1, 0);
	glVertexAttrib1f(coordLoc, 0); glVertexAttrib3f(posLoc, 0, 0, 0);
	glVertexAttrib1f(coordLoc, 1); glVertexAttrib3f(posLoc, 0, 1, 0);
	
	glVertexAttrib3f(colorLoc, 0, 0.2, 0);
	glVertexAttrib1f(coordLoc, 0); glVertexAttrib3f(posLoc, 0, 0, 0);
	glVertexAttrib1f(coordLoc, 1); glVertexAttrib3f(posLoc, 0, -1, 0);
	
	glVertexAttrib3f(colorLoc, 0, 0, 1);
	glVertexAttrib1f(coordLoc, 0); glVertexAttrib3f(posLoc, 0, 0, 0);
	glVertexAttrib1f(coordLoc, 1); glVertexAttrib3f(posLoc, 0, 0, 1);
	
	glVertexAttrib3f(colorLoc, 0, 0, 0.2);
	glVertexAttrib1f(coordLoc, 0); glVertexAttrib3f(posLoc, 0, 0, 0);
	glVertexAttrib1f(coordLoc, 1); glVertexAttrib3f(posLoc, 0, 0, -1);

	// vector
	glVertexAttrib3f(colorLoc, 0.7, 0.7, 0.7);
	glVertexAttrib3f(posLoc, 0, 0, 0);
	glVertexAttrib3fv(posLoc, &normal.x);
	glEnd();

	// helper lines
	static Color kColNeg = {0.2,0.2,0.2}, kColPos = {0.4,0.4,0.4};

	glUniform1f(stippleLoc, 0.1f);
	glBegin(GL_LINES);
	glVertexAttrib3fv(colorLoc, normal.x > 0 ? &kColPos.r : &kColNeg.r); 
	glVertexAttrib1f(coordLoc, 0); glVertexAttrib3f(posLoc, 0, normal.y, 0);
	glVertexAttrib1f(coordLoc, normal.x); glVertexAttrib3f(posLoc, normal.x, normal.y, 0);

	glVertexAttrib3fv(colorLoc, normal.y > 0 ? &kColPos.r : &kColNeg.r); 
	glVertexAttrib1f(coordLoc, 0); glVertexAttrib3f(posLoc, normal.x, 0, 0);
	glVertexAttrib1f(coordLoc, normal.y); glVertexAttrib3f(posLoc, normal.x, normal.y, 0);
	
	glVertexAttrib3fv(colorLoc, normal.z > 0 ? &kColPos.r : &kColNeg.r); 
	glVertexAttrib1f(coordLoc, 0); glVertexAttrib3f(posLoc, normal.x, normal.y, 0);
	glVertexAttrib1f(coordLoc, normal.z); glVertexAttrib3fv(posLoc, &normal.x);
	
	glEnd();

	glLineWidth(1.f);
	checkGlError("menu_DrawNormalVecView");
}

void VecSliderMenuItem::UpdateData()
{
	float w = kDisplaySize;
	float h = kDisplaySize + kVSpace;
	
	float curw,curh;
	font_GetDims(kRotateX, kFontSize, &curw, &curh);
	w = Max(w, curw);
	h += 3*(curh + kVSpace);
	font_GetDims("X: 10000.10000", kFontSize, &curw, &curh);
	w = Max(w, curw);
	h += 4*(curh + kVSpace);

	m_w = w;
	m_h = h;

	vec3 vec = m_get();
	{
		std::stringstream str;
		str << "X: " << vec.x;
		m_strX = str.str();
	}
	{
		std::stringstream str;
		str << "Y: " << vec.y;
		m_strY = str.str();
	}
	{
		std::stringstream str;
		str << "Z: " << vec.z;
		m_strZ = str.str();
	}
	{
		std::stringstream str;
		str << "Length: " << Length(vec);
		m_strLength = str.str();
	}
}

void VecSliderMenuItem::Render(const Camera& curCamera)
{
	menu_DrawQuad(m_x, m_y, m_w, m_h, 1.f);
	vec3 val = m_get();
	vec3 nval = Normalize(val);

	DrawNormalVecView(curCamera, m_x+1.f, m_y+1.f, kDisplaySize-2.f, kDisplaySize-2.f, nval);
	float x = m_x + kHSpace;
	float y = m_y + kDisplaySize + kVSpace;

	float curw, curh;
	font_GetDims(kRotateX, kFontSize, &curw, &curh);
	y += curh;
	font_Print(x, y, kRotateX, VECSLIDE_RotateX == m_pos ? kColWhite : kTextColor, kFontSize);

	font_GetDims(kRotateY, kFontSize, &curw, &curh);
	y += kVSpace + curh;
	font_Print(x, y, kRotateY, VECSLIDE_RotateY == m_pos ? kColWhite : kTextColor, kFontSize);
	
	font_GetDims(kRotateZ, kFontSize, &curw, &curh);
	y += kVSpace + curh;
	font_Print(x, y, kRotateZ, VECSLIDE_RotateZ == m_pos ? kColWhite : kTextColor, kFontSize);

	font_GetDims(m_strX.c_str(), kFontSize, &curw, &curh);
	y += kVSpace + curh;
	font_Print(x, y, m_strX.c_str(), VECSLIDE_X == m_pos ? kColWhite : kTextColor, kFontSize);
	
	font_GetDims(m_strY.c_str(), kFontSize, &curw, &curh);
	y += kVSpace + curh;
	font_Print(x, y, m_strY.c_str(), VECSLIDE_Y == m_pos ? kColWhite : kTextColor, kFontSize);
	
	font_GetDims(m_strZ.c_str(), kFontSize, &curw, &curh);
	y += kVSpace + curh;
	font_Print(x, y, m_strZ.c_str(), VECSLIDE_Z == m_pos ? kColWhite : kTextColor, kFontSize);

	font_GetDims(m_strLength.c_str(), kFontSize, &curw, &curh);
	y += kVSpace + curh;
	font_Print(x, y, m_strLength.c_str(), VECSLIDE_Length == m_pos ? kColWhite : kTextColor, kFontSize);
}

bool VecSliderMenuItem::OnKey(int key, int mod) 
{

	float inc = m_scale * kInc;
	if(mod & KMOD_SHIFT) inc = m_scale * kLargeInc;
	else if(mod & KMOD_CTRL) inc = m_scale * kSmallInc;

	float incAmt = 0.f;
	switch(key)
	{
		case SDLK_UP:
			m_pos = Max(0, m_pos - 1);
			break;
		case SDLK_DOWN:
			m_pos = Min(VECSLIDE_NUM-1, m_pos + 1);
			break;
		case SDLK_LEFT:
			incAmt = -inc;
			break;
		case SDLK_RIGHT:
			incAmt = inc;
			break;
		case SDLK_BACKSPACE:
			menu_DeactivateMenuItem();
			break;
		default:
			break;
	}

	vec3 val = m_get();
	vec3 old = val;
	float radAmt = (M_PI / 180.f) * incAmt;
	mat4 rotmat;
	static const vec3 kXAxis = {1,0,0};
	static const vec3 kYAxis = {0,1,0};
	static const vec3 kZAxis = {0,0,1};
	switch(m_pos)
	{
		case VECSLIDE_RotateX:
			rotmat = RotateAround(kXAxis, radAmt);
			val = TransformVec(rotmat, val);
			break;
		case VECSLIDE_RotateY:
			rotmat = RotateAround(kYAxis, radAmt);
			val = TransformVec(rotmat, val);
			break;
		case VECSLIDE_RotateZ:
			rotmat = RotateAround(kZAxis, radAmt);
			val = TransformVec(rotmat, val);
			break;
		case VECSLIDE_X:
			val.x += incAmt;
			break;
		case VECSLIDE_Y:
			val.y += incAmt;
			break;
		case VECSLIDE_Z:
			val.z += incAmt;
			break;
		case VECSLIDE_Length:
			{
				float len = Length(val);
				val = val * (len+incAmt) / len;
			}
			break;
		default:break;
	}
	vec3 newVal = m_limits(val);
	if(newVal != old)
	{
		m_set(newVal);
		UpdateData();
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
void menu_SetTop(const std::shared_ptr<TopMenuItem>& top)
{
	if(!g_menuShader) 
		g_menuShader = render_CompileShader("shaders/menu.glsl");
	if(!g_normalViewShader)
		g_normalViewShader = render_CompileShader("shaders/normalView.glsl", g_normalViewUniformNames);

	g_itemStack.clear();
	g_updating.clear();

	g_top = top;

	// this flag makes menus that are parented to the top use slightly different
	//  keyboard handling.
	for(auto child : g_top->GetChildren())
		child->SetFlag(MENUSTATE_BaseItem);

	menu_ActivateMenuItem(g_top);
}

void menu_Update(float dt)
{
	for(auto item: g_updating)
		item->OnUpdate(dt);
	auto newEnd = std::remove_if(g_updating.begin(), g_updating.end(), 
		[](const std::shared_ptr<MenuItem>& item) {
			return !item->RequiresUpdate()	;
		}
	);
	g_updating.erase(newEnd, g_updating.end());
}

void menu_Draw(const Camera& curCamera)
{
	if(g_top) g_top->Render(curCamera);
}

static void menu_ActivateMenuItem(const std::shared_ptr<MenuItem>& item)
{
	if(item->CanActivate() && !item->HasFlag(MENUSTATE_Active))
	{
		g_itemStack.push_back(item);
		item->SetFlag(MENUSTATE_Active);
		item->OnActivate();
		if(item->RequiresUpdate())
			g_updating.push_back(item);
	}
}

static void menu_DeactivateMenuItem()
{
	// g_top is always the first item
	if(g_itemStack.size() > 1) 
	{
		auto ptr = g_itemStack.back();
		g_itemStack.pop_back();
		ptr->OnDeactivate();
		ptr->ClearFlag(MENUSTATE_Active);
	}
}

void menu_Key(int key, int mod)
{
	for(int i = g_itemStack.size() - 1; i >= 0; --i)
	{
		auto ptr = g_itemStack[i];
		if(ptr->OnKey(key, mod))
			break;
	}
}

void menu_Mouse(int x, int y, int buttons)
{
	for(int i = g_itemStack.size() - 1; i >= 0; --i)
	{
		auto ptr = g_itemStack[i];
		if(ptr->OnMouse(x, y , buttons))
			break;
	}
}

