// +-------------------------------------------------+
// |   Spiky Heart:                                  |
// |                                                 |
// |   Expected input uv ranges:                     |
// |   u = [0, PI)                                   |
// |   v = [0, TWO_PI)                               |
// +-------------------------------------------------+
vec3 get_spiky_heart(float u, float v)
{
	vec3 object = to_sphere(u, v);

	if (u < HALF_PI) {
		object.y *= 1.0 - (cos(sqrt(sqrt(abs(object.x * PI * 0.7)))) * 0.8);
	}
	else {
		object.x *= sin(u) * sin(u);
	}
	object.x *= 0.90;
	object.z *= 0.40;

	const float PI_ACHTERL = PI / 8.0;
	if (u > PI_ACHTERL && u < PI - PI_ACHTERL - PI_ACHTERL) {
		// Just copy&passteh from above:
		const float NUMSPIKES       = 10.0;
		const float SPIKENARROWNESS = 100.0;
		const float spikeheight     = 0.4;

		float repeatU = u / PI * NUMSPIKES - round(u / PI * NUMSPIKES);
		float repeatV = v / PI * NUMSPIKES - round(v / PI * NUMSPIKES);
		float d       = repeatU * repeatU + repeatV * repeatV;
		float r       = 1.0 + exp(-d * SPIKENARROWNESS) * spikeheight;

		object *= r;
	}
	return object;
}
