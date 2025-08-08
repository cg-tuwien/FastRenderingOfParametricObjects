#version 460
#extension GL_EXT_nonuniform_qualifier                   : require
#extension GL_EXT_shader_image_int64                     : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive                   : enable

#include "../shader_includes/host_device_shared.h"
#include "../shader_includes/util/ui64_conv.glsl"
#include "../shader_includes/types.glsl"

// ###### BOUND RESOURCES #################################
#include "../shader_includes/common_ubo.glsl"
layout(set = 3, binding = 0) buffer SeashellLodIndexMapping { uint mMappedIndex[]; } uSeashellLodIndexMapping;
layout(set = 3, binding = 1) buffer SeashellLodDrawParamsBuffer { VkDrawIndirectCommand mDrawParams[]; } uSeashellLodDrawParamsBuffer;

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;

// +------------------------------------------------------------------------------+
// |   PushConstants                                                              |
// +------------------------------------------------------------------------------+

layout(push_constant) uniform PushConstants
{
    mat4 mModelMatrix;
    int  mMatIndex;
    int  mLod;
}
pushConstants;

// +------------------------------------------------------------------------------+
// |   in/out and structs                                                         |
// +------------------------------------------------------------------------------+

layout (location = 0) out PerVertexData
{
	vec3 positionWS;
	vec3 normalWS;
	vec2 texCoords;
    vec3 shadingUserParams;
	flat int matIndex;
	flat vec3 color;
} v_out;

#define MAX_COLORS 13
vec3 vertexColors[MAX_COLORS] = {
    vec3(0.987, 0.537, 0.490), // Saturated Peach
    vec3(0.698, 0.911, 0.989), // Saturated Sky Blue
    vec3(0.850, 0.996, 0.737), // Saturated Mint Green
    vec3(0.996, 0.651, 0.529), // Saturated Apricot
    vec3(0.698, 0.741, 0.890), // Saturated Lavender
    vec3(0.949, 0.651, 0.898), // Saturated Lilac
    vec3(0.800, 0.964, 0.651), // Saturated Pistachio
    vec3(0.996, 0.600, 0.690), // Saturated Coral
    vec3(0.851, 0.937, 0.651), // Saturated Mint
    vec3(0.996, 0.827, 0.580), // Saturated Banana
    vec3(0.600, 0.867, 0.996), // Saturated Baby Blue
    vec3(0.851, 0.600, 0.949), // Saturated Orchid
    vec3(0.851, 0.839, 0.600)  // Saturated Beige
};

// ###### MAIN ###########################
void main() {
	vec4 posWS = pushConstants.mModelMatrix * vec4(inPosition.xyz, 1.0);

    // The following if block is for handling the LODs of seashells; it is inactive for Sponza + Terrain:
    // (Note: One could argue that this shader is no longer that simple.vert ^^)
    if (gl_InstanceIndex > 0) {
        int mappingIndex  = int(gl_InstanceIndex.x) - 1;
        int seashellIndex = int(uSeashellLodIndexMapping.mMappedIndex[SEASHELL_LOD_IDS_STRIDE * pushConstants.mLod + mappingIndex]);

        int x = (seashellIndex - 1) / 71;
        int y = (seashellIndex - 1) % 71;
        vec3 seashellPos = vec3(5.0 * (x - 36), 0.0, 5.0 * (y - 36));
        posWS += vec4(seashellPos, 0.0);
    }

	v_out.positionWS = posWS.xyz;
	v_out.normalWS   = mat3(pushConstants.mModelMatrix) * inNormal;
    v_out.texCoords  = inTexCoord;
    v_out.shadingUserParams = (ubo.mViewProjMatrix * vec4(v_out.normalWS, 0.0)).xyz;
    v_out.matIndex   = -5; // -5 + pushConstants.mLod + (pushConstants.mLod > 2 ? 1 : 0);
	v_out.color      = vertexColors[gl_VertexIndex % MAX_COLORS];
    gl_Position      = ubo.mViewProjMatrix * posWS;
}
