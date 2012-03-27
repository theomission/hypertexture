#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>

#include "commonmath.hh"

class Camera;

enum MenuStateFlag
{
	MENUSTATE_Active = 0x1,
	MENUSTATE_BaseItem = 0x2,
};

class MenuItem
{
public:
	MenuItem(const std::string& name);
	virtual ~MenuItem() {}

	void SetFlag(int flag) { m_state |= flag; }
	void ClearFlag(int flag) { m_state &= ~flag; }
	bool HasFlag(int flag) { return m_state & flag; }

	virtual bool CanActivate() { return true; }
	virtual bool RequiresUpdate() { return false; }
	virtual void GetTitleDims(float &w, float &h) ;
	virtual void DrawTitle(float x, float y, const Color& color) ;

	virtual void Render(const Camera& curCamera) { }
	virtual bool OnKey(int key, int mod) { return false; }
	virtual bool OnMouse(int x, int y, int buttons) { return false; } 
	virtual void OnActivate() {}
	virtual void OnDeactivate() { }
	virtual void OnUpdate(float) { }

	const std::string& GetName() const { return m_name; }
private:
	std::string m_name;
	int m_state;
	Color m_titleColor;
public:
	float m_x,m_y;
	float m_w,m_h;
};

class ButtonMenuItem : public MenuItem
{
public:
	ButtonMenuItem(const std::string& name, std::function<void()> activate);
	void OnActivate() ;
	bool CanActivate() ;
	bool RequiresUpdate() ;
	void OnUpdate(float dt) ;
	void DrawTitle(float x, float y, const Color& color) ;
private:
	std::function<void()> m_activate;
	float m_activeTimer;
};

class BoolMenuItem : public MenuItem
{
public:
	BoolMenuItem(const std::string& name, int *ival);	// for use with globals
	BoolMenuItem(const std::string& name, bool *bval);
	BoolMenuItem(const std::string& name, std::function<bool()> get, std::function<void(bool)> set);
	
	void OnActivate() ;
	bool CanActivate() ;
	bool RequiresUpdate() ;
	void OnUpdate(float dt) ;
	void GetTitleDims(float &w, float &h) ;
	void DrawTitle(float x, float y, const Color& color) ;
private:
	std::function<bool()> m_get;
	std::function<void(bool)> m_set;
	float m_activeTimer;
};

class SubmenuMenuItem : public MenuItem
{
public:
	typedef std::vector<std::shared_ptr<MenuItem>> ChildListType;
	SubmenuMenuItem(const std::string& name);
	SubmenuMenuItem(const std::string& name, const std::vector<std::shared_ptr<MenuItem>>& children);
	SubmenuMenuItem(const std::string& name, std::vector<std::shared_ptr<MenuItem>>&& children);

	void Render(const Camera& curCamera) ;
	bool OnKey(int key, int mod) ;
	void OnActivate();

	void InsertChild(int index, const std::shared_ptr<MenuItem>& item);
	void AppendChild(const std::shared_ptr<MenuItem>& item);

	const std::vector<std::shared_ptr<MenuItem>>& GetChildren() const { return m_children; }
	std::vector<std::shared_ptr<MenuItem>>& GetChildren() { return m_children; }

protected:
	std::shared_ptr<MenuItem> GetSelected() const { return m_children.empty() ? nullptr : m_children[m_pos]; }
	void SetSelection(int idx);
	int GetPos() const { return m_pos; }
private:
	std::vector<std::shared_ptr<MenuItem>> m_children;
	int m_pos;
};

class TopMenuItem : public SubmenuMenuItem
{
public:
	TopMenuItem();
	TopMenuItem(const std::vector<std::shared_ptr<MenuItem>>& children);
	TopMenuItem(std::vector<std::shared_ptr<MenuItem>>&& children);

	bool OnKey(int key, int mod) ;
	void Render(const Camera& curCamera) ;
	void OnActivate ();
};


