// +-------------------------------------------------+
// |   Giant Worm:                                   |
// +-------------------------------------------------+

#define WORM_ANI sin(ubo.mAbsoluteTime)*0.5+0.5

vec3 get_giant_worm_body(float u, float v, uvec3 userData, out vec3 pos, out vec3 outward, out vec3 forward)
{
	const float thickness = 0.75;
	const float numSawtoothElements = 15.0;

	const int numControlPoints = 5;
	vec3 controlPoints[NUM_BEZIER_CONTROL_POINTS];
	controlPoints[0] = 1.5 * vec3( 0.0, -1.0 * 0.65,   0.0);
	controlPoints[1] = 1.5 * mix(vec3( 0.0, 1.0 * 0.65,   0.0), vec3( 0.0,  1.5 * 0.65, 1.0), WORM_ANI);
	controlPoints[2] = 1.5 * mix(vec3( 2.0, 4.0 * 0.65,   0.0), vec3( 1.0,  5.0 * 0.65, 1.0), WORM_ANI);
	controlPoints[3] = 1.5 * mix(vec3( 0.0, 8.0 * 0.65,   0.0), vec3( 2.0, 10.0 * 0.65, 0.0), WORM_ANI);
	controlPoints[4] = 1.5 * mix(vec3(-2.0, 7.0 * 0.65,   0.0), vec3(-1.0,  8.0 * 0.65, 0.0), WORM_ANI);

	pos      = bezier_value_at(numControlPoints, controlPoints,u);
	forward  = normalize(bezier_slope_at(numControlPoints, controlPoints, u));

	// Manually make a pipe (without using the pipeify function, but same steps):
	vec3 o  = orthogonal(forward);

	// Small bumps:
	//float bumpySkin = abs(cos(u * 100.0)) * 0.05;
	float bumpySkin = sin(100.0*u)*sin(100.0*u)*sin(100.0*u)*sin(100.0*u) * 0.05 * sin(20.0*v);

	// Let the gitant worm be a bit thicker at the bottom:
	float thicknessscale = 0.75 / (u * 0.5 + 0.5) + 0.5;

	vec3 bodyPos = pos + rotate_around_axis(o * thickness * thicknessscale + bumpySkin, forward, v);
	outward = bodyPos - pos;
	return bodyPos;
}

vec3 get_giant_worm_jaws(float u, float v, float offset, float flipStrength, float dragToInnerRadius, uvec3 userData)
{
	v = v / 3.0;
	vec3 pos, outwd, fwd;
	vec3 jaws = get_giant_worm_body(1.0, offset + v, userData,  /* out: */ pos, /* out: */ outwd, /* out: */ fwd);

	// Fake rotation from fwd to outwd:
	fwd = mix(fwd, outwd, sin(ubo.mAbsoluteTime) * 0.15 + 0.3); // <- fake rotation 
	fwd = normalize(fwd);

	outwd *= flipStrength;

	jaws += fwd * u * 3.0 * sin(3.0*v/2.0)
		 + sin(3.0*v/2.0) * outwd * 0.2 * sin(u * PI);

	jaws = mix(jaws, pos + fwd * u * 1.5, ((-u)*(-u)*(-u)+1.0) * dragToInnerRadius);


	return jaws;
}

vec3 get_giant_worm_tongue(float u, float v, uvec3 userData)
{
	const float radius = 0.65;
	u = u * PI * 0.8;
	if (u < PI * 0.5) {
		u += 0.25 * PI - sin(5.0 * v)*sin(5.0 * v)*sin(5.0 * v)*sin(5.0 * v) * 0.2 * PI + sin(ubo.mAbsoluteTime + 1.5) * 0.1;
	}

	vec3 pos, outwd, fwd;
	vec3 jaws = get_giant_worm_body(1.0, 0.0, userData,  /* out: */ pos, /* out: */ outwd, /* out: */ fwd);

	vec3 tongue = to_sphere(u, v, radius);

	tongue.y *= sin(ubo.mAbsoluteTime + 1.0) * 0.5 + 1.0;

	// rotate along with the forward vector:
	outwd  = normalize(outwd);
	fwd    = normalize(fwd);
	tongue = mat3(cross(fwd, outwd), fwd, outwd) * tongue;
	tongue += pos;

	return tongue;
}
