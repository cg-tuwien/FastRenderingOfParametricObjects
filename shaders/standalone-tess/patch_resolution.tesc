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

layout (vertices=4) out;

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
layout(std430, set = 4, binding = 3) buffer IndicesBuffer  { uint mIndices[]; } idxBuffer;

#include "../../shader_includes/param/shape_functions.glsl"
#include "../../shader_includes/parametric_curve_helpers.glsl"

layout(push_constant) uniform PushConstants
{
    uint mObjectId;
    float mInnerTessLevel;
    float mOuterTessLevel;
}
pushConstants;

// Varying input from vertex shader:
layout (location = 0) in PerVertexPayload
{
    vec3 mNormal;
	vec2 mTexCoords;
} vert_in[];

// Varying output to eval shader:
layout (location = 0) out PerControlPointPayload
{ 
	vec3 mPosition;
    vec3 mNormal;
	vec2 mTexCoords;
} control_out[];

float screen_dist_between(vec2 uv0, vec2 uv1)
{
    const uint objectId    = pushConstants.mObjectId;
    const uvec3 userData   = uvec3(0); // NOTE: User data not supported with tessellation standalone
	const mat4 tM          = uObjectData.mElements[objectId].mTransformationMatrix;
	const int  curveIndex  = uObjectData.mElements[objectId].mCurveIndex;

	vec4 rawWS0 = paramToWS(uv0.x, uv0.y, curveIndex, userData);
    vec4 posWS0 = tM * rawWS0;
	vec4 posCS0 = toCS(posWS0);
	vec3 vpc0   = toScreen(posCS0); 

	vec4 rawWS1 = paramToWS(uv1.x, uv1.y, curveIndex, userData);
    vec4 posWS1 = tM * rawWS1;
	vec4 posCS1 = toCS(posWS1);
	vec3 vpc1   = toScreen(posCS1); 

    float l = length(vpc1.xy - vpc0.xy);
//    l -= l * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z * vpc1.z;
    return l * 0.75;
}

void main()
{
    // Regarding gl_Position: 
    // > The use of any of these in a TCS is completely optional. Indeed, their semantics will generally 
    // > be of no practical value to the TCS. They have the same general meaning as for vertex shaders, 
    // > but since a TCS must always be followed by an evaluation shader, the TCS never has to write to any of them.
    // see: https://www.khronos.org/opengl/wiki/Tessellation_Control_Shader
//    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    control_out[gl_InvocationID].mPosition = gl_in[gl_InvocationID].gl_Position.xyz;
    control_out[gl_InvocationID].mNormal = vert_in[gl_InvocationID].mNormal;
    control_out[gl_InvocationID].mTexCoords = vert_in[gl_InvocationID].mTexCoords;
//	if (gl_PrimitiveID == 0) debugPrintfEXT("mParams[%f, %f]", control_out[gl_InvocationID].mParams.x, control_out[gl_InvocationID].mParams.y);

    // TODO: Evaluate parametric function and set tessellation level based on that

    memoryBarrier();
    barrier();

    // ----------------------------------------------------------------------
    // invocation zero controls tessellation levels for the entire patch
    if (gl_InvocationID == 0)
    {
//        debugPrintfEXT("control_out[x].mParams[%f, %f]", control_out[3].mParams.x, control_out[0].mParams.y);
        
        gl_TessLevelOuter[0] = pushConstants.mOuterTessLevel;
        gl_TessLevelOuter[1] = pushConstants.mOuterTessLevel;
        gl_TessLevelOuter[2] = pushConstants.mOuterTessLevel;
        gl_TessLevelOuter[3] = pushConstants.mOuterTessLevel;

        gl_TessLevelInner[0] = pushConstants.mInnerTessLevel;
        gl_TessLevelInner[1] = pushConstants.mInnerTessLevel;
    }
}