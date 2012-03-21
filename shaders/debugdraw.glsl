uniform mat4 mvp;

#ifdef VERTEX_P
in vec3 pos;
in vec3 color;
out vec3 vColor;
void main()
{
	gl_Position = mvp * vec4(pos, 1);
	vColor = color;
}
#endif

#ifdef FRAGMENT_P
in vec3 vColor;
out vec4 outColor;
void main()
{
	outColor = vec4(vColor, 1);
}
#endif
