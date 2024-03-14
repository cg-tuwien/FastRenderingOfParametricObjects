
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
    uvec4 mDetailEvalDims; // Think: Task Shader Dimensions
    mat4  mTransformationMatrix;
    int   mCurveIndex;
    int   mMaterialIndex;
    int   padding[2];
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

    // [0]: EITHER max screen dist u
    // [1]: EITHER max screen dist v
    // [2]: min screen coords of patch u (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    // [3]: min screen coords of patch v (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    vec4 mScreenDists;

#if WRITE_MAX_COORDS_IN_PASS2
    // [0]: min screen coords of patch u (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    // [1]: min screen coords of patch v (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    vec4 mScreenMax;
#endif
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
