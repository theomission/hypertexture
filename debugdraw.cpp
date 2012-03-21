#include <cstring>
#include <cmath>
#include <utility>
#include <memory>
#include "debugdraw.hh"
#include "framemem.hh"
#include "render.hh"
#include "matrix.hh"
#include "camera.hh"
#include "common.hh"

////////////////////////////////////////////////////////////////////////////////
// types
struct ddAABB
{
	AABB m_aabb;
	Color m_color;
	const struct ddAABB* m_next;
};

struct ddOBB
{
	mat4 m_xfm;
	OBB m_obb;
	Color m_color;
	const struct ddOBB* m_next;
};

struct ddPoint
{
	mat4 m_xfm;
	vec3 m_point;
	Color m_color;
	const struct ddPoint* m_next;
} ;

struct ddPlane
{
	mat4 m_xfm;
	Plane m_plane;
	AABB m_bounds;
	Color m_color;
	const struct ddPlane* m_next;
} ;

struct ddSphere
{
	mat4 m_xfm;
	vec3 m_center;
	float m_radius;
	Color m_color;
	const struct ddSphere* m_next;
} ;

struct ddVec
{
	mat4 m_xfm;
	vec3 m_from;
	vec3 m_vec;
	Color m_color;
	const struct ddVec* m_next;
};

struct ddLists
{
	const ddAABB* m_aabbs;
	const ddOBB* m_obbs;
	const ddPoint* m_points;
	const ddPlane* m_planes;
	const ddSphere* m_spheres;
	const ddVec* m_vecs;
} ;

////////////////////////////////////////////////////////////////////////////////
// globals
int g_dbgdrawEnabled = 1;
int g_dbgdrawEnableDepthTest = 1;

////////////////////////////////////////////////////////////////////////////////
// file globals
static ddLists g_lists;
static std::shared_ptr<ShaderInfo> g_dbgdrawShader;
static std::shared_ptr<Geom> g_dbgSphereGeom;

////////////////////////////////////////////////////////////////////////////////
void dbgdraw_Init()
{
	if(!g_dbgdrawShader)
		g_dbgdrawShader = render_CompileShader("shaders/debugdraw.glsl");

	g_dbgSphereGeom = render_GenerateSphereGeom(20, 10);
}

void dbgdraw_SetEnabled(int enabled)
{
	ASSERT(!(enabled & ~1));
	g_dbgdrawEnabled = enabled;
}

int dbgdraw_IsEnabled() { return g_dbgdrawEnabled; }

void dbgdraw_SetDepthTestEnabled(int enabled)
{
	ASSERT(!(enabled & ~1));
	g_dbgdrawEnableDepthTest = enabled;
}

int dbgdraw_IsDepthTestEnabled() { return g_dbgdrawEnableDepthTest; }

static void ddRenderAABBs(const mat4& projview);
static void ddRenderOBBs(const mat4& projview);
static void ddRenderPoints(const mat4& projview);
static void ddRenderPlanes(const mat4& projview);
static void ddRenderSpheres(const mat4& projview); 
static void ddRenderVecs(const mat4& projview);

void dbgdraw_Render(const Camera& camera)
{
	if(g_dbgdrawEnableDepthTest) 
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
	mat4 projview = camera.GetProj() * camera.GetView();

	const ShaderInfo* shader = g_dbgdrawShader.get();
	glUseProgram(shader->m_program);

	ddRenderAABBs(projview);
	ddRenderOBBs(projview);
	ddRenderPoints(projview);
	ddRenderPlanes(projview);
	ddRenderSpheres(projview); 
	ddRenderVecs(projview);
}

void dbgdraw_Clear()
{
	// memory is part of framemem so doesn't need to be free'd here
	bzero(&g_lists, sizeof(g_lists));
}

////////////////////////////////////////////////////////////////////////////////
void dbgdraw_AABB(const AABB& aabb, const Color& color)
{
	if(!g_dbgdrawEnabled) return;

	ddAABB* dd = (ddAABB*)framemem_Alloc(sizeof(ddAABB));
	bzero(dd, sizeof(*dd));

	dd->m_aabb = aabb;
	dd->m_color = color;
	dd->m_next = g_lists.m_aabbs;
	g_lists.m_aabbs = dd;
}

