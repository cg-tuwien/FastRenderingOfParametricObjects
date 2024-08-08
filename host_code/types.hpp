
#include "../shader_includes/host_device_shared.h"

/** Contains the necessary buffers for drawing everything */
struct data_for_draw_call
{
	vk::DeviceSize mPositionsBufferOffset;
	vk::DeviceSize mTexCoordsBufferOffset;
	vk::DeviceSize mNormalsBufferOffset;
	vk::DeviceSize mIndexBufferOffset;
    uint32_t mNumElements;

	glm::mat4 mModelMatrix;

	int32_t mMaterialIndex;
	int32_t mPixelsOnMeridian;
};

/** Contains the data for each draw call */
struct loaded_model_data
{
	std::vector<glm::vec3> mPositions;
	std::vector<glm::vec2> mTexCoords;
	std::vector<glm::vec3> mNormals;
	std::vector<uint32_t> mIndices;

	glm::mat4 mModelMatrix;

	int32_t mMaterialIndex;
	int32_t mPixelsOnMeridian;
};

struct frame_data_ubo
{
    glm::mat4            mViewMatrix;
    glm::mat4            mViewProjMatrix;
    glm::mat4            mInverseProjMatrix;
    // xy = resoolution, z = bfc on/off
    glm::uvec4           mResolutionAndCulling;
    // Debug sliders:
    glm::vec4            mDebugSliders;
    glm::ivec4           mDebugSlidersi;
    // Common, global settings:
    VkBool32             mHeatMapEnabled;
    VkBool32             mGatherPipelineStats;
    VkBool32             mUseMaxPatchResolutionDuringPxFill;
    VkBool32             mWriteToCombinedAttachmentInFragmentShader;
    float                mAbsoluteTime;
    float                mDeltaTime;
    float               _padding[2];
};

// Defines the type of parametric object to draw:
enum struct parametric_object_type : int32_t
{
    Plane = 0,
    Sphere,
    PalmTreeTrunk,
    Water,
    Terrain,
    SHBasisFunction,
    FunkyPlane,
    ExtraFunkyPlane,
    JuliasParametricHeart,
    JohisHeart,
    Spherehog,
    SpikyHeart,
    SHGlyph,
    MichiBall, // 13
    YarnCurve, // 14
    YarnCurveAnimated,
    FiberCurve,
    FiberCurveAnimated,
    Seashell1, // 18
    Seashell2,
    Seashell3
};

enum struct rendering_method : int
{
    point_rendered = 0,
    tessellated_rasterized,
    tessellated_rasterized_wireframe,
    hybrid,
    vertices_rasterized
};

// ATTENTION: Whenever you add a new enum item  ^^^  here, also add it to the string  vvv  here!
static const char* PARAMETRIC_OBJECT_TYPE_UI_STRING
	= "Plane\0Sphere\0PalmTreeTrunk\0Water\0Terrain\0SHBasisFunction (change via debug sliders (int))\0FunkyPlane\0ExtraFunkyPlane\0JuliasParametricHeart\0JohisHeart\0Spherehog\0SpikyHeart\0SH Glyph\0Michi Ball\0Yarn Curve\0Yarn Curve (animated)\0Fiber Curve\0Fiber Curve (animated)\0Seashell 1\0Seashell 2\0Seashell 3\0";

// Data about one parametric object:
class parametric_object
{
public:
    parametric_object(const char* aName, const char* aPreviewImagePath, bool isEnabled, parametric_object_type objType, float uFrom, float uTo, float vFrom, float vTo, glm::mat4 aTransformationMatrix = glm::mat4{ 1.0f }, int32_t aMaterialIndex = -1)
        : mName{ aName }
        , mPreviewImagePath{ aPreviewImagePath }
        , mEnabled{ isEnabled }
        , mModifying{ false }
        , mParams{uFrom, uTo, vFrom, vTo}
        , mEvalDims{1, 1, 0, 0}
        , mParamObjType { objType }
        , mTransformationMatrix{ aTransformationMatrix }
        , mMaterialIndex{ aMaterialIndex }
        , mRenderingMethod{ rendering_method::tessellated_rasterized }
        , mSuperSampled{ false }
    { }

    const char* name() const { return mName; }
    const char* preview_image_path() const { return mPreviewImagePath; }
    bool is_enabled() const { return mEnabled; }
    bool is_modifying() const { return mModifying; }
    glm::vec2 u_param_range() const { return glm::vec2{ mParams[0], mParams[1] }; };
    glm::vec2 v_param_range() const { return glm::vec2{ mParams[2], mParams[3] }; };
    glm::vec4 uv_param_ranges() const { return mParams; }
    glm::uvec4 eval_dims() const { return mEvalDims; }
    glm::mat4 transformation_matrix() const { return mTransformationMatrix; }
    auto      param_obj_type() const { return mParamObjType; }
    int32_t   curve_index() const { return static_cast<std::underlying_type_t<parametric_object_type>>(mParamObjType); }
    int32_t   material_index() const { return mMaterialIndex; }
    auto      how_to_render() const { return mRenderingMethod; }
    bool      super_sampling_on() const { return mSuperSampled; }

