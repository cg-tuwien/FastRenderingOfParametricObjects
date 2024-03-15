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

//layout (quads, equal_spacing          , ccw) in;
layout (quads, fractional_even_spacing, ccw) in;
//layout (quads, fractional_odd_spacing , ccw) in;

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
layout(std430, set = 4, binding = 0) buffer PositionBuffer { float mData[]; }   posBuffer;
layout(std430, set = 4, binding = 1) buffer NormalsBuffer  { float mData[]; }   nrmBuffer;
layout(std430, set = 4, binding = 2) buffer TexCoords      { float mData[]; }   tcoBuffer;
layout(std430, set = 4, binding  = 3) buffer IndicesBuffer  { uint mIndices[]; } idxBuffer;

#include "../../shader_includes/param/shape_functions.glsl"
#include "../../shader_includes/parametric_curve_helpers.glsl"

layout(push_constant) uniform PushConstants
{
    int mMaterialIndex;
    float mInnerTessLevel;
    float mOuterTessLevel;
	float mBulginess;
}
pushConstants;

layout (location = 0) in PerControlPointPayload
{
	vec3 mPosition;
    vec3 mNormal;
	vec2 mTexCoords;
} eval_in[];

layout (location = 0) out PerVertexData
{
	vec3 positionWS;
	vec3 normalWS;
	vec2 texCoords;
    vec3 shadingUserParams;
	flat int matIndex;
	flat vec3 color;
} eval_out;

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

#define BEZIERIFY 1

void main() 
{
    // get patch coordinate
    float su = gl_TessCoord.x;
    float sv = gl_TessCoord.y;

    // retrieve params:
    vec3 p00 = eval_in[0].mPosition;
    vec3 p01 = eval_in[1].mPosition;
    vec3 p10 = eval_in[2].mPosition;
    vec3 p11 = eval_in[3].mPosition;

    // retrieve params:
    vec3 n00 = eval_in[0].mNormal;
    vec3 n01 = eval_in[1].mNormal;
    vec3 n10 = eval_in[2].mNormal;
    vec3 n11 = eval_in[3].mNormal;

    // retrieve params:
    vec2 t00 = eval_in[0].mTexCoords;
    vec2 t01 = eval_in[1].mTexCoords;
    vec2 t10 = eval_in[2].mTexCoords;
    vec2 t11 = eval_in[3].mTexCoords;

#if BEZIERIFY
    vec3 cp0 = p00, cp3 = p01, cp12 = p10, cp15 = p11;
    vec3 cp1 = mix(cp0 , cp3,  0.25),  cp2 = mix(cp0 , cp3,  0.75);
    vec3 cp4 = mix(cp0 , cp12, 0.25),  cp8 = mix(cp0 , cp12, 0.75);
    vec3 cp7 = mix(cp3 , cp15, 0.25), cp11 = mix(cp3 , cp15, 0.75);
    vec3 cp13= mix(cp12, cp15, 0.25), cp14 = mix(cp12, cp15, 0.75);
    vec3 cp5 = mix(cp4, cp7,  0.25),  cp6 = mix(cp4, cp7,  0.75);
    vec3 cp9 = mix(cp8, cp11, 0.25), cp10 = mix(cp8, cp11, 0.75);
    float bs = pushConstants.mBulginess;
    vec3 cps[16] = {
        cp0 , cp1            , cp2            , cp3 ,
        cp4 , cp5 + n00 * bs , cp6  + n00 * bs, cp7 ,
        cp8 , cp9 + n00 * bs , cp10 + n00 * bs, cp11,
        cp12, cp13           , cp14           , cp15
    };

    vec3 myPosition, myNormal;
    DeCasteljauBicubic(vec2(su, sv), cps, myPosition, myNormal);
#endif

#if !BEZIERIFY 
    // bilinearly interpolate positions:
    vec3 p0 = (p01 - p00) * su + p00;
    vec3 p1 = (p11 - p10) * su + p10;
    vec3 myPosition = (p1 - p0) * sv + p0;

    // bilinearly interpolate normals:
    vec3 n0 = (n01 - n00) * su + n00;
    vec3 n1 = (n11 - n10) * su + n10;
    vec3 myNormal = (n1 - n0) * sv + n0;
#endif

    // bilinearly interpolate texture coordinates:
    vec2 t0 = (t01 - t00) * su + t00;
    vec2 t1 = (t11 - t10) * su + t10;
    vec2 myTexCoords = (t1 - t0) * sv + t0;

//    // Estimate the deltas for normals calculation:
//    float du = (p11.x - p00.x) / gl_TessLevelInner[0];
//    float dv = (p11.y - p00.y) / gl_TessLevelInner[1];
//
//	const uint objectId    = pushConstants.mObjectId;
//    const uvec3 userData   = uvec3(0); // NOTE: User data not supported with tessellation standalone
//	const mat4 tM          = uObjectData.mElements[objectId].mTransformationMatrix;
//	const int  curveIndex  = uObjectData.mElements[objectId].mCurveIndex;
//	const int  matIndex    = uObjectData.mElements[objectId].mMaterialIndex; 
//
//	vec4 rawWS = paramToWS(myParams[0], myParams[1], curveIndex, userData);
//    vec4 posWS = tM * rawWS;
//	vec4 posCS = toCS(posWS);
//	
//	// Is this point even inside the view frustum?
//	uint isOff = is_off_screen(posCS); 
//
//    vec4 rawWS_u = paramToWS(myParams[0] + du, myParams[1]     , curveIndex, userData);
//    vec4 posWS_u = tM * rawWS_u;
//    vec4 rawWS_v = paramToWS(myParams[0]     , myParams[1] + dv, curveIndex, userData);
//    vec4 posWS_v = tM * rawWS_v;

	eval_out.positionWS        = myPosition;
//	vec3 rawNormalWS           = calculateNormalWS(posWS.xyz, posWS_u.xyz, posWS_v.xyz, curveIndex, userData);
    eval_out.normalWS          = myNormal;
    eval_out.texCoords         = myTexCoords;
    eval_out.shadingUserParams = vec3(0.0);
    eval_out.matIndex          = pushConstants.mMaterialIndex;
	eval_out.color             = vertexColors[(gl_PrimitiveID + uint(gl_TessCoord.x * 997.0) + uint(gl_TessCoord.y * 131.0)) % MAX_COLORS];
	gl_Position = ubo.mViewProjMatrix * vec4(myPosition, 1.0);
}
