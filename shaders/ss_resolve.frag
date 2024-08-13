#version 460
#extension GL_EXT_nonuniform_qualifier                   : require
#extension GL_EXT_shader_image_int64                     : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive                   : enable
#extension GL_EXT_debug_printf                           : enable
#extension GL_EXT_samplerless_texture_functions : require

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

void main() 
{
	fs_out       = texture(sampler2D(uColorTex, uColorSampler), v_in.texCoords);
	gl_FragDepth = texture(sampler2D(uDepthTex, uDepthSampler), v_in.texCoords).r;
}