void dbgdraw_OBB(const OBB& obb, const Color& color)
{
	dbgdraw_OBBXfm(obb, {mat4::identity_t()}, color);
}

void dbgdraw_OBBXfm(const OBB& obb, const mat4& xfm, const Color& color)
{
	if(!g_dbgdrawEnabled) return;
	
	ddOBB* dd = (ddOBB*)framemem_Alloc(sizeof(ddOBB));
	bzero(dd, sizeof(*dd));

	dd->m_xfm = xfm;
	dd->m_obb = obb;
	dd->m_color = color;
	dd->m_next = g_lists.m_obbs;
	g_lists.m_obbs = dd;
}

void dbgdraw_Plane(const Plane& plane, const AABB& limits, const Color& color)
{
	dbgdraw_PlaneXfm(plane, limits, {mat4::identity_t()}, color);
}

void dbgdraw_PlaneXfm(const Plane& plane, const AABB& limits, const mat4& xfm, const Color& color)
{
	if(!g_dbgdrawEnabled) return;
	
	ddPlane* dd = (ddPlane*)framemem_Alloc(sizeof(ddPlane));
	bzero(dd, sizeof(*dd));

	dd->m_xfm = xfm;
	dd->m_plane = plane;
	dd->m_bounds = limits;
	dd->m_color = color;
	dd->m_next = g_lists.m_planes;
	g_lists.m_planes = dd;
}

void dbgdraw_Point(const vec3& v, const Color& color)
{
	dbgdraw_PointXfm(v, {mat4::identity_t()}, color);
}

void dbgdraw_PointXfm(const vec3& v, const mat4& xfm, const Color& color)
{
	if(!g_dbgdrawEnabled) return;
	
	ddPoint* dd = (ddPoint*)framemem_Alloc(sizeof(ddPoint));
	bzero(dd, sizeof(*dd));

	dd->m_xfm = xfm;
	dd->m_point = v;
	dd->m_color = color;
	dd->m_next = g_lists.m_points;
	g_lists.m_points = dd;
}

void dbgdraw_Sphere(const vec3& center, float radius, const Color& color)
{
	dbgdraw_SphereXfm(center, radius, {mat4::identity_t()}, color);
}

void dbgdraw_SphereXfm(const vec3& center, float radius, const mat4& xfm, const Color& color)
{
	if(!g_dbgdrawEnabled) return;
	
	ddSphere* dd = (ddSphere*)framemem_Alloc(sizeof(ddSphere));
	bzero(dd, sizeof(*dd));

	dd->m_xfm = xfm;
	dd->m_center = center;
	dd->m_radius = radius;
	dd->m_color = color;
	dd->m_next = g_lists.m_spheres;
	g_lists.m_spheres = dd;
}

void dbgdraw_Vector(const vec3& from, const vec3& vector, const Color& color)
{
	dbgdraw_VectorXfm(from, vector, {mat4::identity_t()}, color);
}

void dbgdraw_VectorXfm(const vec3& from, const vec3& vector, const mat4& xfm, const Color& color)
{
	if(!g_dbgdrawEnabled) return;
	
	ddVec* dd = (ddVec*)framemem_Alloc(sizeof(ddVec));
	bzero(dd, sizeof(*dd));

	dd->m_xfm = xfm;
	dd->m_from = from;
	dd->m_vec = vector;
	dd->m_color = color;
	dd->m_next = g_lists.m_vecs;
	g_lists.m_vecs = dd;
}

void dbgdraw_Line(const vec3& from, const vec3& to, const Color& color)
{
	dbgdraw_LineXfm(from, to, {mat4::identity_t()}, color);
}

void dbgdraw_LineXfm(const vec3& from, const vec3& to, const mat4& xfm, const Color& color)
{
	if(!g_dbgdrawEnabled) return;
	
	ddVec* dd = (ddVec*)framemem_Alloc(sizeof(ddVec));
	bzero(dd, sizeof(*dd));

	dd->m_xfm = xfm;
	dd->m_from = from;
	dd->m_vec = to - from;
	dd->m_color = color;
	dd->m_next = g_lists.m_vecs;
	g_lists.m_vecs = dd;
}

