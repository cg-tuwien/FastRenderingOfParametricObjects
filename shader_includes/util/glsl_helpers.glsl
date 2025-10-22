// ========= vvv       projection/clip space utilities        vvv ========= 

// Tests whether pointCS (given in clip space) is outside the view frustum.
// Returns a value > 0 to indicate which side it of the view frustum it is outside of.
// Returns 0 if it is inside the view frustum.
uint is_off_screen(vec4 vertex)
{
    return 
		(vertex.z < -vertex.w ? 1  : 0)  | 
		(vertex.z >  vertex.w ? 2  : 0)  | 
		(vertex.x < -vertex.w ? 4  : 0)  | 
		(vertex.x >  vertex.w ? 8  : 0)  |
        (vertex.y < -vertex.w ? 16 : 0)  | 
		(vertex.y >  vertex.w ? 32 : 0);
}

// Transforms given clip space coordinates into viewport coordinates
vec3 cs_to_viewport(vec4 pointCS, vec2 resolution)
{
    vec3 ndc = pointCS.xyz / pointCS.w;
    vec3 vpc = vec3((ndc.xy * 0.5 + 0.5) * resolution, ndc.z);
    return vpc;
}

// Transforms given viewport coordinates into view space
vec3 viewport_to_vs(vec3 vpc, vec2 resolution, mat4 inverseProjMat)
{
	vec3 cs = vec3((vpc.xy / resolution - 0.5) * 2.0, vpc.z);
	vec4 ip = inverseProjMat * vec4(cs, 1.0);
	vec3 vs = ip.xyz / ip.w;
	return vs;
}

// <0 ... pt lies on the negative halfspace   TO BE VERIFIED
//  0 ... pt lies on the plane                TO BE VERIFIED
// >0 ... pt lies on the positive halfspace   TO BE VERIFIED
float classify_point(vec4 plane, vec3 pt)
{
	return dot(plane, vec4(pt, 1.0));
}

// ========= vvv   some convenience functions  vvv ========= 

int maxOf(ivec2 v)
{
	return max(v.x, v.y);
}

int maxOf(ivec3 v)
{
	return max(max(v.x, v.y), v.z);
}

int maxOf(ivec4 v)
{
	return max(max(max(v.x, v.y), v.z), v.w);
}

float maxOf(vec2 v)
{
	return max(v.x, v.y);
}

float maxOf(vec3 v)
{
	return max(max(v.x, v.y), v.z);
}

float maxOf(vec4 v)
{
	return max(max(max(v.x, v.y), v.z), v.w);
}

int minOf(ivec2 v)
{
	return min(v.x, v.y);
}

int minOf(ivec3 v)
{
	return min(min(v.x, v.y), v.z);
}

int minOf(ivec4 v)
{
	return min(min(min(v.x, v.y), v.z), v.w);
}

float minOf(vec2 v)
{
	return min(v.x, v.y);
}

float minOf(vec3 v)
{
	return min(min(v.x, v.y), v.z);
}

float minOf(vec4 v)
{
	return min(min(min(v.x, v.y), v.z), v.w);
}

// Very stupidly guesstimate how many pixels there are to be filled between the 
// corners of a quad, the corners of which are to be given in (counter-)clockwise order:
vec2 guesstimatePixelsCovered(vec3 corner1, vec3 corner2, vec3 corner3, vec3 corner4)
{
    vec2 pixelDists = vec2(
        max(length(corner2 - corner1), length(corner3 - corner4)),
        max(length(corner4 - corner1), length(corner3 - corner2)) // Attention: Under estimation for many (most?) cases
	);
    return pixelDists;
}

// Just multiplies the two dimensions that are retured by guesstimatePixelsCovered
int guesstimateNumberOfPixelsCovered(vec3 corner1, vec3 corner2, vec3 corner3, vec3 corner4) 
{
    vec2 pixelDists = guesstimatePixelsCovered(corner1, corner2, corner3, corner4);
    return int(ceil(pixelDists.x) * ceil(pixelDists.y));
}

// Example usage: 
// var subgroupInvocationId = calcInvocationIdFrom2DIndices(gl_LocalInvocationID, gl_WorkGroupSize);
uint calcInvocationIdFrom2DIndices(uvec2 indices, uvec2 size)
{
	return indices.y * size.x + indices.x;
}

// Get a point that is bilinearly interpolated according to u and v interpolation factor
vec3 getBilinearInterpolated(vec3 Pos0, vec3 PosU, vec3 PosV, vec3 PosUV, float u, float v)
{
	vec3 P =  Pos0  * (1.0 - u) * (1.0 - v)
			+ PosU  * u * (1.0 - v) 
			+ PosV  * (1.0 - u) * v
			+ PosUV * u * v;
	return P;
}

// Get a point that is bilinearly interpolated according to u and v interpolation factor
vec4 getBilinearInterpolated(vec4 Pos0, vec4 PosU, vec4 PosV, vec4 PosUV, float u, float v)
{
	vec4 P =  Pos0  * (1.0 - u) * (1.0 - v)
			+ PosU  * u * (1.0 - v) 
			+ PosV  * (1.0 - u) * v
			+ PosUV * u * v;
	return P;
}
