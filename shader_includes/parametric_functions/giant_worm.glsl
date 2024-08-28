// +-------------------------------------------------+
// |   Giant Worm:                                   |
// +-------------------------------------------------+
vec3 get_giant_worm_body(float u, float v, uvec3 userData, out vec3 pos, out vec3 outward, out vec3 forward)
{
	const float thickness = 0.75;
	const float numSawtoothElements = 15.0;

	const int numControlPoints = 5;
	vec3 controlPoints[NUM_BEZIER_CONTROL_POINTS];
	controlPoints[0] = 1.5 * vec3( 0.0,-0.5 * 0.65,   0.0);
	controlPoints[1] = 1.5 * vec3( 0.0, 1.0 * 0.65,   0.0);
	controlPoints[2] = 1.5 * vec3( 2.0, 4.0 * 0.65,   0.0);
	controlPoints[3] = 1.5 * vec3( 0.0, 8.0 * 0.65,   0.0);
	controlPoints[4] = 1.5 * vec3(-2.0, 7.0 * 0.65,   0.0);

	pos      = bezier_value_at(numControlPoints, controlPoints,u);
	forward  = normalize(bezier_slope_at(numControlPoints, controlPoints, u));

	// Manually make a pipe (without using the pipeify function, but same steps):
	vec3 o  = orthogonal(forward);

	// Small bumps:
	float bumpySkin = abs(cos(u * 100.0)) * 0.05;

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
	fwd = mix(fwd, outwd, 0.3); // <- fake rotation 

	outwd *= flipStrength;

	jaws += fwd * u * 3.0 * sin(3.0*v/2.0)
		 + sin(3.0*v/2.0) * outwd * 0.2 * sin(u * PI);

	jaws = mix(jaws, pos + fwd * u * 1.5, ((-u)*(-u)*(-u)+1.0) * dragToInnerRadius);

	//// For the translation:
	//vec3 root = get_giant_worm_body(1.0, offset, userData, /* out: */ outwd, /* out: */ fwd);
	//// For the rotation:
	//get_giant_worm_body(1.0, offset + v, userData, /* out: */ outwd, /* out: */ fwd);
	//outwd = normalize(outwd);
	//fwd   = normalize(fwd);

	//vec3 jaws = get_plane(u, v);
	//// rotate along with the forward vector:
	//jaws = mat3(fwd, cross(outwd, fwd), outwd) * jaws;
	//// translate to the root position:
	//jaws += root;

	return jaws;
}

vec3 get_giant_worm_tongue(float u, float v, uvec3 userData)
{
	u *= 0.9;
	v *= 0.9;

	vec3 pos, outwd, fwd;
	vec3 jaws = get_giant_worm_body(1.0, 0.0, userData,  /* out: */ pos, /* out: */ outwd, /* out: */ fwd);

	vec3 tongue = to_sphere(u, v);
	tongue += pos;

	return tongue;
}
