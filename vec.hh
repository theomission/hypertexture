#pragma once

#include <cmath>
#include <iostream>
#include "mathhelpers.hh"

class vec3
{
public:
	float x, y, z;

	vec3() {}
	explicit vec3(float v) : x(v), y(v), z(v) {}
	vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
	void Set(float x_, float y_, float z_) ;
	vec3 operator+(const vec3& r) const;
	vec3& operator+=(const vec3& r) ;
	vec3 operator-(const vec3& r) const;
	vec3& operator-=(const vec3& r) ;
	vec3 operator-() const;
	vec3 operator*(float s) const;
	vec3& operator*=(float s) ;
	vec3 operator/(float s) const;
	vec3& operator/=(float s) ;
	bool operator==(const vec3& r) const;
	bool operator!=(const vec3& r) const;

	void Normalize();
};
	
inline void vec3::Set(float x_, float y_, float z_) {
	x = x_; y = y_; z = z_;
}
	
inline vec3 vec3::operator+(const vec3& r) const {
	return vec3(x + r.x, y + r.y, z + r.z);
}

inline vec3& vec3::operator+=(const vec3& r)  {
	x += r.x;
	y += r.y;
	z += r.z;
	return *this;
}

inline vec3 vec3::operator-(const vec3& r) const {
	return vec3(x - r.x, y - r.y, z - r.z);
}

inline vec3& vec3::operator-=(const vec3& r)  {
	x -= r.x;
	y -= r.y;
	z -= r.z;
	return *this;
}
	
inline vec3 vec3::operator-() const {
	return vec3(-x, -y, -z);
}

inline vec3 vec3::operator*(float s) const
{
	return vec3(x*s,y*s,z*s);
}	

inline vec3& vec3::operator*=(float s) 
{
	x *= s;
	y *= s;
	z *= s;
	return *this;
}
	
inline vec3 operator*(float s, const vec3& v)  {
	return vec3(v.x*s, v.y*s, v.z*s);
}

inline vec3 vec3::operator/(float s) const
{
	return vec3(x/s,y/s,z/s);
}	

inline vec3& vec3::operator/=(float s) 
{
	x /= s;
	y /= s;
	z /= s;
	return *this;
}
	
inline bool vec3::operator==(const vec3& r) const {
	return x == r.x && y == r.y && z == r.z;
}

inline bool vec3::operator!=(const vec3& r) const {
	return x != r.x || y != r.y || z != r.z;
}

inline std::ostream& operator<<(std::ostream& s, const vec3& v) {
	s << '<' << v.x << ',' << v.y << ',' << v.z << '>';
	return s;
}

////////////////////////////////////////////////////////////////////////////////
inline float Dot(const vec3& l, const vec3& r) {
	return l.x * r.x + l.y * r.y + l.z * r.z;
}

inline vec3 Cross(const vec3& l, const vec3& r) {
	return vec3(
			l.y*r.z - l.z*r.y,
			l.z*r.x - l.x*r.z,
			l.x*r.y - l.y*r.x);
}

inline float LengthSq(const vec3& l) {
	return (l.x * l.x + l.y * l.y + l.z * l.z);
}

inline float Length(const vec3& l) {
	return sqrtf(l.x * l.x + l.y * l.y + l.z * l.z);
};

inline float Dist(const vec3& l, const vec3& r) {
	return Length(l-r);
}

inline float DistSq(const vec3& l, const vec3& r) {
	return LengthSq(l-r);
}

inline vec3 Normalize(const vec3 &v) {
	return v / Length(v);
}

inline void Normal(vec3& normal, float &len, const vec3 &v) {
	len = Length(v);
	normal = v / len;
}

inline vec3 Min(const vec3& l, const vec3& r)
{
	return vec3(Min(l.x,r.x), Min(l.y,r.y), Min(l.z,r.z));
}


inline vec3 Max(const vec3& l, const vec3& r)
{
	return vec3(Max(l.x,r.x), Max(l.y,r.y), Max(l.z,r.z));
}

inline vec3 VecMin(const vec3& l, const vec3& r) { return Min(l,r); }
inline vec3 VecMax(const vec3& l, const vec3& r) { return Max(l,r); }

vec3 RotateAround(const vec3& v, const vec3& axis, float angle);

////////////////////////////////////////////////////////////////////////////////

inline void vec3::Normalize()
{
	float invLen = 1.f/ Length(*this);
	x*=invLen;
	y*=invLen;
	z*=invLen;
}

