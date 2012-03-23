#ifdef VERTEX_P
in vec3 pos;
out vec3 vCoord;
void main()
{
	vCoord = 0.5 * pos + vec3(0.5);
	gl_Position = vec4(pos.xy,0,1);
}
#endif

#ifdef FRAGMENT_P
in vec3 vCoord;
out float outDensity;
void main()
{
	outDensity = density(vCoord);
}
#endif

