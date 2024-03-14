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
layout(set = 3, binding = 1) buffer PxFillParams { px_fill_data mElements[]; } uPxFillParams; 
layout(set = 3, binding = 2) buffer PxFillCount  { VkDrawIndirectCommand mDrawParams; } uPxFillCount;

#include "../../shader_includes/param/shape_functions.glsl"

// Varying input from vertex shader:
layout (location = 0) in PerVertexPayload
{
    flat uint mPxFillId; // ... is not varying :ugly:
} vert_in[];

// Varying output to eval shader:
layout (location = 0) out PerControlPointPayload
{ 
	vec2 mParams;
} control_out[];

layout (location = 1) patch out PerPatchPayload
{
    uint mObjectId;
    uvec3 mUserData;
} patch_out;

layout(push_constant) uniform PushConstants
{
    float mConstOuterSubdivLevel;
    float mConstInnerSubdivLevel;
}
pushConstants;

void main()
{
    control_out[gl_InvocationID].mParams = gl_in[gl_InvocationID].gl_Position.xy;

    if (gl_InvocationID == 0)
    {
        const uint pxFillId  = vert_in[0].mPxFillId;
	    const uint objectId  = uPxFillParams.mElements[pxFillId].mObjectIdUserData[0];
	    const uvec3 userData = uPxFillParams.mElements[pxFillId].mObjectIdUserData.yzw;
        patch_out.mObjectId  = objectId;
        patch_out.mUserData  = userData;

        if (ubo.mUseMaxPatchResolutionDuringPxFill) {
            const float absoluteMin =  4.0;
            const float absoluteMax = 64.0;
            const vec2 screenDists = uPxFillParams.mElements[pxFillId].mScreenDists.xy;
            const float threshold  = ubo.mScreenDistanceThreshold;

            const float factorU = screenDists.x / threshold;
            gl_TessLevelOuter[1] = clamp(pushConstants.mConstOuterSubdivLevel * factorU, absoluteMin, absoluteMax);
            gl_TessLevelOuter[3] = clamp(pushConstants.mConstOuterSubdivLevel * factorU, absoluteMin, absoluteMax);
            gl_TessLevelInner[0] = clamp(pushConstants.mConstInnerSubdivLevel * factorU, absoluteMin, absoluteMax);

            const float factorV = screenDists.y / threshold;
            gl_TessLevelOuter[0] = clamp(pushConstants.mConstOuterSubdivLevel * factorV, absoluteMin, absoluteMax);
            gl_TessLevelOuter[2] = clamp(pushConstants.mConstOuterSubdivLevel * factorV, absoluteMin, absoluteMax);
            gl_TessLevelInner[1] = clamp(pushConstants.mConstInnerSubdivLevel * factorV, absoluteMin, absoluteMax);
        }
        else {
            gl_TessLevelOuter[0] = pushConstants.mConstOuterSubdivLevel;
            gl_TessLevelOuter[1] = pushConstants.mConstOuterSubdivLevel;
            gl_TessLevelOuter[2] = pushConstants.mConstOuterSubdivLevel;
            gl_TessLevelOuter[3] = pushConstants.mConstOuterSubdivLevel;

            gl_TessLevelInner[0] = pushConstants.mConstInnerSubdivLevel;
            gl_TessLevelInner[1] = pushConstants.mConstInnerSubdivLevel;
        }
    }
}
