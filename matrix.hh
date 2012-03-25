#pragma once

#include <cstring>
#include "vec.hh"

class mat4
{
public:
	float m[16];

	struct identity_t{};

	mat4() {}
	mat4(const identity_t&) { 
		bzero(m, sizeof(m));
		m[15] = m[10] = m[5] = m[0] = 1.f;
	}
	mat4(float m0, float m1, float m2, float m3,
		float m4, float m5, float m6, float m7,
		float m8, float m9, float m10, float m11,
		float m12, float m13, float m14, float m15)
	{
		m[0] = m0;
		m[1] = m1;
		m[2] = m2;
		m[3] = m3;
		
		m[4] = m4;
		m[5] = m5;
		m[6] = m6;
		m[7] = m7;
		
		m[8] = m8;
		m[9] = m9;
		m[10] = m10;
		m[11] = m11;
		
		m[12] = m12;
		m[13] = m13;
		m[14] = m14;
		m[15] = m15;
	}
	
	inline mat4(const mat4& r) { Copy(r); }
	mat4& operator=(const mat4& r) ;
	mat4 operator*(const mat4& r) const;
	mat4& operator*=(const mat4& r) ;

	float operator[](int index) const { return m[index]; }
	float& operator[](int index) { return m[index]; }

	void Copy(const mat4& r);
};
	
inline void mat4::Copy(const mat4& r)
{
	memcpy(m, r.m, sizeof(m));
}

inline mat4& mat4::operator=(const mat4& r) 
{
	if(this != &r)
		Copy(r);
	return *this;
}
	
inline mat4 mat4::operator*(const mat4& r) const
{
	mat4 result;
	for(int off = 0; off < 16; off+=4)
	{
		result.m[off] = 0.f; result.m[off+1] = 0.f; result.m[off+2] = 0.f; result.m[off+3] = 0.f;
		for(int i = 0, j = 0; j < 4; ++j)
		{
			float bval = r.m[off+j];
			result.m[off]   += bval * m[i]; ++i;
			result.m[off+1] += bval * m[i]; ++i;
			result.m[off+2] += bval * m[i]; ++i;
			result.m[off+3] += bval * m[i]; ++i;
		}
	}
	return result;
}

inline mat4& mat4::operator*=(const mat4& r) 
{
	mat4 cur = *this;
	for(int off = 0; off < 16; off+=4)
	{
		m[off] = 0.f; m[off+1] = 0.f; m[off+2] = 0.f; m[off+3] = 0.f;
		for(int i = 0, j = 0; j < 4; ++j)
		{
			float bval = r.m[off+j];
			m[off]   += bval * cur.m[i]; ++i;
			m[off+1] += bval * cur.m[i]; ++i;
			m[off+2] += bval * cur.m[i]; ++i;
			m[off+3] += bval * cur.m[i]; ++i;
		}
	}
	return *this;
}

inline std::ostream& operator<<(std::ostream& s, const mat4& mat)
{
	for(int i = 0; i < 4; ++i)
	{
		std::cout << "[ " << mat.m[i] << ", " << mat.m[i+4] << 
			", " << mat.m[i+8] << ", " << mat.m[i+12] << " ]" << std::endl;
	}
	return s;
}

////////////////////////////////////////////////////////////////////////////////
// utilities

vec3 TransformVec(const mat4& m, const vec3& v);
vec3 TransformPoint(const mat4& m, const vec3& v);
void TransformFloat4(float* out, const mat4& m, const float *in);

// ortho projection specifically for the front end. Basically y is inversedin a weird way.
mat4 Compute2DProj(float w, float h, float znear, float zfar);
// normal ortho projection
mat4 ComputeOrthoProj(float w, float h, float znear, float zfar);
mat4 Compute3DProj(float degfov, float aspect, float znear, float zfar);
mat4 ComputeFrustumProj(float left, float right, float bottom, float top, float znear, float zfar);
mat4 ComputeDirShadowView(const vec3& focus, const vec3 &dir, float distance);

mat4 RotateAround(const vec3& axis, float rads);
mat4 AffineInverse(const mat4& m);
mat4 Transpose(const mat4& m);
mat4 MakeTranslation(float tx, float ty, float tz);
mat4 MakeTranslation(const vec3& t);
mat4 MakeScale(const vec3& v) ;
mat4 MakeScale(float sx, float sy, float sz);
mat4 MatFromFrame(const vec3& xaxis, const vec3& yaxis, const vec3& zaxis, const vec3& trans);
mat4 MakeCoordinateScale(float scale, float add);

inline mat4 TransposeOfInverse(const mat4& m) {
	return Transpose(AffineInverse(m));
}

