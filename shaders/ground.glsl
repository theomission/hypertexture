uniform mat4 mvp;
uniform mat4 model;
uniform mat4 modelIT;
uniform mat4 matShadow;
uniform vec3 eyePos;
uniform vec3 sundir = vec3(1,1,1);
uniform vec3 sunColor = vec3(1,1,1);
uniform float shininess = 70;
uniform float ambient = 1;
uniform float Ka = 0.2;
uniform float Kd = 0.4;
uniform float Ks = 0.4;
uniform sampler2D shadowMap; // projection map, not a normal shadow map

#ifdef VERTEX_P
in vec3 pos;
in vec3 normal;
out vec3 vNormal;
out vec3 vToEye;
out vec2 vShadowCoord;
void main()
{
	gl_Position = mvp * vec4(pos, 1);
	vNormal = (modelIT * vec4(normal, 0)).xyz;
	vec3 xfmPos = (model * vec4(pos,1)).xyz;
	vToEye = eyePos - xfmPos;

	vShadowCoord = (matShadow * vec4(pos,1)).xy;
}
#endif

#ifdef FRAGMENT_P
in vec3 vNormal;
in vec3 vToEye;
in vec2 vShadowCoord;
out vec4 outColor;
void main()
{
	float lightVisible = 1.0;
	if(all(lessThan(vShadowCoord, vec2(1))) && all(greaterThan(vShadowCoord, vec2(0))))	
		lightVisible = 1.0 - 0.5 * texture(shadowMap, vShadowCoord).x;

	vec3 N = normalize(vNormal);
	vec3 V = normalize(vToEye);
	vec3 H = normalize (V + sundir);
	float sunDotN = dot(sundir, N);
	float diffuseAmount = max(0,sunDotN);
	float specularAmount = max(0, dot(H, N)) ;
	float isVisible = float(sunDotN > 0.0);
	lightVisible *= isVisible;
	specularAmount = pow(specularAmount, shininess);
	specularAmount *= lightVisible;

	vec3 diffuseLight = Kd * diffuseAmount * sunColor;
	vec3 specularLight = Ks * specularAmount * sunColor;
	vec3 ambientLight = Ka * ambient * sunColor;

	vec3 shadowedLight = mix(ambientLight, ambientLight + diffuseLight + specularLight, lightVisible);
	outColor = vec4(shadowedLight, 1);
}
#endif

