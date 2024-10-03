#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_debug_printf         : enable

#include "../shader_includes/host_device_shared.h"
// -------------------------------------------------------

vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0),
    vec2(0.0, 0.0)
);

// ###### DATA PASSED ON ALONG THE PIPELINE ##############
// Data from vert -> tesc or frag:
layout (location = 0) out VertexData {
	vec2 texCoords;
} v_out;
// -------------------------------------------------------

// ###### VERTEX SHADER MAIN #############################
void main()
{

	v_out.texCoords = positions[gl_VertexIndex];
    gl_Position = vec4(positions[gl_VertexIndex] * 2.0 - 1.0, 0.0, 1.0);
}
// -------------------------------------------------------
