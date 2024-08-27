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

// ========= vvv   older utilities from previous project(s)   vvv ========= 

struct Plane
{
	float a, b, c, d;
};

// <0 ... pt lies on the negative halfspace
//  0 ... pt lies on the plane
// >0 ... pt lies on the positive halfspace
float classify_point(Plane plane, vec3 pt)
{
	float d;
	d = plane.a * pt.x + plane.b * pt.y + plane.c * pt.z + plane.d;
	return d;
}

vec4 bone_transform(mat4 BM0, mat4 BM1, mat4 BM2, mat4 BM3, vec4 weights, vec4 positionToTransform)
{
	weights.w = 1.0 - dot(weights.xyz, vec3(1.0, 1.0, 1.0));
	vec4 tr0 = BM0 * positionToTransform;
	vec4 tr1 = BM1 * positionToTransform;
	vec4 tr2 = BM2 * positionToTransform;
	vec4 tr3 = BM3 * positionToTransform;
	return weights[0] * tr0 + weights[1] * tr1 + weights[2] * tr2 + weights[3] * tr3;
}

vec3 bone_transform(mat4 BM0, mat4 BM1, mat4 BM2, mat4 BM3, vec4 weights, vec3 normalToTransform)
{
	weights.w = 1.0 - dot(weights.xyz, vec3(1.0, 1.0, 1.0));
	vec3 tr0 = mat3(BM0) * normalToTransform;
	vec3 tr1 = mat3(BM1) * normalToTransform;
	vec3 tr2 = mat3(BM2) * normalToTransform;
	vec3 tr3 = mat3(BM3) * normalToTransform;
	return weights[0] * tr0 + weights[1] * tr1 + weights[2] * tr2 + weights[3] * tr3;
}

struct bounding_box
{
	vec4 mMin;
	vec4 mMax;
};

vec3[8] bounding_box_corners(bounding_box bb)
{
	vec3 corners[8] = vec3[8](
		vec3(bb.mMin.x, bb.mMin.y, bb.mMin.z),
		vec3(bb.mMin.x, bb.mMin.y, bb.mMax.z),
		vec3(bb.mMin.x, bb.mMax.y, bb.mMin.z),
		vec3(bb.mMin.x, bb.mMax.y, bb.mMax.z),
		vec3(bb.mMax.x, bb.mMin.y, bb.mMin.z),
		vec3(bb.mMax.x, bb.mMin.y, bb.mMax.z),
		vec3(bb.mMax.x, bb.mMax.y, bb.mMin.z),
		vec3(bb.mMax.x, bb.mMax.y, bb.mMax.z)
	);
	return corners;
}

bounding_box transform(bounding_box bbIn, mat4 M)
{
	vec3[8] corners = bounding_box_corners(bbIn);
	// Transform all the corners:
	for (int i = 0; i < 8; ++i) {
		vec4 transformed = (M * vec4(corners[i], 1.0));
		corners[i] = transformed.xyz / transformed.w;
	}

	bounding_box bbOut;
	bbOut.mMin = vec4(corners[0], 0.0);
	bbOut.mMax = vec4(corners[0], 0.0);
	for (int i = 1; i < 8; ++i) {
		bbOut.mMin.xyz = min(bbOut.mMin.xyz, corners[i]);
		bbOut.mMax.xyz = max(bbOut.mMax.xyz, corners[i]);
	}
	return bbOut;
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
