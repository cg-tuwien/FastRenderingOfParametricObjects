// +-------------------------------------------------+
// |   Giant Worm:                                   |
// +-------------------------------------------------+
vec3 get_giant_worm_body(float u, float v, uvec3 userData)
{
	const float thickness = 0.5;
	const float numSawtoothElements = 15.0;

	const int numControlPoints = 5;
	vec3 controlPoints[NUM_BEZIER_CONTROL_POINTS];
	controlPoints[0] = vec3( 0.0, 0.0  - 5.0,   0.0);
	controlPoints[1] = vec3( 0.0, 1.0  - 5.0,   0.0);
	controlPoints[2] = vec3( 2.0, 4.0  - 5.0,   1.0);
	controlPoints[3] = vec3( 0.0, 8.0  - 5.0,   0.0);
	controlPoints[4] = vec3(-1.0, 10.0 - 5.0, -1.0);

	vec3 pos     = bezier_value_at(numControlPoints, controlPoints, u);
	vec3 upwards = normalize(bezier_slope_at(numControlPoints, controlPoints, u));

	// Manually make a pipe (without using the pipeify function, but same steps):
	vec3 nTrunk = orthogonal(upwards);

	// Generate palm tree trunk shape:
	float sawtooth = (u * numSawtoothElements) - floor(u * numSawtoothElements);
	float sawtoothscale = sawtooth * 0.3 + 1.0;

	// Let the tree trunk be a bit thicker at the bottom:
	float thicknessscale = 1.0 / (u + 1.0) + 0.5;

	vec3 treeTrunkPos = pos + rotate_around_axis(nTrunk * thickness * sawtoothscale * thicknessscale, upwards, v);
	return treeTrunkPos;
}
