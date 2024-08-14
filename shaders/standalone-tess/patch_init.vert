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

#include "../../shader_includes/parametric_functions/shape_functions.glsl"
#include "../../shader_includes/parametric_curve_helpers.glsl"

layout(push_constant) uniform PushConstants
{
    uint mObjectId;
}
pushConstants;

void main() 
{
	const uint objectId = pushConstants.mObjectId;
	const uint vertexId = gl_VertexIndex / 4;
	const uint vertexSubId = gl_VertexIndex - vertexId * 4;

	// in:
	const vec2 paramsRange = vec2(
		uObjectData.mElements[objectId].mParams[1] - uObjectData.mElements[objectId].mParams[0],
		uObjectData.mElements[objectId].mParams[3] - uObjectData.mElements[objectId].mParams[2]
	);

	uvec2 evalDims = uObjectData.mElements[objectId].mDetailEvalDims.xy;
	const uint vertexRow = vertexId / evalDims.x;
	const uint vertexCol = vertexId - vertexRow * evalDims.x;
//	if (vertexId == 18) debugPrintfEXT("evalDims[%u, %u], vertexRow[%u], vertexCol[%u]", evalDims.x, evalDims.y, vertexRow, vertexCol);

	const vec2 rangePerEvalBucket = paramsRange / vec2(evalDims);
	const vec2 paramsFrom = rangePerEvalBucket * vec2(vertexCol, vertexRow);
	const vec2 paramsTo   = paramsFrom + rangePerEvalBucket;
//	if (vertexId == 5) debugPrintfEXT("paramsFrom[%f, %f], paramsTo[%f, %f]", paramsFrom.x, paramsFrom.y, paramsTo.x, paramsTo.y);

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
