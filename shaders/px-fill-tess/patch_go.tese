#version 460
#extension GL_EXT_nonuniform_qualifier                   : require
#extension GL_EXT_shader_image_int64                     : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive                   : enable
#extension GL_EXT_debug_printf                           : enable

#include "../../shader_includes/host_device_shared.h"
#include "../../shader_includes/util/ui64_conv.glsl"
#include "../../shader_includes/util/glsl_helpers.glsl"
#include "../../shader_includes/types.glsl"
#include "../../shader_includes/common_ubo.glsl"

layout (quads, fractional_odd_spacing, ccw) in;
// TODO: fractional_odd_spacing ^ ?

// +------------------------------------------------------------------------------+
// |   Bound Resources                                                            |
// +------------------------------------------------------------------------------+
// layout(set = 0, binding = 0) buffer is contained within common_ubo.glsl
// layout(set = 1, binding = 0, r64ui) uniform restrict u64image2D uCombinedAttachment;
#if STATS_ENABLED
// layout(set = 1, binding = 1, r32ui) uniform restrict uimage2D   uHeatmapImage;
#endif
layout(set = 2, binding = 0) buffer SsboCounters { uint mCounters[4]; } uCounters;
layout(set = 3, binding = 0) buffer ObjectData   { object_data mElements[]; }  uObjectData;
layout(set = 4, binding = 0) buffer BigDataset { dataset_sh_coeffs mEntries[]; } uBigDataset; 

#include "../../shader_includes/parametric_functions/shape_functions.glsl"
#include "../../shader_includes/parametric_curve_helpers.glsl"

layout (location = 0) in PerControlPointPayload
{
	vec2 mParams;
} eval_in[];

layout (location = 1) patch in PerPatchPayload
{
    uint mObjectId;
    uvec3 mUserData;
} patch_in;

layout (location = 0) out PerVertexData
{
	vec3 positionWS;
	vec3 normalWS;
	vec2 texCoords;
    vec3 shadingUserParams;
	flat int matIndex;
	flat vec3 color;
} eval_out;

layout(push_constant) uniform PushConstants
{
    int   mPxFillParamsBufferOffset;
}
pushConstants;

#define MAX_COLORS 13
vec3 vertexColors[MAX_COLORS] = {
    vec3(0.987, 0.537, 0.490), // Saturated Peach
    vec3(0.698, 0.911, 0.989), // Saturated Sky Blue
    vec3(0.850, 0.996, 0.737), // Saturated Mint Green
    vec3(0.996, 0.651, 0.529), // Saturated Apricot
    vec3(0.698, 0.741, 0.890), // Saturated Lavender
    vec3(0.949, 0.651, 0.898), // Saturated Lilac
    vec3(0.800, 0.964, 0.651), // Saturated Pistachio
    vec3(0.996, 0.600, 0.690), // Saturated Coral
    vec3(0.851, 0.937, 0.651), // Saturated Mint
    vec3(0.996, 0.827, 0.580), // Saturated Banana
    vec3(0.600, 0.867, 0.996), // Saturated Baby Blue
    vec3(0.851, 0.600, 0.949), // Saturated Orchid
    vec3(0.851, 0.839, 0.600)  // Saturated Beige
};

void main() 
{
    // get patch coordinate
    float su = gl_TessCoord.x;
    float sv = gl_TessCoord.y;

    // retrieve params:
    vec2 p00 = eval_in[0].mParams;
    vec2 p01 = eval_in[1].mParams;
    vec2 p10 = eval_in[2].mParams;
    vec2 p11 = eval_in[3].mParams;

    // bilinearly interpolate params:
    vec2 p0 = (p01 - p00) * su + p00;
    vec2 p1 = (p11 - p10) * su + p10;
    vec2 myParams = (p1 - p0) * sv + p0;
    
    // Estimate the deltas for normals calculation:
    float du = (p11.x - p00.x) / gl_TessLevelInner[0];
    float dv = (p11.y - p00.y) / gl_TessLevelInner[1];
//    debugPrintfEXT("du[%f], dv[%f]", du, dv);
//    const float du = 0.001f;
//    const float dv = 0.001f;

	const uint objectId    = patch_in.mObjectId;
    const uvec3 userData   = patch_in.mUserData;
	const mat4 tM          = uObjectData.mElements[objectId].mTransformationMatrix;
	const int  curveIndex  = uObjectData.mElements[objectId].mCurveIndex;
	const int  matIndex    = uObjectData.mElements[objectId].mMaterialIndex; 

	vec4 rawWS = paramToWS(myParams[0], myParams[1], curveIndex, userData);
    vec4 posWS = tM * rawWS;
	vec4 posCS = toCS(posWS);
	
    vec4 rawWS_u = paramToWS(myParams[0] + du, myParams[1]     , curveIndex, userData);
    vec4 posWS_u = tM * rawWS_u;
    vec4 rawWS_v = paramToWS(myParams[0]     , myParams[1] + dv, curveIndex, userData);
    vec4 posWS_v = tM * rawWS_v;

	// Is this point even inside the view frustum?
	uint isOff = is_off_screen(posCS); 

	eval_out.positionWS        = posWS.xyz;
	vec3 rawNormalWS           = calculateNormalWS(posWS.xyz, posWS_u.xyz, posWS_v.xyz, curveIndex, userData);
    eval_out.normalWS          = normalize(rawNormalWS); // Note: Not accounting for non-uniform scale
    eval_out.texCoords         = getParamTexCoords(myParams[0], myParams[1], curveIndex, userData, posWS.xyz);
    eval_out.shadingUserParams = getParamShadingUserParams(myParams[0], myParams[1], curveIndex, userData, rawWS.xyz, objectId);
    eval_out.matIndex          = matIndex;
	eval_out.color             = vertexColors[(gl_PrimitiveID + uint(gl_TessCoord.x * 997.0) + uint(gl_TessCoord.y * 131.0)) % MAX_COLORS];
	gl_Position = posCS;
}
