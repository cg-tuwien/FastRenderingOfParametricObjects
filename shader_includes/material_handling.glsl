
layout(set = 0, binding = 1) uniform sampler2D textures[];

struct MaterialGpuData
{
    vec4 mDiffuseReflectivity;
    vec4 mAmbientReflectivity;
    vec4 mSpecularReflectivity;
    vec4 mEmissiveColor;
    vec4 mTransparentColor;
    vec4 mReflectiveColor;
    vec4 mAlbedo;

    float mOpacity;
    float mBumpScaling;
    float mShininess;
    float mShininessStrength;

    float mRefractionIndex;
    float mReflectivity;
    float mMetallic;
    float mSmoothness;

    float mSheen;
    float mThickness;
    float mRoughness;
    float mAnisotropy;

    vec4 mAnisotropyRotation;
    vec4 mCustomData;

    int mDiffuseTexIndex;
    int mSpecularTexIndex;
    int mAmbientTexIndex;
    int mEmissiveTexIndex;
    int mHeightTexIndex;
    int mNormalsTexIndex;
    int mShininessTexIndex;
    int mOpacityTexIndex;
    int mDisplacementTexIndex;
    int mReflectionTexIndex;
    int mLightmapTexIndex;
    int mExtraTexIndex;

    vec4 mDiffuseTexOffsetTiling;
    vec4 mSpecularTexOffsetTiling;
    vec4 mAmbientTexOffsetTiling;
    vec4 mEmissiveTexOffsetTiling;
    vec4 mHeightTexOffsetTiling;
    vec4 mNormalsTexOffsetTiling;
    vec4 mShininessTexOffsetTiling;
    vec4 mOpacityTexOffsetTiling;
    vec4 mDisplacementTexOffsetTiling;
    vec4 mReflectionTexOffsetTiling;
    vec4 mLightmapTexOffsetTiling;
    vec4 mExtraTexOffsetTiling;
};

layout(set = 0, binding = 2) buffer Material
{
    MaterialGpuData materials[];
}
materialsBuffer;
