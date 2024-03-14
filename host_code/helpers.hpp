
template <typename T>
T floorSqrt(T n)
{
    if (n == 0)
        return 0;
    T left  = 1;
    T right = n / 2 + 1;
    T res;

    while (left <= right) {
        T mid = left + ((right - left) / 2);
        if (mid <= n / mid) {
            left = mid + 1;
            res  = mid;
        }
        else {
            right = mid - 1;
        }
    }

    return res;
}

// Get a vector which is orthogonal to the given vector v:
glm::vec3 orthogonal(glm::vec3 v) 
{
	if (v.x == 0 && v.y == 0) {
		return glm::vec3(1, 0, 0);
	} else {
		return glm::normalize(glm::vec3(v.y, -v.x, 0));
	}
}

// Transforms given clip space coordinates into viewport coordinates
glm::vec3 clipSpaceToViewport(glm::vec4 pointCS, glm::vec2 resolution)
{
    glm::vec3 ndc = glm::vec3{pointCS} / pointCS.w;
    glm::vec3 vpc = glm::vec3((glm::vec2{ndc} * 0.5f + 0.5f) * resolution, ndc.z);
    return vpc;
}

template <typename T>
T roundUpToMultipleOf(T number, T multiple)
{
    auto tmp = (number + multiple - 1) / multiple;
    return tmp * multiple;
}
