
uniform mat4 mvp;
in vec2 pos;
in vec2 uv;
out vec2 vUV;
void main()
{
	gl_Position = mvp * vec4(pos, 0, 1);
	vUV = uv;
}

