
#ifdef VERTEX_P
uniform mat4 mvp;
in vec3 pos;
in vec3 color;
in float uv;
out vec3 vColor;
out float vCoord;
void main()
{
	gl_Position = mvp * vec4(pos, 1);
	vColor = color;
	vCoord = uv;
}
#endif

#ifdef FRAGMENT_P
uniform float stipple;
in vec3 vColor;
in float vCoord;
out vec4 outColor;
void main()
{
	if(cos(vCoord * 800 * stipple) < -0.2) discard;
	outColor = vec4(vColor, 1);
}
#endif

