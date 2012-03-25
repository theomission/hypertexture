#include "matrix.hh"
#include <cstring>
#include <cmath>
#include "vec.hh"
#include "common.hh"

vec3 TransformVec(const mat4& m, const vec3& v)
{
	float x,y,z;

	x = v.x * m.m[0];
	y = v.x * m.m[1];
	z = v.x * m.m[2];

	x += v.y * m.m[4];
	y += v.y * m.m[5];
	z += v.y * m.m[6];
	
	x += v.z * m.m[8];
	y += v.z * m.m[9];
	z += v.z * m.m[10];

	return {x,y,z};
}

vec3 TransformPoint(const mat4& m, const vec3& v)
{
	float x,y,z,w;

	x = v.x * m.m[0];
	y = v.x * m.m[1];
	z = v.x * m.m[2];
	w = v.x * m.m[3];

	x += v.y * m.m[4];
	y += v.y * m.m[5];
	z += v.y * m.m[6];
	w += v.y * m.m[7];
	
	x += v.z * m.m[8];
	y += v.z * m.m[9];
	z += v.z * m.m[10];
	w += v.z * m.m[11];
	
	x += m.m[12];
	y += m.m[13];
	z += m.m[14];
	w += m.m[15];

	float inv_w = 1.f/w;
	x*=inv_w;
	y*=inv_w;
	z*=inv_w;
	return {x,y,z};
}

void TransformFloat4(float* out, const mat4& m, const float *in)
{
	float x,y,z,w;
	x = in[0] * m.m[0];
	y = in[0] * m.m[1];
	z = in[0] * m.m[2];
	w = in[0] * m.m[3];
	
	x += in[1] * m.m[4];
	y += in[1] * m.m[5];
	z += in[1] * m.m[6];
	w += in[1] * m.m[7];
	
	x += in[2] * m.m[8];
	y += in[2] * m.m[9];
	z += in[2] * m.m[10];
	w += in[2] * m.m[11];
	
	x += in[3] * m.m[12];
	y += in[3] * m.m[13];
	z += in[3] * m.m[14];
	w += in[3] * m.m[15];

	out[0] = x;
	out[1] = y;
	out[2] = z;
	out[3] = w;
}

mat4 Compute2DProj(float w, float h, float znear, float zfar)
{
	mat4 result;
	float zlen = zfar - znear;
	bzero(result.m, sizeof(float)*16);
	result.m[0] = 2.f / w;
	result.m[5] = -2.f / h;
	result.m[10] = -2.f / (zlen);
	result.m[12] = -1.f;
	result.m[13] = 1.f;
	result.m[14] = 1 + 2.f * znear / zlen;
	result.m[15] = 1.f;
	return result;
}

mat4 ComputeOrthoProj(float w, float h, float znear, float zfar)
{
	mat4 result;
	float zlen = zfar - znear;
	float inv_zlen = 1.0 / zlen;
	bzero(result.m, sizeof(float)*16);
	result.m[0] = 2.f / w;
	result.m[5] = 2.f / h;
	result.m[10] = -2.f * (inv_zlen);
	result.m[14] = - (zfar + znear) * inv_zlen;
	result.m[15] = 1.f;
	return result;
}

mat4 Compute3DProj(float degfov, float aspect, float znear, float zfar)
{
	const float fov2 = 0.5 * degfov * (M_PI / 180.f);
	const float top = tan(fov2) * znear;
	const float bottom = -top;
	const float right = aspect * top;
	const float left = aspect * bottom;

	return ComputeFrustumProj(left, right, bottom, top, znear, zfar);
}

mat4 ComputeFrustumProj(float left, float right, float bottom, float top, float znear, float zfar)
{
	mat4 result;
	const float inv_r_minus_l = 1.f / (right - left);
	const float inv_t_minus_b = 1.f / (top - bottom);
	const float inv_f_minus_n = 1.f / (zfar - znear);

	result.m[0] = 2.f * znear * inv_r_minus_l;
	result.m[1] = 0.f;
	result.m[2] = 0.f;
	result.m[3] = 0.f;

	result.m[4] = 0.f;
	result.m[5] = 2.f * znear * inv_t_minus_b;
	result.m[6] = 0.f;
	result.m[7] = 0.f;

	result.m[8] = (right + left) * inv_r_minus_l;
	result.m[9] = (top + bottom) * inv_t_minus_b;
	result.m[10] = -(zfar + znear) * inv_f_minus_n;
	result.m[11] = -1.f;
	
	result.m[12] = 0.f;
	result.m[13] = 0.f;
	result.m[14] = -2.f * znear * zfar * inv_f_minus_n;
	result.m[15] = 0.f;
	return result;
}

mat4 RotateAround(const vec3& axis, float rads)
{
	mat4 result;
	float c = cos(rads);
	float one_minus_c = 1.f-c;
	float s = sin(rads);
	float ax = axis.x;
	float ay = axis.y;
	float az = axis.z;

	float c1xy = one_minus_c * ax * ay;
	float c1xz = one_minus_c * ax * az;
	float c1yz = one_minus_c * ay * az;

	float sax = s * ax;
	float say = s * ay;
	float saz = s * az;

	result.m[0] = c + one_minus_c * ax * ax;
	result.m[1] = c1xy + saz;
	result.m[2] = c1xz - say;
	result.m[3] = 0.f;
	
	result.m[4] = c1xy - saz;
	result.m[5] = c + one_minus_c * ay * ay;
	result.m[6] = c1yz + sax;
	result.m[7] = 0.f;

	result.m[8] = c1xz + say;
	result.m[9] = c1yz - sax;
	result.m[10] = c + one_minus_c * az * az;
	result.m[11] = 0.f;
	
	result.m[12] = 0.f;
	result.m[13] = 0.f;
	result.m[14] = 0.f;
	result.m[15] = 1.f;
	return result;
}

