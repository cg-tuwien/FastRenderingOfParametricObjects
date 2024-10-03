#version 460
#extension GL_EXT_nonuniform_qualifier                   : require
#extension GL_EXT_shader_image_int64                     : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive                   : enable
#extension GL_EXT_debug_printf                           : enable
#extension GL_EXT_samplerless_texture_functions          : require

#include "../shader_includes/host_device_shared.h"

layout(set = 0, binding = 0) uniform sampler uColorSampler;
layout(set = 0, binding = 1) uniform sampler uDepthSampler;
layout(set = 0, binding = 2) uniform texture2D uColorTex;
layout(set = 0, binding = 3) uniform texture2D uDepthTex;

// ###### DATA PASSED ON ALONG THE PIPELINE ##############
// Data from vert -> frag:
layout (location = 0) in VertexData {
	vec2 texCoords;
} v_in;
// -------------------------------------------------------

layout (location = 0) out vec4 fs_out;

// assume it may be modified such that its value will only decrease
layout (depth_less) out float gl_FragDepth;

void main() 
{
	fs_out       = texture(sampler2D(uColorTex, uColorSampler), v_in.texCoords);

	ivec2 iuv = ivec2(textureSize(uDepthTex, 0).xy * v_in.texCoords);
	float depth = texelFetch(uDepthTex, iuv, 0).r;
	gl_FragDepth = depth; 

//	gl_FragDepth = v_in.texCoords.x * 0.01;
//	gl_FragDepth = 0.9999;
//	gl_FragDepth = texture(sampler2D(uDepthTex, uDepthSampler), v_in.texCoords).r;
}
