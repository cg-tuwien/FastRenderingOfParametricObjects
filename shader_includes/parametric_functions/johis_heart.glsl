// +-------------------------------------------------+
// |   Johi's Heart:                                 |
// |                                                 |
// |   Expected input uv ranges:                     |
// |   u = [0, PI)                                   |
// |   v = [0, TWO_PI)                               |
// +-------------------------------------------------+
vec3 get_johis_heart(float u, float v)
{
	vec3 object = to_sphere(u, v);

	if (u < HALF_PI) {
		object.y *= 1.0 - (cos(sqrt(sqrt(abs(object.x * PI * 0.7)))) * 0.8);
	}
	else {
		object.x *= sin(u) * sin(u);
	}
	object.x *= 0.9;
	object.z *= 0.4;
	return object;
}
