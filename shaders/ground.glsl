uniform mat4 mvp;
uniform mat4 model;
uniform mat4 modelIT;
uniform vec3 eyePos;
uniform vec3 sundir = vec3(1,1,1);
uniform vec3 sunColor = vec3(1,1,1);
uniform float shininess = 70;
uniform float ambient = 1;
uniform float Ka = 0.3;
uniform float Kd = 0.9;
uniform float Ks = 0.9;

#ifdef VERTEX_P
in vec3 pos;
in vec3 normal;
out vec3 vNormal;
out vec3 vToEye;
void main()
{
	gl_Position = mvp * vec4(pos, 1);
	vNormal = (modelIT * vec4(normal, 0)).xyz;
	vec3 xfmPos = (model * vec4(pos,1)).xyz;
	vToEye = eyePos - xfmPos;
}
#endif

#ifdef FRAGMENT_P
in vec3 vNormal;
in vec3 vToEye;
out vec4 outColor;
void main()
{
	vec3 N = normalize(vNormal);
	vec3 V = normalize(vToEye);
	vec3 H = normalize (V + sundir);
	float sunDotN = dot(sundir, N);
	float diffuseAmount = max(0,sunDotN);
	float specularAmount = max(0, dot(H, N)) ;
	float isVisible = float(sunDotN > 0.0);
	specularAmount *= isVisible;
	specularAmount = pow(specularAmount, shininess);

	vec3 diffuseLight = Kd * diffuseAmount * sunColor;
	vec3 specularLight = Ks * specularAmount * sunColor;
	vec3 ambientLight = Ka * ambient * sunColor;
	outColor = vec4(ambientLight + diffuseLight + specularLight, 1);
}
#endif

