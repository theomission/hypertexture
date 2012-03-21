#pragma once

#include "commonmath.hh"

class Camera;

extern int g_dbgdrawEnabled;
extern int g_dbgdrawEnableDepthTest;

void dbgdraw_Init();
void dbgdraw_Enable();
void dbgdraw_Disable();
void dbgdraw_Render(const Camera& camera);
void dbgdraw_Clear();

void dbgdraw_AABB(const AABB& aabb, const Color& color = Color(1,1,1));
void dbgdraw_OBB(const OBB& obb, const Color& color = Color(1,1,1));
void dbgdraw_OBBXfm(const OBB& obb, const mat4& xfm, const Color& color = Color(1,1,1));
void dbgdraw_Plane(const Plane& plane, const AABB& limits, const Color& color = Color(1,1,1));
void dbgdraw_PlaneXfm(const Plane& plane, const AABB& limits, const mat4 &xfm, const Color& color = Color(1,1,1));
void dbgdraw_Point(const vec3& v, const Color& color = Color(1,1,1));
void dbgdraw_PointXfm(const vec3& v, const mat4 &xfm, const Color& color = Color(1,1,1));
void dbgdraw_Sphere(const vec3& center, float radius, const Color& color = Color(1,1,1));
void dbgdraw_SphereXfm(const vec3& center, float radius, const mat4& xfm, const Color& color = Color(1,1,1));
void dbgdraw_Vector(const vec3& from, const vec3& vector, const Color& color = Color(1,1,1));
void dbgdraw_VectorXfm(const vec3& from, const vec3& vector, const mat4 &xfm, const Color& color = Color(1,1,1));
void dbgdraw_Line(const vec3& from, const vec3& to, const Color& color = Color(1,1,1));
void dbgdraw_LineXfm(const vec3& from, const vec3& to, const mat4 &xfm, const Color& color = Color(1,1,1));


