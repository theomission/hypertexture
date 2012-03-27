#include "camera.hh"
#include "vec.hh"

////////////////////////////////////////////////////////////////////////////////
// globals
Screen g_screen(1280, 720);

////////////////////////////////////////////////////////////////////////////////
Camera::Camera(float fov, float aspect, float znear, float zfar)
	: m_vf()
	, m_aspect(aspect)
	, m_fov(fov)
	, m_znear(znear)			
	, m_zfar(zfar)
	, m_proj(Compute3DProj(fov, aspect, znear, zfar))
{
	LookAt({0,0,0}, {10,10,10},{0,0,1});
	ComputeViewFrustum();
}
	
void Camera::Compute()
{
	vec3 negSide = -m_vf.m_side;
	vec3 negFwd = -m_vf.m_fwd;

	m_view.m[0] = negSide.x;
	m_view.m[1] = m_vf.m_up.x;
	m_view.m[2] = negFwd.x;
	m_view.m[3] = 0.0f;
	
	m_view.m[4] = negSide.y;
	m_view.m[5] = m_vf.m_up.y;
	m_view.m[6] = negFwd.y;
	m_view.m[7] = 0.0f;
	
	m_view.m[8] = negSide.z;
	m_view.m[9] = m_vf.m_up.z;
	m_view.m[10] = negFwd.z;
	m_view.m[11] = 0.0f;

	vec3 negTrans = -m_vf.m_pos;

	m_view.m[12] = Dot(negTrans, negSide);
	m_view.m[13] = Dot(negTrans, m_vf.m_up);
	m_view.m[14] = Dot(negTrans, negFwd);
	m_view.m[15] = 1.f;
}

void Camera::LookAt(const vec3& focus, const vec3& eye, const vec3& up)
{
	m_vf.m_fwd = focus - eye;
	m_vf.m_side = Cross(up, m_vf.m_fwd);
	m_vf.m_up = Cross(m_vf.m_fwd, m_vf.m_side);
	m_vf.m_fwd.Normalize();
	m_vf.m_side.Normalize();
	m_vf.m_up.Normalize();
	m_vf.m_pos = eye;
}

void Camera::TiltBy(float rad)
{
	m_vf.m_fwd = RotateAround(m_vf.m_fwd, m_vf.m_side, rad);
	m_vf.m_up = Cross(m_vf.m_fwd, m_vf.m_side);
	m_vf.m_side = Cross(m_vf.m_up, m_vf.m_fwd);
	m_vf.m_up.Normalize();
	m_vf.m_side.Normalize();
}

void Camera::TurnBy(float rad)
{
	m_vf.m_fwd = RotateAround(m_vf.m_fwd, m_vf.m_up, rad);
	m_vf.m_side = Cross(m_vf.m_up, m_vf.m_fwd);
	m_vf.m_up = Cross(m_vf.m_fwd, m_vf.m_side);
	m_vf.m_side.Normalize();
	m_vf.m_up.Normalize();
}

void Camera::RollBy(float rad)
{
	m_vf.m_up = RotateAround(m_vf.m_up, m_vf.m_fwd, rad);
	m_vf.m_fwd = Cross(m_vf.m_side, m_vf.m_up);
	m_vf.m_side = Cross(m_vf.m_up, m_vf.m_fwd);
	m_vf.m_fwd.Normalize();
	m_vf.m_side.Normalize();
}

void Camera::MoveBy(const vec3& v)
{
	m_vf.m_pos += v;
}

// Compute planes in view space
void Camera::ComputeViewFrustum()
{
	vec3 pt, n;

	// znear
	pt = {0,0,-m_znear};
	n = {0,0,-1};
	m_frustum[FRUSTUM_Near] = {n, pt};

	// zfar
	pt = {0,0,-m_zfar};
	n = {0,0,1};
	m_frustum[FRUSTUM_Far] = {n, pt};

	// the rest pass through the view-space origin
	pt = {0,0,0};

	float halffov = 0.5f * m_fov * M_PI / 180.f;
	float voff = tan(halffov);
	float hoff = m_aspect * voff;

	// right
	n = Normalize({-1.f, 0, -hoff});
	m_frustum[FRUSTUM_Right] = {n, pt};
	
	// left
	n = Normalize({1.f, 0, -hoff});
	m_frustum[FRUSTUM_Left] = {n, pt};

	// top
	n = Normalize({0, -1, -voff});
	m_frustum[FRUSTUM_Top] = {n, pt};

	// bottom
	n = Normalize({0, 1, -voff});
	m_frustum[FRUSTUM_Bottom] = {n, pt};
}

