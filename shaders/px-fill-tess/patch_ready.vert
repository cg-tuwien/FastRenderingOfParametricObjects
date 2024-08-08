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
layout(set = 3, binding = 2) buffer PxFillCount  { VkDrawIndirectCommand mDrawParams[]; } uPxFillCounts;

#include "../../shader_includes/param/shape_functions.glsl"
#include "../../shader_includes/parametric_curve_helpers.glsl"

// Varying output from vertex shader:
layout (location = 0) out PerVertexPayload
{
    flat uint mPxFillId; // ... is not varying :ugly:
} vert_out;

void main() 
{
	uint pxFillId = gl_InstanceIndex - 1; // Gotta subtract 1 (see clear_r64_image.comp)
	vert_out.mPxFillId = pxFillId;
	
	const uint  objectId           = uPxFillParams.mElements[pxFillId].mObjectIdUserData[0];
	const int   pxFillSetIndex     = uObjectData.mElements[objectId].mPxFillSetIndex;
	pxFillId  += MAX_INDIRECT_DISPATCHES * pxFillSetIndex;

	// in: 
	const vec2 paramsFrom = vec2(
		uPxFillParams.mElements[pxFillId].mParams[0],
		uPxFillParams.mElements[pxFillId].mParams[2]
	);
	const vec2 paramsTo = vec2(
		uPxFillParams.mElements[pxFillId].mParams[1],
		uPxFillParams.mElements[pxFillId].mParams[3]
	);
	const uint vertexSubId = gl_VertexIndex - 1; // Gotta subtract 1 (see clear_r64_image.comp)

	switch (vertexSubId)
	{
	case 0:
		gl_Position = vec4(paramsFrom.x, paramsFrom.y, 0.0, 1.0);
		break;
	case 1:
		gl_Position = vec4(paramsTo.x,   paramsFrom.y, 0.0, 1.0);
		break;
	case 2:
		gl_Position = vec4(paramsFrom.x, paramsTo.y,   0.0, 1.0);
		break;
	case 3:
		gl_Position = vec4(paramsTo.x,   paramsTo.y,   0.0, 1.0);
		break;
	default: 
		debugPrintfEXT("ERROROROR");
	}
}
