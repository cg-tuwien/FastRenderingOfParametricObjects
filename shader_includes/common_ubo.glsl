
// +------------------------------------------------------------------------------+
// |   Uniform Buffer:                                                            |
// +------------------------------------------------------------------------------+

layout(set = 0, binding = 0) uniform FrameData
{
    mat4    mViewMatrix;
    mat4    mViewProjMatrix;
    mat4    mInverseProjMatrix;
    // xy = resoolution, z = bfc on/off
    uvec4   mResolutionAndCulling;
    // Debug sliders:
    vec4    mDebugSliders;
    ivec4   mDebugSlidersi;
    // Common, global settings:
    bool    mHoleFillingEnabled;
    bool    mCreateTrianglesEnabled;
    bool    mLimitFilledPixelsInShaders;
    bool    mHeatMapEnabled;
    bool    mGatherPipelineStats;
    bool    mAdaptivePxFill;
    bool    mUseMaxPatchResolutionDuringPxFill;
    bool    mWriteToCombinedAttachmentInFragmentShader;
    float   mAbsoluteTime;
    float   mDeltaTime;
	float   mScreenDistanceThreshold;
    float   _padding;
} ubo;
