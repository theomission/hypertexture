uniform mat4 mvp;
uniform sampler3D densityMap;
uniform sampler3D transMap;
uniform vec3 eyePosInModel;
uniform vec3 sundir;
uniform vec3 sunColor;
uniform float absorption;
uniform vec3 color;
uniform float densityMult = 1.0;
uniform vec3 absorptionColor = vec3(1,1,1);

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

uniform vec3 phaseConstants;
// x = (3.0/2.0) * (1.f - g2) / (2.f + g2)
// y = 1 + g2
// z = -2*g
float MiePhase(vec3 L, vec3 V)
{
	float c = dot(L,V);
	float c2 = c*c;

	float c2_plus_1 = c2 + 1.0;

	float mphase = phaseConstants.x * (c2_plus_1) / pow(phaseConstants.y + phaseConstants.z * c, 1.5f);
	return mphase;
}

vec3 GetLight(vec3 pos, vec3 V)
{
	vec3 T = texture(transMap, pos).rgb;
	return T * sunColor * MiePhase(sundir, V);
}

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

	vec3 curColor = vec3(0);
	float alpha = 0.0;
	vec3 T = vec3(1.0);
	vec3 Tfactor = -absorption * len * absorptionColor * densityMult;
	vec3 colorFactor = color * len * densityMult;

	for(int i = 0; i < NUM_STEPS; ++i)
	{
		if(any(lessThan(position, vec3(0)))) break;
		if(any(greaterThan(position, vec3(1)))) break;

		float sample = texture(densityMap, position).r;
		vec3 Tlocal = exp(Tfactor * sample);
		T *= Tlocal;

		vec3 colorIn = T * GetLight(position, -ray) * colorFactor * sample;
		curColor += colorIn;

		float alphaT = min(min(Tlocal.x, Tlocal.y), Tlocal.z);
		alpha = alpha + (1 - alphaT) * (1 - alpha);
		
		if(all(lessThan(T,vec3(0.001))))
			break;
		position += step;
	}
	outColor = vec4(curColor, alpha);
}
#endif

