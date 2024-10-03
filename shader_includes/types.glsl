
// +------------------------------------------------------------------------------+
// | DEVICE SIDE:                                                                 |
// |   Some common types, used across all kinds of shader files in this project   |
// +------------------------------------------------------------------------------+

struct VkDrawIndirectCommand {
    uint    vertexCount;
    uint    instanceCount;
    uint    firstVertex;
    uint    firstInstance;
};

struct object_data
{
    vec4  mParams;
    uvec4 mDetailEvalDims;
    mat4  mTransformationMatrix;
    int   mCurveIndex;
    int   mMaterialIndex;
    // The following means "adaptive tessellation levels" (for tessellation-based rendering) or "adaptive sampling" (for point-based rendering):
    int   mUseAdaptiveDetail;
    int   mRenderingVariantIndex;
    // The following settings are stored in the vec4:
    //  .xy ... Percent how much to incre ase patch parameters (s.t. neighboring patches overlap a bit)
    //  .z  ... screen-space threshold for the LOD stage
    //  .w  ... sampling factor for point-based rendering (< 1.0 probably means too little samples, > 1.0 might mean oversampling but also filling holes) 
    vec4  mLodAndRenderSettings;
    // .xy: Tessellation settings: constant inner (x) and outer (y) tessellation levels, i.e., IF mUseAdaptiveDetail == 1
    // .zw: Sampling density for point-based rendering techniques in u (z) and v (w) directions.
    //      (< 1.0 probably means too little samples, > 1.0 might mean oversampling but also filling holes) 
    vec4  mTessAndSamplingSettings;
    // The first three values of user data are being passed-along and can be accessed:
    // Note: SH glyphs and yarn/fiber curves get fixed user data (assigned in pass1_init_shbrain.comp or pass1_init_kityarn.comp, respectively).
    uvec4 mUserData;
};

struct px_fill_data
{
    // [0..1]: u from->to
    // [2..3]: v from->to
    vec4 mParams;

    // [0]: object id
    // [1]: Used for row offset, but only with knit yarn
    // [2]: unused
    // [3]: unused
    uvec4 mObjectIdUserData;

    // [0]: max. screen dist u
    // [1]: max. screen dist v
    // [2]: hybrid levels to go (counting down)
    // [3]: hybrid screen threshold divisor
    vec4 mScreenDistsHybridData;

    // [0]: min screen coords of patch u (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    // [1]: min screen coords of patch v (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    // [2]: max screen coords of patch u (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    // [3]: max screen coords of patch v (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    vec4 mScreenMinMax;
};

void getParams(vec4 data, out float uFrom, out float uTo, out float vFrom, out float vTo)
{
    uFrom = data[0];
    uTo   = data[1];
    vFrom = data[2];
    vTo   = data[3];
}

vec4 setParams(float uFrom, float uTo, float vFrom, float vTo)
{
    return vec4(uFrom, uTo, vFrom, vTo);
}

uint getObjectId(uvec4 aObjectId_Stats)
{
    return aObjectId_Stats[0];
}

struct vertex_buffers_offsets_sizes
{
    uint mPositionsOffset;
    uint mPositionsSize;

    uint mNormalsOffset;
    uint mNormalsSize;

    uint mTexCoordsOffset;
    uint mTexCoordsSize;

    uint mIndicesOffset;
    uint mIndicesSize;
};

struct dataset_sh_coeffs
{
    float mCoeffs[92];
};