    void set_enabled(bool yesOrNo) { mEnabled = yesOrNo; }
    void set_modifying(bool yesOrNo) { mModifying = yesOrNo; }
    void set_u_param_range(glm::vec2 uFromTo) { mParams[0] = uFromTo[0]; mParams[1] = uFromTo[1]; };
    void set_v_param_range(glm::vec2 vFromTo) { mParams[2] = vFromTo[0]; mParams[3] = vFromTo[1]; };
    void set_eval_dims(glm::uvec4 ed) { mEvalDims = ed; }
    void set_transformation_matrix(const glm::mat4& aTransformationMatrix) { mTransformationMatrix = aTransformationMatrix; }
    void set_curve(parametric_object_type objType) { mParamObjType = objType; }
    void set_curve_index(int32_t curveIndex) { mParamObjType = static_cast<parametric_object_type>(curveIndex); }
    void set_material_index(int32_t matIndex) { mMaterialIndex = matIndex; }
    void set_how_to_render(rendering_method renderMeth) { mRenderingMethod = renderMeth; }
    void set_super_sampling(bool onOrOff) { mSuperSampled = onOrOff; }

private:
    const char* mName;
    const char* mPreviewImagePath;
    bool mEnabled;
    bool mModifying;
    glm::vec4 mParams; // uFrom -> uTo, vFrom -> vTo
    glm::uvec4 mEvalDims;
    glm::mat4 mTransformationMatrix;
    parametric_object_type mParamObjType;
    int32_t mMaterialIndex;
    rendering_method mRenderingMethod;
    bool mSuperSampled;
};

// +------------------------------------------------------------------------------+
// | HOST SIDE:                                                                   |
// |   Corresponding types which are also used in GLSL shaders:                   |
// +------------------------------------------------------------------------------+

struct object_data
{
    object_data()
        : mParams{0.f}
        , mDetailEvalDims{1u}
        , mTransformationMatrix{1.f}
        , mCurveIndex{0}
        , mMaterialIndex{0}
        , mUseAdaptiveDetail{1}
        , _padding{ 0xDEAD }
        , mLodAndRenderSettings{ 1.0f, 1.0f, 100.0f, 1.0f }
    {}

    glm::vec4  mParams;
    glm::uvec4 mDetailEvalDims;
    glm::mat4  mTransformationMatrix;
    int32_t    mCurveIndex;
    int32_t    mMaterialIndex;
    // The following means "adaptive tessellation levels" (for tessellation-based rendering) or "adaptive sampling" (for point-based rendering)
    int32_t    mUseAdaptiveDetail;
    int32_t    _padding;
    // The following settings are stored in the vec4:
    //  .xy ... Percent how much to increase patch parameters (s.t. neighboring patches overlap a bit)
    //  .z  ... screen-space distance for the LOD stage
    //  .w  ... sampling factor for point-based rendering (< 1.0 probably means too little samples, > 1.0 might mean oversampling but also filling holes) 
    glm::vec4  mLodAndRenderSettings;
};

struct px_fill_data
{
    // [0..1]: u from->to
    // [2..3]: v from->to
    glm::vec4 mParams;

    // [0]: object id
    // [1]: Used for row offset, but only with knit yarn
    // [2]: unused
    // [3]: unused
    glm::uvec4 mObjectIdUserData;

    // [0]: EITHER max screen dist u
    // [1]: EITHER max screen dist v
    // [2]: min screen coords of patch u (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    // [3]: min screen coords of patch v (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    glm::vec4 mScreenDists;

#if WRITE_MAX_COORDS_IN_PASS2
    // [0]: min screen coords of patch u (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    // [1]: min screen coords of patch v (if WRITE_OUT_MIN_MAX_SCREEN_COORDS is enabled in pass2x_patch_lod.comp)
    glm::vec4 mScreenMax;
#endif
};

inline void getParams(const glm::vec4& data, float& uFrom, float& uTo, float& vFrom, float& vTo) 
{
    uFrom = data[0];
    uTo   = data[1];
    vFrom = data[2];
    vTo   = data[3];
}

inline glm::vec4 setParams(float uFrom, float uTo, float vFrom, float vTo) 
{
    return glm::vec4{uFrom, uTo, vFrom, vTo};
}

inline uint32_t getObjectId(const glm::uvec4& aObjectId_Stats) 
{
    return aObjectId_Stats[0];
}

struct vertex_buffers_offsets_sizes
{
    uint32_t mPositionsOffset;
    uint32_t mPositionsSize;

    uint32_t mNormalsOffset;
    uint32_t mNormalsSize;

    uint32_t mTexCoordsOffset;
    uint32_t mTexCoordsSize;

    uint32_t mIndicesOffset;
    uint32_t mIndicesSize;
};

// +------------------------------------------------------------------------------+
// | PUSH CONSTANTS:                                                              |
// |   Corresponding types which are also used in GLSL shaders:                   |
// +------------------------------------------------------------------------------+

struct vertex_pipe_push_constants
{
	glm::mat4 mModelMatrix;
    int32_t   mMatIndex;
};

struct copy_to_backbuffer_push_constants
{
	int32_t  mWhatToCopy; // 0 ... rendering, 1 ... heat map
};

// Push constants for all the pass2x "ping pong" patch LOD dispatch calls:
struct pass2x_push_constants
{
    vk::Bool32 mGatherPipelineStats;
    uint32_t   mPatchLodLevel; // corresponding to the n-th dispatch invocation
    vk::Bool32 mPerformFrustumCulling;
};

// Push constants for all the pass2x "ping pong" patch LOD dispatch calls:
struct pass3_push_constants
{
    vk::Bool32 mGatherPipelineStats;
    int32_t    _padding;
	glm::vec2  mPatchTargetResolutionScaler; // Scaler to increase/decrease point density
	glm::vec2  mParamShift;   // Shift u/v params by amount of deltaU/deltaV (in order to fill holes to neighboring tiles)
	glm::vec2  mParamStretch; // Stretch u/v params by amount of deltaU/deltaV (in order to fill holes to neighboring tiles)
};

// Push constants for standalone tessellation shaders
struct standalone_tess_push_constants
{
    uint32_t mObjectId;
};

// Push constants for patch->tess method
struct patch_into_tess_push_constants
{
    float mConstOuterSubdivLevel;
    float mConstInnerSubdivLevel;
};
