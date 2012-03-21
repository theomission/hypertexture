#pragma once

#include "vec.hh"
#include "matrix.hh"
#include "commonmath.hh"

class Screen
{
public:
	Screen(float width, float height) : m_width(width), m_height(height), m_aspect(width/height),
		m_proj(Compute2DProj(width, height, -1.f, 1.f)) {}
	float m_width;
	float m_height;
	float m_aspect;
	mat4 m_proj;
};

class Viewframe
{
public:
	vec3 m_fwd;
	vec3 m_side;
	vec3 m_up;
	vec3 m_pos;
};

enum FrustumPlanesType {
	FRUSTUM_Near,
	FRUSTUM_Far,
	FRUSTUM_Right,
	FRUSTUM_Left,
	FRUSTUM_Top,
	FRUSTUM_Bottom
};

class Camera
{
public:
	Camera(float fov, float aspect, float znear = 1.f, float zfar = 10000.f);

	void TiltBy(float rad);
	void TurnBy(float rad);
	void RollBy(float rad);
	void MoveBy(const vec3& v);

	void LookAt(const vec3& focus, const vec3& eye, const vec3& up);
	void Compute();

	const mat4& GetProj() const { return m_proj; }
	const mat4& GetView() const { return m_view; }
	const Plane& GetFrustum(int plane) const { return m_frustum[plane]; }
	const Viewframe& GetViewframe() const { return m_vf; }
	const vec3& GetPos() const { return m_vf.m_pos; }
	void SetPos(const vec3& v) { m_vf.m_pos = v; }

	float GetFov() const { return m_fov; }
private:
	void ComputeViewFrustum();

	Viewframe m_vf;
	float m_aspect;
	float m_fov;
	float m_znear;
	float m_zfar;
	Plane m_frustum[6];
	mat4 m_view;
	mat4 m_proj;
};

