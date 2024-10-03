// +-------------------------------------------------+
// |   Plane:                                        |
// +-------------------------------------------------+
vec3 get_plane(float u, float v)
{
	vec3 object = to_plane(u, v);
	swap(object.y, object.z); // corresponds to rotate_x(plane, HALF_PI);
	return object;
}
