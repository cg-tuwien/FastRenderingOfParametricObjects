
#include "parametric_functions/plane.glsl"
#include "parametric_functions/palm_tree_trunk.glsl"
#include "parametric_functions/johis_heart.glsl"
#include "parametric_functions/spiky_heart.glsl"
#include "parametric_functions/sh_glyph.glsl"
#include "parametric_functions/single_yarn_curve.glsl"
#include "parametric_functions/single_fiber_curve.glsl"
#include "parametric_functions/curtain_yarn_curve.glsl"
#include "parametric_functions/curtain_fiber_curve.glsl"
#include "parametric_functions/seashells.glsl"
#include "parametric_functions/giant_worm.glsl"

// +------------------------------------------------------------------------------+
// |   Parametric Curve Helpers:                                                  |
// +------------------------------------------------------------------------------+

// Gets texture coordinates per parametric curve
vec2 getParamTexCoords(float u, float v, int curveIndex, uvec3 userData, vec3 posWS) {
    vec2 texCoords = vec2(u, v);
    switch (curveIndex) {
        // +--------------------+
        // | Curtains:          |
        // +--------------------+
	    case 9: 
	    case 10: 
            texCoords = vec2(3.55 - posWS.x / 7.38, posWS.y / 7.4);
            break;
    }
    return texCoords;
}

// Gets user parameters for shading
vec3 getParamShadingUserParams(float u, float v, int curveIndex, uvec3 userData, vec3 posWS_untranslated, uint objectId) {
    vec3 shadingUserParams = vec3(userData);
    switch (curveIndex) {
        // +-------------------------------------------------+
        // |   SH Glyphs                                     |
        // +-------------------------------------------------+
        case 5:
            {
                shadingUserParams = posWS_untranslated;
            }
            break;
	    case 6:
            {
                uvec2 datasetDims = userData.xy;
                uint  glyphId     = userData.z;
                shadingUserParams = vec3(posWS_untranslated) - get_translation(datasetDims, glyphId);
            }
            break;
        // +-------------------------------------------------+
        // |   Seashells                                     |
        // +-------------------------------------------------+
        case 11:
        case 12:
        case 13:
            shadingUserParams.y = smoothstep(TWO_PI, TWO_PI + TWO_PI, u);
            break;
    }
    return shadingUserParams;
}


// Calculates the normal at posWS, based on the position of two of its 
// neighbor, namely:
//  - neighborUPosWS ... Position that results from evaluating the parametric curve
//                       direction of positively increasing u parameter.
//  - neighborVPosWS ... Position that results from evaluating the parametric curve
//                       direction of positively increasing v parameter.
vec3 calculateNormalWS(vec3 posWS, vec3 neighborUPosWS, vec3 neighborVPosWS, int curveIndex, uvec3 userData) {
    vec3 n = cross(posWS - neighborUPosWS, neighborVPosWS - posWS);
    n = normalize(n);
    return n;
}

vec4 paramToWS(float u, float v, int curveIndex, uvec3 userData)
{
    if (ubo.mGatherPipelineStats) {
        atomicAdd(uCounters.mCounters[0], 1);
    }

    vec3 object;
    switch (curveIndex) {
        case 0:
            object = get_plane(u, v);
            break;
        case 1:
            object = to_sphere(u, v);
            break;
        case 2:
            object = get_palm_tree_trunk(u, v);
            break;
        case 3:
            object = get_johis_heart(u, v);
            break;
        case 4:
            object = get_spiky_heart(u, v);
            break;
        case 5:
            object = get_sh_glyph(u, v, userData, /* Single SH glyph reads from: */ 0);
            break;
        case 6:
            object = get_sh_glyph(u, v, userData, /* SH brain glyphs read from: */ 1);
            break;
	    case 7: 
            object = get_yarn_curve(u, v, userData);
            break;
	    case 8: 
            object = get_fiber_curve(u, v, userData);
            break;
	    case 9: 
            object = get_curtain_yarn(u, v, userData);
            break;
	    case 10: 
            object = get_curtain_fiber(u, v, userData);
            break;
	    case 11: 
            object = get_seashell1(u, v);
            break;
	    case 12: 
            object = get_seashell2(u, v);
            break;
	    case 13: 
            object = get_seashell3(u, v);
            break;
        case 14: // Giant worm body 
            {
                vec3 notNeeded1, notNeeded2, notNeeded3;
                object = get_giant_worm_body(u, v, userData, notNeeded1, notNeeded2, notNeeded3);
            }
            break;
        case 15: // Giant worm mouth piece (inside)
            object = get_giant_worm_jaws(u, TWO_PI - v, -TWO_PI/6.0 + float(userData.x) * TWO_PI/3.0, -0.7, 0.5, userData);
            break;
        case 16: // Giant worm mouth piece (outside)
            object = get_giant_worm_jaws(u, v, -TWO_PI/6.0 + float(userData.x) * TWO_PI/3.0, 1.0, 0.0, userData);
            break;
        case 17: // Giant worm tongue
            object = get_giant_worm_tongue(u, v, userData);
            break;
        case 18: // Giant worm teeth 
            {
                object  = to_cone(u * PI, v);
                object.y = 1.0 - object.y;
                object *= vec3(0.025, 0.1, 0.025);
                //object = to_sphere(u * PI, v, 0.1f);
                float eps = 0.01;
                float discreteV = TWO_PI - 0.55 - TWO_PI / 22.0 * float(userData.y);
                vec3 jaws0 = get_giant_worm_jaws(0.85 + 0.12 * sin(discreteV * 0.5)      , discreteV      , -TWO_PI/6.0 + float(userData.x) * TWO_PI/3.0, -0.7, 0.5, userData);
                vec3 jawsU = get_giant_worm_jaws(0.85 + 0.12 * sin(discreteV * 0.5) - eps, discreteV      , -TWO_PI/6.0 + float(userData.x) * TWO_PI/3.0, -0.7, 0.5, userData);
                vec3 jawsV = get_giant_worm_jaws(0.85 + 0.12 * sin(discreteV * 0.5)      , discreteV - eps, -TWO_PI/6.0 + float(userData.x) * TWO_PI/3.0, -0.7, 0.5, userData);
                jawsU  = normalize(jawsU - jaws0);
                jawsV  = normalize(jawsV - jaws0);
                vec3 X = cross(jawsU, jawsV);
                mat3 jawtrix = mat3(jawsU, X, jawsV);
                // rotate with jaws:
                object = jawtrix * object;
                // translate to jaws:
                object += jaws0 + X * 0.17;
            }
            break;
    }

    // Return homogeneous world space:
    return vec4(object, 1.0);
}

vec4 toCS(vec4 posWS)
{
    vec4 posCS = ubo.mViewProjMatrix * posWS;
    return posCS;
}

vec3 toNDC(vec4 posCS)
{
    return posCS.xyz / posCS.w;
}

vec4 clampCS(vec4 posCS)
{
    float w = abs(posCS.w);
    posCS.x = clamp(posCS.x,  -w, w);
    posCS.y = clamp(posCS.y,  -w, w);
    posCS.z = clamp(posCS.z, 0.0, w);
    return posCS;
}

vec3 toScreen(vec4 posCS)
{
    posCS = clampCS(posCS);
    return cs_to_viewport(posCS, ubo.mResolutionAndCulling.xy);
}