class IntSliderMenuItem : public MenuItem
{
public:
	IntSliderMenuItem(const std::string& name, std::function<int()> get, std::function<void(int)> set, 
		int scale = 1, const Limits<int>& lm = Limits<int>());
	IntSliderMenuItem(const std::string& name, int* ival, 
		int scale = 1, const Limits<int>& lm = Limits<int>());
	
	bool OnKey(int key, int mod) ;
	void Render(const Camera& curCamera) ;
	void OnActivate() { UpdateData(); }
private:
	void UpdateData();
	std::function<int()> m_get;
	std::function<void(int)> m_set;
	Limits<int> m_limits;
	int m_scale;
	std::string m_str;
};

class FloatSliderMenuItem : public MenuItem
{
public:
	FloatSliderMenuItem(const std::string& name, std::function<float()> get, std::function<void(float)> set,
		float scale = 1.f, const Limits<float>& lm = Limits<float>());
	FloatSliderMenuItem(const std::string& name, float* fval, float scale = 1.f,
		const Limits<float>& lm = Limits<float>());
	
	bool OnKey(int key, int mod) ;
	void Render(const Camera& curCamera) ;
	void OnActivate() { UpdateData(); }
private:
	void UpdateData();
	std::function<float()> m_get;
	std::function<void(float)> m_set;
	Limits<float> m_limits;
	float m_scale;
	std::string m_str;
};

class ColorSliderMenuItem : public MenuItem
{
public:
	ColorSliderMenuItem(const std::string& name, Color* val);
	ColorSliderMenuItem(const std::string& name,
		std::function<Color()> get,
		std::function<void(const Color&)> set);
	
	bool OnKey(int key, int mod) ;
	void Render(const Camera& curCamera) ;
	void OnActivate() { UpdateData(); }
private:
	enum ColorSliderPosType {
		COLSLIDE_Red,
		COLSLIDE_Green,
		COLSLIDE_Blue,
		COLSLIDE_NUM,
	};
	void UpdateData();

	std::function<Color()> m_get;
	std::function<void(const Color&)> m_set;
	int m_pos;

	std::string m_strRed;
	std::string m_strGreen;
	std::string m_strBlue;
	
	constexpr static float kDisplayW = 256.f;
	constexpr static float kDisplayH = 50.f;

};

class VecSliderMenuItem : public MenuItem
{
public:
	VecSliderMenuItem(const std::string& name, vec3* val,
		float scale = 1.0f,
		const Limits<vec3>& lm = Limits<vec3>());
	VecSliderMenuItem(const std::string& name,
		std::function<vec3()> get,
		std::function<void(const vec3&)> set,
		float scale,
		const Limits<vec3>& lm = Limits<vec3>());
	
	bool OnKey(int key, int mod) ;
	void Render(const Camera& curCamera) ;
	void OnActivate() { UpdateData(); }
private:
	enum VecSliderPosType {
		VECSLIDE_RotateX,
		VECSLIDE_RotateY,
		VECSLIDE_RotateZ,
		VECSLIDE_X,
		VECSLIDE_Y,
		VECSLIDE_Z,
		VECSLIDE_Length,
		VECSLIDE_NUM,
	};
	void UpdateData();

	void DrawNormalVecView(const Camera& curCamera, float x, float y, float w, float h, const vec3& normal);

	std::function<vec3()> m_get;
	std::function<void(const vec3&)> m_set;
	Limits<vec3> m_limits;
	float m_scale;
	int m_pos;

	std::string m_strX;
	std::string m_strY;
	std::string m_strZ;
	std::string m_strLength;
	
	constexpr static const char* kRotateX = "Rotate around X";
	constexpr static const char* kRotateY = "Rotate around Y";
	constexpr static const char* kRotateZ = "Rotate around Z";
	constexpr static float kDisplaySize = 256.f;
};

void menu_SetTop(const std::shared_ptr<TopMenuItem>& top);
void menu_Draw(const Camera& curCamera);
void menu_Update(float dt);
void menu_Key(int key, int mod);
void menu_Mouse(int x, int y, int buttons);

