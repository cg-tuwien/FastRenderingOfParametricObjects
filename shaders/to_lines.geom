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

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (location = 0) in PerVertexData
{
	vec3 positionWS;
	vec3 normalWS;
	vec2 texCoords;
    vec3 shadingUserParams;
	flat int matIndex;
	flat vec3 color;
} v_in[];

void GenerateLine(int indexFrom, int indexTo)
{
    gl_Position = gl_in[indexFrom].gl_Position;
    gl_Position.z -= 0.0002;
    EmitVertex();
    gl_Position = gl_in[indexTo].gl_Position;
    gl_Position.z -= 0.0002;
    EmitVertex();
}

void main() 
{
    vec3 p0 = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
    vec3 p1 = gl_in[1].gl_Position.xyz / gl_in[1].gl_Position.w;
    vec3 p2 = gl_in[2].gl_Position.xyz / gl_in[2].gl_Position.w;

    if (cross(p1-p0, p2-p0).z < 0.0) {
        GenerateLine(0, 1); // first  line
        GenerateLine(1, 2); // second line
        GenerateLine(2, 0); // third  line
    }

    EndPrimitive();
}