static void ddRenderAABBs(const mat4& projview)
{
	const ShaderInfo* shader = g_dbgdrawShader.get();
	GLint posLoc = shader->m_attrs[GEOM_Pos];
	GLint colorLoc = shader->m_attrs[GEOM_Color];
	GLint mvpLoc = shader->m_uniforms[BIND_Mvp];
	glUniformMatrix4fv(mvpLoc, 1, 0, projview.m);

	const ddAABB* cur = g_lists.m_aabbs;
	if(cur) 
	{	
		glBegin(GL_LINES);
		while(cur)
		{
			glVertexAttrib3fv(colorLoc, &cur->m_color.r);
			const AABB* aabb = &cur->m_aabb;

			// Bottom half
			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_min.y, aabb->m_min.z);
			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_min.y, aabb->m_max.z);

			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_min.y, aabb->m_max.z);
			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_min.y, aabb->m_max.z);

			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_min.y, aabb->m_max.z);
			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_min.y, aabb->m_min.z);

			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_min.y, aabb->m_min.z);
			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_min.y, aabb->m_min.z);

			// Top half
			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_max.y, aabb->m_min.z);
			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_max.y, aabb->m_max.z);

			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_max.y, aabb->m_max.z);
			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_max.y, aabb->m_max.z);

			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_max.y, aabb->m_max.z);
			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_max.y, aabb->m_min.z);

			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_max.y, aabb->m_min.z);
			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_max.y, aabb->m_min.z);

			// Connecting lines

			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_min.y, aabb->m_min.z);
			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_max.y, aabb->m_min.z);

			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_min.y, aabb->m_min.z);
			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_max.y, aabb->m_min.z);

			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_min.y, aabb->m_max.z);
			glVertexAttrib3f(posLoc, aabb->m_max.x, aabb->m_max.y, aabb->m_max.z);

			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_min.y, aabb->m_max.z);
			glVertexAttrib3f(posLoc, aabb->m_min.x, aabb->m_max.y, aabb->m_max.z); 

			cur = cur->m_next;
		}
		glEnd();
	}
}

static void ddRenderOBBs(const mat4& projview)
{
	const ShaderInfo* shader = g_dbgdrawShader.get();
	GLint posLoc = shader->m_attrs[GEOM_Pos];
	GLint colorLoc = shader->m_attrs[GEOM_Color];
	GLint mvpLoc = shader->m_uniforms[BIND_Mvp];
	glUniformMatrix4fv(mvpLoc, 1, 0, projview.m);

	static const float s_coords[][3] = {
		{ -1.f, -1.f, -1.f },
		{ 1.f, -1.f, -1.f },
		{ 1.f, 1.f, -1.f },
		{ -1.f, 1.f, -1.f },
		{ -1.f, -1.f, 1.f },
		{ 1.f, -1.f, 1.f },
		{ 1.f, 1.f, 1.f },
		{ -1.f, 1.f, 1.f },
	};

	const ddOBB* cur = g_lists.m_obbs;
	if(cur) 
	{	
		glBegin(GL_LINES);
		while(cur)
		{
			OBB obb = OBBTransform(cur->m_xfm, cur->m_obb);

			glVertexAttrib3fv(colorLoc, &cur->m_color.r);
			vec3 pt[8];
			for(int i = 0; i < 8; ++i)
			{
				pt[i] = obb.m_center + 
					obb.m_b[0] * s_coords[i][0] +
					obb.m_b[1] * s_coords[i][1] +
					obb.m_b[2] * s_coords[i][2];
			}

			// Bottom half
			glVertexAttrib3fv(posLoc, &pt[0].x);
			glVertexAttrib3fv(posLoc, &pt[1].x);

			glVertexAttrib3fv(posLoc, &pt[1].x);
			glVertexAttrib3fv(posLoc, &pt[2].x);

			glVertexAttrib3fv(posLoc, &pt[2].x);
			glVertexAttrib3fv(posLoc, &pt[3].x);

			glVertexAttrib3fv(posLoc, &pt[3].x);
			glVertexAttrib3fv(posLoc, &pt[0].x);

			// Top half
			glVertexAttrib3fv(posLoc, &pt[4].x);
			glVertexAttrib3fv(posLoc, &pt[5].x);

			glVertexAttrib3fv(posLoc, &pt[5].x);
			glVertexAttrib3fv(posLoc, &pt[6].x);

			glVertexAttrib3fv(posLoc, &pt[6].x);
			glVertexAttrib3fv(posLoc, &pt[7].x);

			glVertexAttrib3fv(posLoc, &pt[7].x);
			glVertexAttrib3fv(posLoc, &pt[4].x);

			// Connecting lines
			glVertexAttrib3fv(posLoc, &pt[0].x);
			glVertexAttrib3fv(posLoc, &pt[4].x);

			glVertexAttrib3fv(posLoc, &pt[1].x);
			glVertexAttrib3fv(posLoc, &pt[5].x);

			glVertexAttrib3fv(posLoc, &pt[2].x);
			glVertexAttrib3fv(posLoc, &pt[6].x);

			glVertexAttrib3fv(posLoc, &pt[3].x);
			glVertexAttrib3fv(posLoc, &pt[7].x);

			cur = cur->m_next;
		}
		glEnd();
	}
}

