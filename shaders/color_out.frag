#version 460
#extension GL_EXT_nonuniform_qualifier                   : require
#extension GL_EXT_shader_image_int64                     : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive                   : enable
#extension GL_EXT_debug_printf                           : enable

layout (location = 0) out vec4 fs_out;

void main() 
{
	fs_out = vec4(1.0, 0.7, 0.0, 1.0);
}
