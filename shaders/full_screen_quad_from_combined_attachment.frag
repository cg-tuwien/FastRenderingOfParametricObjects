#version 460
#extension GL_EXT_nonuniform_qualifier                   : require
#extension GL_EXT_shader_image_int64                     : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive                   : enable
#extension GL_EXT_debug_printf                           : enable
#extension GL_EXT_samplerless_texture_functions : require

#include "../shader_includes/host_device_shared.h"
#include "../shader_includes/util/ui64_conv.glsl"

layout(set = 0, binding = 0) uniform sampler uColorSampler;
layout(set = 0, binding = 1) uniform sampler uDepthSampler;
layout(set = 1, binding = 0, r64ui)  uniform restrict   u64image2D uCombinedAttachment;
#if STATS_ENABLED
layout(set = 1, binding = 1, r32ui) readonly uniform restrict   uimage2D   uHeatmapImage;
#endif

#define MAX_COLORS 23
vec3 heatmapColors[MAX_COLORS] = {
    vec3(0.18, 0.0, 0.29),  // 0 writes 
    vec3(0.3, 0.57, 0.13),  // dark green:    1 write  ... perfect 
    vec3(0.5, 0.74, 0.25),  // medium green:  2 writes ... as good as one can hope for
    vec3(0.72, 0.88, 0.53), // light green:   3 writes ... still okay
    vec3(1.0, 1.0, 0.75),   // bright yellow: 4 writes ... we have to recon with that in some regions due to hole filling, but we definitely do not want more than this
    vec3(1.0, 1.0, 1.0),    // white:         5 writes
    vec3(0.99, 0.88, 0.94), // pink tones:    6--10 writes
    vec3(0.95, 0.71, 0.85),	// from
    vec3(0.87, 0.47, 0.68),	// light
    vec3(0.77, 0.11, 0.49),	// to
    vec3(0.56, 0.0, 0.32),  // dark pink
    vec3(0.27, 0.46, 0.71), // blue tones:    11--14 writes
    vec3(0.45, 0.68, 0.82),	// from dark
    vec3(0.67, 0.85, 0.91),	// to
    vec3(0.88, 0.95, 0.97), // light blue
    vec3(1.0, 0.88, 0.56),  // orange tones:  15--18 writes
    vec3(0.99, 0.68, 0.38),	// from light
    vec3(0.96, 0.43, 0.26),	// to
    vec3(0.84, 0.19, 0.15),	// very dark orange ... very bad already
    vec3(0.95, 0.0, 0.22),  // red tones:     19--22 writes 
    vec3(0.65, 0.0, 0.15),	// from light
    vec3(0.45, 0.0, 0.1),	// to
    vec3(0.35, 0.0, 0.08)	// very dark red    ... super gau
};

layout(push_constant) uniform PushConstants 
{
	int mWhatToCopy; // 0 ... rendering output, 1 ... heat map
} pushConstants;


// ###### DATA PASSED ON ALONG THE PIPELINE ##############
// Data from vert -> frag:
layout (location = 0) in VertexData {
	vec2 texCoords;
} v_in;
// -------------------------------------------------------

layout (location = 0) out vec4 fs_out;

void main() 
{
	ivec2 iuv = ivec2(imageSize(uCombinedAttachment).xy * v_in.texCoords);

#if STATS_ENABLED
	switch (pushConstants.mWhatToCopy) {
	case 0:
		{
#endif
			uint64_t inp = imageLoad(uCombinedAttachment, iuv).r;
//			uint64_t r = 0xFF & (inp >> (32     ));
//			uint64_t g = 0xFF & (inp >> (32 +  8));
//			uint64_t b = 0xFF & (inp >> (32 + 16));
//			imageStore(uDst, iuv, vec4(intBitsToFloat(int(r)) * 255.0,
//									intBitsToFloat(int(g)) * 255.0,
//									intBitsToFloat(int(b)) * 255.0,
//									1.0));

			int bytes[8] = {
				int(0xFF & (inp)),
				int(0xFF & (inp >> 8)),
				int(0xFF & (inp >> 16)),
				int(0xFF & (inp >> 24)),
				int(0xFF & (inp >> 32)),
				int(0xFF & (inp >> 40)),
				int(0xFF & (inp >> 48)),
				int(0xFF & (inp >> 56))
			};

//			vec4 oup = vec4(
//				float(bytes[0]) / 255.0,
//				float(bytes[1]) / 255.0,
//				float(bytes[2]) / 255.0,
//				1.0
//			);

			float depth;
			vec4  color;
			extract_depth_and_color(inp, depth, color);

			fs_out       = color;
			gl_FragDepth = depth;
#if STATS_ENABLED
		}
		break;
	case 1:
		{
			uint inp = imageLoad(uHeatmapImage, iuv).r;
			inp = clamp(inp, 0, MAX_COLORS-1);
			fs_out       = vec4(heatmapColors[inp], 1.0);
			gl_FragDepth = 1e-5;
		}
		break;
	default:
		fs_out       = vec4(0.0);
		gl_FragDepth = 0.99999999;
		break;
	}
#endif
}
