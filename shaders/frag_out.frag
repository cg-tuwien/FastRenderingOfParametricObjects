#version 460
#extension GL_EXT_nonuniform_qualifier                   : require
#extension GL_EXT_shader_image_int64                     : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive                   : enable
#extension GL_EXT_debug_printf                           : enable

#include "../shader_includes/host_device_shared.h"
#include "../shader_includes/util/ui64_conv.glsl"
#include "../shader_includes/common_ubo.glsl"
#include "../shader_includes/material_handling.glsl"

layout (location = 0) out vec4 fs_out;
layout(origin_upper_left) in vec4 gl_FragCoord;

// +------------------------------------------------------------------------------+
// |   Bound Resources                                                            |
// +------------------------------------------------------------------------------+
// layout(set = 0, binding = 0) buffer is contained within common_ubo.glsl
// ###### COMBINED ATTACHMENT #############################
layout(set = 1, binding = 0, r64ui) uniform restrict u64image2D uCombinedAttachment;
#if STATS_ENABLED
layout(set = 1, binding = 1, r32ui) uniform restrict uimage2D   uHeatmapImage;
#endif
layout(set = 2, binding = 0) buffer SsboCounters { uint mCounters[4]; } uCounters;

#include "../shader_includes/combined_attachment_shared.glsl"
// Gotta define some defines before including shading_model.glsl
#define SHADE_ADDITIONAL_PARAMS
#define SAMPLE texture
#define SAMPLE_ADDITIONAL_PARAMS
#include "../shader_includes/shading_model.glsl"

layout (location = 0) in PerVertexData
{
	vec3 positionWS;
	vec3 normalWS;
	vec2 texCoords;
    vec3 shadingUserParams;
	flat int matIndex;
	flat vec3 color;
} v_in;

void main() 
{
	vec3 c = v_in.color;
//	c = vec3(1.0);
	fs_out = vec4(
		shade(v_in.matIndex, c, v_in.shadingUserParams, normalize(v_in.normalWS), v_in.texCoords),
		1.0
	);

	// TODO: Why was this active? vvvvvvv

//	if (ubo.mWriteToCombinedAttachmentInFragmentShader) {
//		// Just write the same to the image as we write to the framebuffer:
//		// TODO: Color-write to the framebuffer no more!
//		writeToCombinedAttachment(ivec2(gl_FragCoord.xy) + ivec2(100, 0), gl_FragCoord.z, fs_out.rgb);
//	}
}
