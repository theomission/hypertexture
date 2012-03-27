#pragma once

#include <cfloat>
#include "vec.hh"
#include "matrix.hh"
#include "mathhelpers.hh"

class AABB
{
public:
	AABB() : m_min(FLT_MAX), m_max(-FLT_MAX) {}
	vec3 m_min;
	vec3 m_max;

	void Extend(const vec3& v);
} ;

class OBB
{
public:
	vec3 m_center;
	vec3 m_b[3];	// orthogonal basis, length = extent along each
} ;

vec3 OBBClosestPoint(const OBB& obb, const vec3& point);
OBB OBBTransform(const mat4& mat, const OBB& obb);

class Plane
{
public:
	Plane() {}
	Plane(const vec3& n, const vec3& pt) : m_n(n), m_d(-Dot(pt, n)) {}
	Plane(const vec3& n, float d) : m_n(n), m_d(d) {}
	vec3 m_n;
	float m_d;
} ;

float PlaneDist(const Plane& p, const vec3& v);
Plane PlaneTransform(const mat4& mat, const Plane& plane);

class Color
{
public:
	Color() {}
	Color(float f) : r(f), g(f), b(f), a(f) {}
	Color(float r_, float g_, float b_) : r(r_), g(g_), b(b_), a(1.f) {}
	Color(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_) {}
	bool operator==(const Color& r) const;
	bool operator!=(const Color& r) const;
	float r, g, b, a;
} ;

inline Color Min(const Color& l, const Color& r) {
	return Color(Min(l.r, r.r),
		Min(l.g, r.g),
		Min(l.b, r.b),
		Min(l.a, r.a));
}

inline Color Max(const Color& l, const Color& r) {
	return Color(Max(l.r, r.r),
		Max(l.g, r.g),
		Max(l.b, r.b),
		Max(l.a, r.a));
}
	
inline bool Color::operator==(const Color& o) const {
	return r == o.r && g == o.g && b == o.b && a == o.a;
}

inline bool Color::operator!=(const Color& o) const {
	return r != o.r || g != o.g || b != o.b || a != o.a;
}

////////////////////////////////////////////////////////////////////////////////
template<class T>
class Limits
{
public:
	Limits() : m_min(0), m_max(0) {}
	Limits(const T& min_, const T& max_) : m_min(min_), m_max(max_) {}
	T operator()(const T& val) const {
		if(m_min == m_max) return val;
		return Min(Max(val, m_min), m_max);
	}

	T Interpolate(float t) const {
		t = Clamp(t, 0.f, 1.f);
		float range = m_max - m_min;
		return T(t*range + m_min);
	}

	bool Valid() const { return m_max > m_min; }

	T m_min;
	T m_max;
};

// c-2 continuous cubic spline for t 0..1
inline float spline_c2(float t)
{
	// C2 continuous spline
	float t2 = t*t;
	float t3 = t2*t;
	float t4 = t2 * t2;
	float t5 = t2 * t3;
	return 6.f * t5 - 15.f * t4 + 10 * t3;
}

inline float SmoothStep(float stepStart, float stepEnd, float val)
{
	float t = (val - stepStart) / (stepEnd - stepStart);
	t = Clamp(t,0.f,1.f);
	return spline_c2(t);
}

float AngleWrap(float angle);

inline float Floor(float val) {
	if(val >= 0.f)
		return static_cast<float>(static_cast<int>(val));
	else
	{
		int fval = static_cast<float>(static_cast<int>(val));
		return fval == val ? val : fval - 1;
	}
}

inline bool IsPower2(unsigned int value)
{
	return (value & (value-1)) == 0;
}

