// +--------------------+
// | Yarn Curve         |
// +--------------------+
vec3 get_yarn_curve(float u, float v, uvec3 userData)
{
	const float threadRadius = uintBitsToFloat(userData[2]);

	const float rowOffset = 5.835;
	const float loopRoundness = 1.354;
	const float loopHeight = 3.710;
	const float loopDepth = 0.619;

	vec3 gamma_t = yarn_curve(u, loopRoundness, loopHeight, loopDepth);
	vec3 nextGamma_t = yarn_curve(u + 0.001 /* <---- TODO */, loopRoundness, loopHeight, loopDepth);

	// Manually pipeify:
	vec3 fwd    = normalize(nextGamma_t - gamma_t);
	vec3 side   = orthogonal(fwd);
	vec3 newPos = gamma_t + rotate_around_axis(side * threadRadius, fwd, v);
	return newPos;
}
