#include <cfloat>
#include <cmath>
#include "commonmath.hh"

void AABB::Extend(const vec3& v)
{
	m_min = VecMin(m_min, v);
	m_max = VecMax(m_max, v);
}

vec3 OBBClosestPoint(const OBB& obb, const vec3& point)
{
	vec3 toPt = point - obb.m_center;
	vec3 result = obb.m_center;
	for(int i = 0; i < 3; ++i)
	{
		vec3 dir;
		float len;
		Normal(dir, len, obb.m_b[i]);

		float dist = Dot(dir, toPt);
		if(dist > len) dist = len;
		else if(dist < -len) dist = -len;
		result += dir * dist;
	}
	return result;
}

OBB OBBTransform(const mat4& mat, const OBB& obb)
{
	OBB result;
	mat4 matIT = TransposeOfInverse(mat);
	result.m_center = TransformPoint(mat, obb.m_center);
	for(int i = 0; i < 3; ++i)
		result.m_b[i] = TransformVec(matIT, obb.m_b[i]);
	return result;
}

float PlaneDist(const Plane& p, const vec3& v)
{
	float dot = Dot(p.m_n, v);
	return dot + p.m_d;
}

Plane PlaneTransform(const mat4& mat, const Plane& plane)
{
	float p[4] = {
		plane.m_n.x,
		plane.m_n.y,
		plane.m_n.z,
		plane.m_d 
	};
	mat4 xfmIT = TransposeOfInverse(mat);
	TransformFloat4(p, xfmIT, p);
	return {{p[0], p[1], p[2]}, p[3]};
}

float AngleWrap(float angle)
{	
	static const float kTwoPi = 2.f * M_PI;
	while(angle < 0.f)
		angle += kTwoPi;
	while(angle > kTwoPi)
		angle -= kTwoPi;
	return angle;
}

