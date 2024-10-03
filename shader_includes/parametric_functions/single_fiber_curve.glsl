// +--------------------+
// | Fiber Curve        |
// +--------------------+
vec3 get_fiber_curve(float u, float v, uvec3 userData)
{
	const float fiberId = float(userData[1]);
	const float threadRadius = uintBitsToFloat(userData[2]);

	const float rowOffset = 5.835;
	const float loopRoundness = 1.354;
	const float loopHeight = 4.2;
	const float loopDepth = 0.619;

	const float yarnRadius = 0.5;
	const float fiberTwist = 5.0;

	vec3 eta_t = fiber_curve(u, loopRoundness, loopHeight, loopDepth, yarnRadius, fiberTwist, fiberId);
	vec3 nextEta_t = fiber_curve(u + 0.001, loopRoundness, loopHeight, loopDepth, yarnRadius, fiberTwist, fiberId);

	// Manually pipeify:
	vec3 fwd    = normalize(nextEta_t - eta_t);
	vec3 side   = orthogonal(fwd);
	vec3 newPos = eta_t + rotate_around_axis(side * threadRadius, fwd, v);
	return newPos;
}
