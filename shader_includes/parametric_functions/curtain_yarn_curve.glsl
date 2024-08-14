// +--------------------+
// | Yarn Curve Curtain |
// +--------------------+
vec3 get_curtain_yarn(float u, float v, uvec3 userData)
{
	const float rowId = float(userData[0]);
	const float threadRadius = uintBitsToFloat(userData[2]);

	const float rowOffset = 5.835;
	const float loopRoundness = 1.354;
	const float loopHeight = 3.710;
	const float loopDepth = 0.619;

	vec3 gamma_t = yarn_curve(u, loopRoundness, loopHeight, loopDepth);
	gamma_t.y += rowId * rowOffset;

	vec3 nextGamma_t = yarn_curve(u + 0.001 /* <---- TODO */, loopRoundness, loopHeight, loopDepth);
	nextGamma_t.y += rowId * rowOffset;

	// Manually pipeify:
	vec3 fwd    = normalize(nextGamma_t - gamma_t);
	vec3 side   = orthogonal(fwd);
	vec3 newPos = gamma_t + rotate_around_axis(side * threadRadius, fwd, v);
	
	// +---------------------------+
	// | Animated Fiber/Yarn Curve |
	// +---------------------------+
	if (ubo.mDebugSlidersi[2] == 1) {
		float time = ubo.mAbsoluteTime;
		float y = smoothstep(1500.0, 0.0, newPos.y);
		float x = newPos.x / 35.0;
		float a1 = 0.2;
		float a2 = 0.5;
		newPos.z += y * (100.0 * a1 * sin(x*0.5 - time) - a2 * sin(x*1.11 - time));
	}
	
	return newPos;
}
