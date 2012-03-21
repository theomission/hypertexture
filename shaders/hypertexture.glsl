uniform mat4 mvp;
uniform sampler3D densityMap;

#ifdef VERTEX_P
in vec3 pos;
in vec3 uv;
out vec3 vCoord;
void main()
{
	gl_Position = mvp * vec4(pos, 1);
	vCoord = uv;
}
#endif

#ifdef FRAGMENT_P
in vec3 vCoord;
out vec4 outColor;
void main()
{
	outColor = vec4(texture(densityMap, vCoord).rrr, 1);
}
#endif