mat4 AffineInverse(const mat4& m)
{
	mat4 dest;
	float isx = 1.f/sqrtf(m.m[0]*m.m[0] + m.m[1]*m.m[1] + m.m[2]*m.m[2]);
	float isy = 1.f/sqrtf(m.m[4]*m.m[4] + m.m[5]*m.m[5] + m.m[6]*m.m[6]);
	float isz = 1.f/sqrtf(m.m[8]*m.m[8] + m.m[9]*m.m[9] + m.m[10]*m.m[10]);
	float itx = -m.m[12];
	float ity = -m.m[13];
	float itz = -m.m[14];

	float isx2 = isx*isx;
	float isy2 = isy*isy;
	float isz2 = isz*isz;

	dest.m[0] = isx2 * m.m[0];
	dest.m[4] = isx2 * m.m[1];
	dest.m[8] = isx2 * m.m[2];

	dest.m[1] = isy2 * m.m[4];
	dest.m[5] = isy2 * m.m[5];
	dest.m[9] = isy2 * m.m[6];

	dest.m[2] = isz2 * m.m[8];
	dest.m[6] = isz2 * m.m[9];
	dest.m[10] = isz2 * m.m[10];

	dest.m[3] = 0.f;
	dest.m[7] = 0.f;
	dest.m[11] = 0.f;

	float tx = dest.m[0] * itx;
	float ty = dest.m[1] * itx;
	float tz = dest.m[2] * itx;
	tx += dest.m[4] * ity;
	ty += dest.m[5] * ity;
	tz += dest.m[6] * ity;
	tx += dest.m[8] * itz; 
	ty += dest.m[9] * itz;
	tz += dest.m[10] * itz;

	dest.m[12] = tx;
	dest.m[13] = ty;
	dest.m[14] = tz;
	dest.m[15] = 1.f;
	return dest;
}

mat4 Transpose(const mat4& m)
{
	mat4 dest;
	for(int c = 0; c < 4; ++c)
	{
		int c4 = c*4;
		for(int r = 0; r < 4; ++r)
			dest.m[r*4 + c] = m.m[c4 + r];
	}
	return dest;
}

mat4 MakeTranslation(float tx, float ty, float tz)
{
	return { 
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		tx, ty, tz, 1};
}

mat4 MakeTranslation(const vec3& t)
{
	return { 
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		t.x, t.y, t.z, 1};
}

mat4 MakeScale(const vec3& v) { return MakeScale(v.x, v.y, v.z); }
mat4 MakeScale(float sx, float sy, float sz)
{
	return {
		sx, 0, 0, 0,
		0, sy, 0, 0,
		0, 0, sz, 0,
		0, 0, 0, 1.0
	};
}

mat4 MatFromFrame(const vec3& xaxis, const vec3& yaxis, const vec3& zaxis, const vec3& trans)
{
	mat4 result;
	result.m[0] = xaxis.x;
	result.m[1] = xaxis.y;
	result.m[2] = xaxis.z;
	result.m[3] = 0.f;

	result.m[4] = yaxis.x;
	result.m[5] = yaxis.y;
	result.m[6] = yaxis.z;
	result.m[7] = 0.f;

	result.m[8] = zaxis.x;
	result.m[9] = zaxis.y;
	result.m[10] = zaxis.z;
	result.m[11] = 0.f;

	result.m[12] = trans.x;
	result.m[13] = trans.y;
	result.m[14] = trans.z;
	result.m[15] = 1.f;
	return result;
}

mat4 ComputeDirShadowView(const vec3& focus, const vec3 &dir, float distance)
{
	vec3 a, b, c = Normalize(dir);
	vec3 pos = focus + distance * dir;
	// compute an orthogonal basis with a to the right, b up, dir back.
	// TODO: fix this for extreme angles
	static const vec3 kZAxis = {0,0,1};

	a = Normalize(Cross(kZAxis, dir));
	b = Normalize(Cross(dir, a)); // should already be normalized.

	mat4 result;
	result.m[0] = a.x;
	result.m[1] = b.x;
	result.m[2] = c.x;
	result.m[3] = 0.f;

	result.m[4] = a.y;
	result.m[5] = b.y;
	result.m[6] = c.y;
	result.m[7] = 0.f;

	result.m[8] = a.z;
	result.m[9] = b.z;
	result.m[10] = c.z;
	result.m[11] = 0.f;

	result.m[12] = -Dot(a,pos);
	result.m[13] = -Dot(b,pos);
	result.m[14] = -Dot(c,pos);
	result.m[15] = 1.f;
	return result;
}	

mat4 MakeCoordinateScale(float scale, float add)
{
	return mat4(
		0.5, 0, 0, 0,
		0, 0.5, 0, 0,
		0, 0, 0.5, 0,
		0.5, 0.5, 0.5, 1);
}