static void ddRenderPoints(const mat4& projview)
{
	const ShaderInfo* shader = g_dbgdrawShader.get();
	GLint posLoc = shader->m_attrs[GEOM_Pos];
	GLint colorLoc = shader->m_attrs[GEOM_Color];
	GLint mvpLoc = shader->m_uniforms[BIND_Mvp];
	glUniformMatrix4fv(mvpLoc, 1, 0, projview.m);

	const ddPoint* cur = g_lists.m_points;
	if(cur) 
	{	
		glPointSize(4.f);
		glBegin(GL_POINTS);
		while(cur)
		{
			vec3 pt = TransformPoint(cur->m_xfm, cur->m_point);
			glVertexAttrib3fv(colorLoc, &cur->m_color.r);
			glVertexAttrib3fv(posLoc, &pt.x);
			cur = cur->m_next;
		}
		glEnd();
	}
}

static void ddDrawPlane(const Plane& plane, const AABB& bounds, const Color& color);
static void ddRenderPlanes(const mat4& projview)
{
	const ShaderInfo* shader = g_dbgdrawShader.get();
	GLint mvpLoc = shader->m_uniforms[BIND_Mvp];
	glUniformMatrix4fv(mvpLoc, 1, 0, projview.m);

	const ddPlane* cur = g_lists.m_planes;
	while(cur)
	{
		Plane plane = PlaneTransform(cur->m_xfm, cur->m_plane);
		ddDrawPlane(plane, cur->m_bounds, cur->m_color);
		cur = cur->m_next;
	}
}

static void ddRenderSpheres(const mat4& projview)
{
	const ShaderInfo* shader = g_dbgdrawShader.get();
	GLint colorLoc = shader->m_attrs[GEOM_Color];
	GLint mvpLoc = shader->m_uniforms[BIND_Mvp];

	const ddSphere* cur = g_lists.m_spheres;
	while(cur)
	{
		// ignore scale -- debug spheres have to be spheres
		vec3 center = TransformPoint(cur->m_xfm, cur->m_center);

		mat4 model = 
			MakeTranslation(center.x, center.y, center.z) * 
			MakeScale(cur->m_radius, cur->m_radius, cur->m_radius);
		mat4 mvp = projview * model;

		glVertexAttrib3fv(colorLoc, &cur->m_color.r);
		glUniformMatrix4fv(mvpLoc, 1, 0, mvp.m);
		g_dbgSphereGeom->Render(*shader);
		
		cur = cur->m_next;
	}
}

static void ddRenderVecs(const mat4& projview)
{
	const ShaderInfo* shader = g_dbgdrawShader.get();
	GLint posLoc = shader->m_attrs[GEOM_Pos];
	GLint colorLoc = shader->m_attrs[GEOM_Color];
	GLint mvpLoc = shader->m_uniforms[BIND_Mvp];
	glUniformMatrix4fv(mvpLoc, 1, 0, projview.m);

	const ddVec* cur = g_lists.m_vecs;
	if(cur) 
	{	
		glBegin(GL_LINES);
		while(cur)
		{
			vec3 from = TransformPoint(cur->m_xfm, cur->m_from);
			vec3 to = TransformPoint(cur->m_xfm, cur->m_from + cur->m_vec);

			glVertexAttrib3fv(colorLoc, &cur->m_color.r);
			glVertexAttrib3fv(posLoc, &from.x);
			glVertexAttrib3fv(posLoc, &to.x);
			cur = cur->m_next;
		}
		glEnd();
	}
}

