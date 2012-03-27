
vec3 GetExitPoint(vec3 pos, vec3 ray)
{
	vec3 invDenom = 1.0 / ray;
	bvec3 isZero = lessThanEqual(abs(ray), vec3(0.001));

	const vec3 boxMin = vec3(0);
	const vec3 boxMax = vec3(1);
	vec3 t1 = (boxMin - pos) * invDenom;
	vec3 t2 = (boxMax - pos) * invDenom;
	vec3 far = max(t1,t2) ;
	far = mix(far,vec3(10.0),isZero);
	float t = min(min(far.x, far.y), far.z);
	return pos + t * ray ;
}

