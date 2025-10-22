

// +------------------------------------------------------------------------------+
// |   Uniform Buffer:                                                            |
// +------------------------------------------------------------------------------+

layout(set = 0, binding = 0) uniform FrameData
{
    mat4    mViewMatrix;
    mat4    mViewProjMatrix;
    mat4    mInverseProjMatrix;
	mat4    mCameraTransform;
    // xy = resoolution, z = bfc on/off
    uvec4   mResolutionAndCulling;
    // Debug sliders:
    vec4    mDebugSliders;
    ivec4   mDebugSlidersi;
    // Frustum culling for LODs of seashells:
    vec4    mFrustumPlanes[6];
    // Common, global settings:
    bool    mHeatMapEnabled;
    bool    mGatherPipelineStats;
    bool    _unused;
    bool    mWriteToCombinedAttachmentInFragmentShader;
    float   mAbsoluteTime;
    float   mDeltaTime;
    float   _padding[2];
} ubo;
