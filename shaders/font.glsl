
#ifdef VERTEX_P
uniform mat4 mvp;
in vec2 pos;
in vec2 uv;
out vec2 vUV;
void main()
{
	gl_Position = mvp * vec4(pos, 0, 1);
	vUV = uv;
}
#endif

#ifdef FRAGMENT_P
in vec2 vUV;
uniform vec3 color;
uniform sampler2D fontTex;
out vec4 outColor;
void main()
{
	outColor = vec4(color, texture(fontTex, vUV).a);
}
#endif

