#ifdef VERTEX_P
uniform mat4 mvp;
in vec2 pos;
void main()
{
	gl_Position = mvp * vec4(pos, 0, 1);
}
#endif

#ifdef FRAGMENT_P
uniform vec3 color;
out vec4 outColor;

void main()
{
	outColor = vec4(color, 1);
}
#endif