static void ddDrawPlane(const Plane& plane, const AABB& bounds, const Color& color)
{
	vec3 points[6];
	struct edge_t
	{
		int start;
		int end;
	};

	static const edge_t edges[12] =
	{
		// bottom
		{0, 1},
		{1, 3},
		{3, 2},
		{2, 0},

		// top
		{4, 5},
		{5, 7},
		{7, 6},
		{6, 4},

		// top to bottom sides
		{0, 4},
		{1, 5},
		{2, 6},
		{3, 7},
	};

	// clip plane to bounds and render a quad
	vec3 corners[8];
	const vec3 *minmax[2] = { &bounds.m_min, &bounds.m_max };       
	for(int j = 0; j < 8; ++j)
	{
		int iz = j & 1;
		int ix = (j >> 1) & 1;
		int iy = (j >> 2) & 1;
		corners[j].Set(minmax[ix]->x, minmax[iy]->y, minmax[iz]->z);
	}

	int numPoints = 0;

	// add corners as points if they are close to the plane
	for(int j = 0; j < 8; ++j)
	{
		float planeDist = PlaneDist(plane, corners[j]);
		if(fabs(planeDist) < EPSILON)
			points[numPoints++] = corners[j];
	}

	// add edge intersections 
	for(int j = 0; j < 12; ++j)
	{
		vec3 a = corners[edges[j].start], 
			b = corners[edges[j].end], 
			ab = b - a;

		// intersect edge with plane
		float t = (-plane.m_d - Dot(plane.m_n, a)) / Dot(plane.m_n, ab);
		if(t >= 0.f && t <= 1.f)
		{
			vec3 pt = a + t * ab, 
				ptA = a - pt, 
				ptB = b - pt;

			float distSqA = LengthSq(ptA);
			float distSqB = LengthSq(ptB);
			if(distSqA > EPSILON_SQ && distSqB > EPSILON_SQ)
			{
				points[numPoints++] = pt;
				if(numPoints == 6)
					break;
			}       
		}
	}

	if(numPoints < 3)
		return;

	// Sort results
	const float inv_num = 1.f / numPoints;
	vec3 center = {0,0,0};
	for(int j = 0; j < numPoints; ++j)
		center += inv_num * points[j];

	vec3 sideVec = Normalize(points[0] - center);
	vec3 upVec = Normalize(Cross(plane.m_n, sideVec));

	for(int j = 1; j < numPoints; ++j)
	{    
		vec3 toPointJ = points[j] - center;

		float angleJ = AngleWrap(atan2(Dot(upVec, toPointJ), Dot(sideVec, toPointJ)));
		for(int k = j+1; k < numPoints; ++k)
		{
			vec3 toPointK = points[k] - center;
			float angleK = AngleWrap(atan2(Dot(upVec, toPointK), Dot(sideVec, toPointK)));
			if(angleK < angleJ) 
			{
				angleJ = angleK;
				std::swap(points[j], points[k]);
			}
		}
	}

	// Draw outline
	const ShaderInfo* shader = g_dbgdrawShader.get();
	GLint posLoc = shader->m_attrs[GEOM_Pos];
	GLint colorLoc = shader->m_attrs[GEOM_Color];
	glVertexAttrib3fv(colorLoc, &color.r);

	glLineWidth(2.f);
	glBegin(GL_LINES);
	for(int j = 0; j < numPoints; ++j)
	{
		int next = (j + 1) % numPoints;
		glVertexAttrib3fv(posLoc, &points[j].x);
		glVertexAttrib3fv(posLoc, &points[next].x);
	}
	glEnd();
	glLineWidth(1.f);

	// Draw triangles
	glVertexAttrib3fv(colorLoc, &color.r);
	glBegin(GL_TRIANGLE_FAN);
	glVertexAttrib3fv(posLoc, &center.x);
	for(int j = 0; j < numPoints; ++j)
		glVertexAttrib3fv(posLoc, &points[j].x);
	glVertexAttrib3fv(posLoc, &points[0].x);
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glVertexAttrib3fv(posLoc, &center.x);
	for(int j = numPoints-1; j >= 0; --j)
		glVertexAttrib3fv(posLoc, &points[j].x);
	glVertexAttrib3fv(posLoc, &points[numPoints-1].x);
	glEnd();

}
