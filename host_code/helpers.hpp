
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

uint64_t color_to_ui64(glm::vec4 color)
{
    uint64_t R = 0xFF & uint64_t(255.0 * color.x);
    uint64_t G = 0xFF & uint64_t(255.0 * color.y);
    uint64_t B = 0xFF & uint64_t(255.0 * color.z);
    uint64_t A = 0xFF & uint64_t(255.0 * color.a);
    return (R << 24) | (G << 16) | (B << 8) | A;
}

uint64_t depth_to_ui64(float depth)
{
    return static_cast<uint64_t>(*reinterpret_cast<uint32_t*>(&depth));
}


uint64_t combine_depth_and_color(uint64_t depthEncoded, uint64_t colorEncoded)
{
    return (depthEncoded << 32) | colorEncoded;
}