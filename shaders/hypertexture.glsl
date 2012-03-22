uniform mat4 mvp;
uniform sampler3D densityMap;
uniform vec3 eyePosInModel;
uniform vec3 sundir;
uniform vec3 sunColor;

#ifdef VERTEX_P
in vec3 pos;
in vec3 uv;
out vec3 vRay;
out vec3 vCoord;
void main()
{
	vec3 dir = pos - eyePosInModel;
	gl_Position = mvp * vec4(pos, 1);
	vCoord = uv;
	vRay = dir;
}
#endif

#define NUM_STEPS 64

#ifdef FRAGMENT_P
in vec3 vCoord;
in vec3 vRay;
out vec4 outColor;

void main()
{
	vec3 ray = normalize(vRay);
	vec3 position = vCoord;
	vec3 step = ray/NUM_STEPS;
	float len = length(step);

	float sum = 0.0;
	while(position.x >= 0.f && position.x <= 1.f &&
		position.y >= 0.f && position.y <= 1.f &&
		position.z >= 0.f && position.z <= 1.f)
	{
		float sample = texture(densityMap, position).r;
		sum += sample * len;
		if(sum > 0.999)
			break;
		position += step;
	}
	outColor = vec4(sum,sum,sum, 1);
}
#endif

