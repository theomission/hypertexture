#include <cmath>
#include "vec.hh"
#include "common.hh"

vec3 RotateAround(const vec3& v, const vec3& axis, float angle)
{
	float c = cos(angle);
	float s = sin(angle);
	float one_minus_c = 1.f - c;
	float axis_dot_v = Dot(axis, v);
	float k = one_minus_c * axis_dot_v;
	vec3 axis_cross_v = Cross(axis, v);
	return vec3(
	 c * v.x + k * axis.x + s * axis_cross_v.x,
	 c * v.y + k * axis.y + s * axis_cross_v.y,
	 c * v.z + k * axis.z + s * axis_cross_v.z);
}

