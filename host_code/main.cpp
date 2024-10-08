#include <bit>
#include <filesystem>
#include <chrono>
#include "imgui.h"
#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "material_image_helpers.hpp"
#include "meshlet_helpers.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "sequential_invoker.hpp"
#include "orbit_camera.hpp"
#include "quake_camera.hpp"
#include "vk_convenience_functions.hpp"
#include "types.hpp"
#include "helpers.hpp"
#include "../shader_includes/host_device_shared.h"
#include <random>
#include "ImGuizmo.h"
#include "big_dataset.hpp"

#define NUM_PREDEFINED_MATERIALS 5
#define NUM_TIMESTAMP_QUERIES 14
#define NUM_DIFFERENT_RENDER_VARIANTS 5

// Sample count possible values:
// vk::SampleCountFlagBits::e1
// vk::SampleCountFlagBits::e2
// vk::SampleCountFlagBits::e4
// vk::SampleCountFlagBits::e8
// vk::SampleCountFlagBits::e16
// vk::SampleCountFlagBits::e32
// vk::SampleCountFlagBits::e64
#define SAMPLE_COUNT vk::SampleCountFlagBits::e8

#define SSAA_ENABLED 1
#define SSAA_FACTOR  glm::uvec2(2, 2)

// TEST_MODE: Uncomment one of the following includes to load a specific configuration for test mode.
//            It will then be tested with the rendering variant assigned to TEST_RENDERING_METHOD.
//            Once the application has started, start the test with [Space].
//#include "perf_tests/test_knit_yarn.hpp"
//#include "perf_tests/test_fiber_curves.hpp"
//#include "perf_tests/test_seashell.hpp"
//#define TEST_RENDERING_METHOD          rendering_variant::PointRendered_direct
#define TEST_GATHER_TIMER_QUERIES 0
#define TEST_GATHER_PATCH_COUNTS  0

static std::array<parametric_object, 14> PredefinedParametricObjects {{
	parametric_object{"Sphere"       , "assets/po-sphere-patches.png",     true , parametric_object_type::Sphere,                 0.0f, glm::pi<float>(),  0.0f,  glm::two_pi<float>() , glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{ 0.f,  0.f,  0.f})},
	parametric_object{"Johi's Heart" , "assets/po-johis-heart.png",        false, parametric_object_type::JohisHeart,             0.0f, glm::pi<float>(),  0.0f,  glm::two_pi<float>(), glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{ 0.f,  0.f, -2.f})},
	parametric_object{"Spiky Heart"  , "assets/po-spiky-heart.png",        false, parametric_object_type::SpikyHeart,             0.0f, glm::pi<float>(),  0.0f,  glm::two_pi<float>(), glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{ 0.f,  0.f,  2.f}), -5},
	parametric_object{"Seashell 1"   , "assets/po-seashell1.png",          false, parametric_object_type::Seashell1,              glm::two_pi<float>() * 8.0f,/* -> */0.0f,   0.0f,/* -> */glm::two_pi<float>(), glm::uvec2{ 1u, 1u }, glm::mat4{ 1.0f }, -3},
	parametric_object{"Seashell 2"   , "assets/po-seashell2.png",          false, parametric_object_type::Seashell2,              glm::two_pi<float>() * 8.0f,/* -> */0.0f,   0.0f,/* -> */glm::two_pi<float>(), glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{-4.5f, 0.0f, 0.0f }), -4},
	parametric_object{"Seashell 3"   , "assets/po-seashell3.png",          false, parametric_object_type::Seashell3,              glm::two_pi<float>() * 8.0f,/* -> */0.0f,   0.0f,/* -> */glm::two_pi<float>(), glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{ 4.5f, 0.0f, 0.0f }), -5},
	parametric_object{"Yarn Curve"   , "assets/po-yarn-curve-single.png",  false, parametric_object_type::SingleYarnCurve,        1.0f, 1.0f,     /* <-- yarn dimensions | n/a yarn -> */ 0.f, /* thickness --> */ 0.8f, glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{-0.3f, 0.0f, 0.0f}) * glm::scale(glm::vec3{ 0.3f }), -5},
	parametric_object{"Fiber Curve"  , "assets/po-fiber-curve-single.png", false, parametric_object_type::SingleFiberCurve,       1.0f, 1.0f,     /* <-- yarn dimensions | #fibers --> */ 6.f, /* thickness --> */ 0.3f, glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{-0.5f, 0.0f, 0.0f}) * glm::scale(glm::vec3{ 0.3f }), -5},
	parametric_object{"Yarn Curtain" , "assets/po-blue-curtain.png",       false, parametric_object_type::CurtainYarnCurves,      235.0f, 254.0f, /* <-- yarn dimensions | #fibers --> */ 6.f, /* thickness --> */ 0.8f, glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{-3.35f, 0.08f, 5.32f}) * glm::scale(glm::vec3{ 0.005f }), 19},
	parametric_object{"Fiber Curtain", "assets/po-blue-curtain.png",       false, parametric_object_type::CurtainFiberCurves,     235.0f, 254.0f, /* <-- yarn dimensions | #fibers --> */ 6.f, /* thickness --> */ 0.8f, glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{-3.35f, 0.08f, 5.32f}) * glm::scale(glm::vec3{ 0.005f }), 19},
	parametric_object{"Palm Tree"    , "assets/po-palm-tree.png",          false, parametric_object_type::PalmTreeTrunk,          0.0f,   1.0f,            0.0f,  glm::two_pi<float>(), glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{ 0.f,  0.f, -4.f})},
	parametric_object{"Giant Worm"   , "assets/po-giant-worm.png",         false, parametric_object_type::GiantWorm,              0.0f,   1.0f,            0.0f,  glm::two_pi<float>(), glm::uvec2{ 1u, 1u }, glm::translate(glm::vec3{ 0.f,  0.f, -4.f}), -5},
	parametric_object{"SH Glyph"     , "assets/po-single-sh-glyph.png",    false, parametric_object_type::SHGlyph,                0.0f, glm::pi<float>(),  0.0f,  glm::two_pi<float>(),     glm::uvec2{ 1u, 1u }, glm::mat4{ 1.0f }, -2},
	parametric_object{"Brain Scan"   , "assets/po-sh-brain.png",           false, parametric_object_type::SHBrain,                0.0f, glm::pi<float>(),  0.0f,  glm::two_pi<float>(), glm::uvec2{ SH_BRAIN_DATA_SIZE_X, SH_BRAIN_DATA_SIZE_Y }, glm::mat4{ 1.0f }, -2}
}};

class vk_parametric_curves_app : public avk::invokee
{

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	vk_parametric_curves_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mEnableSampleShadingFor8xMS{ false }
		, mEnableSampleShadingFor4xSS8xMS{ false }
		, mRenderVariantDataTransferEnabled { { true, true, true } }
		, mOutputResultOfRenderVariantIndex{ 3 }
	{
        for (const auto& po : PredefinedParametricObjects) {
            mParametricObjects.push_back(po);
        }
	}

	/** Creates buffers for all the drawcalls.
	 *  Called after everything has been loaded and split into meshlets properly.
	 *  @param dataForDrawCall		The loaded data for the drawcalls.
	 *	@param drawCallsTarget		The target vector for the draw call data.
	 */
	void add_draw_calls(std::vector<loaded_model_data>& dataForDrawCall) {
		using namespace avk;

		for (auto& drawCallData : dataForDrawCall) {
			auto& drawCall = mDrawCalls.emplace_back();
			drawCall.mModelMatrix = drawCallData.mModelMatrix;
			drawCall.mMaterialIndex = drawCallData.mMaterialIndex;
			drawCall.mPixelsOnMeridian = drawCallData.mPixelsOnMeridian;

			const auto insertIdx = mVertexBuffersOffsetsSizesCount.x++;
			uint32_t posOffset = 0;
			uint32_t nrmOffset = 0;
			uint32_t tcoOffset = 0;
			uint32_t idxOffset = 0;
            if (0 != insertIdx) { // There are already elements => update offsets:
				const auto& b = mVertexBuffersOffsetsSizes.back();
				posOffset = roundUpToMultipleOf(b.mPositionsOffset + b.mPositionsSize, 16u);
				nrmOffset = roundUpToMultipleOf(b.mNormalsOffset   + b.mNormalsSize  , 16u);
				tcoOffset = roundUpToMultipleOf(b.mTexCoordsOffset + b.mTexCoordsSize, 16u);
				idxOffset = roundUpToMultipleOf(b.mIndicesOffset   + b.mIndicesSize  , 16u);
            }

			const auto& os = mVertexBuffersOffsetsSizes.emplace_back(
				posOffset, drawCallData.mPositions.size() * sizeof(drawCallData.mPositions[0]),
				nrmOffset, drawCallData.mNormals.size()   * sizeof(drawCallData.mNormals[0]),
				tcoOffset, drawCallData.mTexCoords.size() * sizeof(drawCallData.mTexCoords[0]),
				idxOffset, drawCallData.mIndices.size()   * sizeof(drawCallData.mIndices[0])
			);

			context().record_and_submit_with_fence({
			    mPositionsBuffer->fill(drawCallData.mPositions.data(), 0, os.mPositionsOffset, os.mPositionsSize),
			    mNormalsBuffer  ->fill(drawCallData.mNormals.data()  , 0, os.mNormalsOffset  , os.mNormalsSize),
			    mTexCoordsBuffer->fill(drawCallData.mTexCoords.data(), 0, os.mTexCoordsOffset, os.mTexCoordsSize),
			    mIndexBuffer    ->fill(drawCallData.mIndices.data()  , 0, os.mIndicesOffset  , os.mIndicesSize)
			}, *mQueue)->wait_until_signalled();

			drawCall.mPositionsBufferOffset = posOffset;
	        drawCall.mTexCoordsBufferOffset = tcoOffset;
	        drawCall.mNormalsBufferOffset   = nrmOffset;
	        drawCall.mIndexBufferOffset     = idxOffset;
            drawCall.mNumElements           = os.mIndicesSize / sizeof(drawCallData.mIndices[0]); // => to get the number of vertices to draw for draw_indexed
		}
	}

	void add_custom_materials(std::vector<avk::material_config>& aMaterials)
	{
		using namespace avk;

		auto& checkerboardMat = aMaterials.emplace_back();
		checkerboardMat.mDiffuseReflectivity = glm::vec4(1.0f);
		checkerboardMat.mAmbientReflectivity = glm::vec4(1.0f);
		checkerboardMat.mAlbedo = glm::vec4(1.0f);
		checkerboardMat.mDiffuseTex = "assets/checkerboard-pattern.jpg";
		checkerboardMat.mDiffuseTexBorderHandlingMode = {{ border_handling_mode::repeat, border_handling_mode::repeat }};
		
		auto& cobblestonesMat = aMaterials.emplace_back();
		cobblestonesMat.mDiffuseReflectivity = glm::vec4(1.0f);
		cobblestonesMat.mAmbientReflectivity = glm::vec4(1.0f);
		cobblestonesMat.mAlbedo = glm::vec4(1.0f);
		cobblestonesMat.mDiffuseTex = "assets/brick_floor_tileable_Base_Color.jpg";
		cobblestonesMat.mAmbientTex = "assets/brick_floor_tileable_Ambient_Occlusion.jpg";
		cobblestonesMat.mNormalsTex  = "assets/brick_floor_tileable_Normal.jpg";
		cobblestonesMat.mDiffuseTexBorderHandlingMode = {{ border_handling_mode::repeat, border_handling_mode::repeat }};
		cobblestonesMat.mAmbientTexBorderHandlingMode = {{ border_handling_mode::repeat, border_handling_mode::repeat }};
		cobblestonesMat.mHeightTexBorderHandlingMode  = {{ border_handling_mode::repeat, border_handling_mode::repeat }};
	}

	void create_buffers()
	{
		using namespace avk;

		// First of all, create all them buffers. We'll have just ONE single per attribute, which contains all the models' data.
		mPositionsBuffer = context().create_buffer(memory_usage::device, {},
			vertex_buffer_meta ::create_from_element_size(sizeof(glm::vec3), MAX_VERTICES).describe_member(0, format_for<glm::vec3>(), content_description::position),
			storage_buffer_meta::create_from_size(sizeof(glm::vec3) * MAX_VERTICES)
		);
		mNormalsBuffer = context().create_buffer(memory_usage::device, {},
			vertex_buffer_meta ::create_from_element_size(sizeof(glm::vec3), MAX_VERTICES).describe_member(0, format_for<glm::vec3>(), content_description::normal),
			storage_buffer_meta::create_from_size(sizeof(glm::vec3) * MAX_VERTICES)
		);
		mTexCoordsBuffer = context().create_buffer(memory_usage::device, {},
			vertex_buffer_meta ::create_from_element_size(sizeof(glm::vec2), MAX_VERTICES).describe_member(0, format_for<glm::vec2>(), content_description::texture_coordinate),
			storage_buffer_meta::create_from_size(sizeof(glm::vec2) * MAX_VERTICES)
		);
		mIndexBuffer = context().create_buffer(
			memory_usage::device, {},
			index_buffer_meta::create_from_element_size(sizeof(uint32_t), MAX_INDICES),
			storage_buffer_meta::create_from_size(sizeof(uint32_t) * MAX_INDICES)
		);
		mVertexBuffersOffsetsSizesBuffer = context().create_buffer(
			memory_usage::device, {},
			storage_buffer_meta::create_from_size(sizeof(vertex_buffers_offsets_sizes) * MAX_OBJECTS) // <-- offsets and sizes (positions, normals, texcoords, indices) for MAX_OBJECTS-many objects
		);
		mVertexBuffersOffsetsSizesCountBuffer = context().create_buffer(
			memory_usage::device, {}, 
			indirect_buffer_meta::create_from_num_elements(1, sizeof(glm::uvec4)),
			storage_buffer_meta::create_from_size(            sizeof(glm::uvec4)) // 1 counter, that's it
		);

		std::vector<avk::material_config> initialMaterials;
		add_custom_materials(initialMaterials);

		// For all the different materials, transfer them in structs which are well
		// suited for GPU-usage (proper alignment, and containing only the relevant data),
		// also load all the referenced images from file and provide access to them
		// via samplers; It all happens in `ak::convert_for_gpu_usage`:
		auto [gpuMaterials, imageSamplers, matCommands] = convert_for_gpu_usage<material_gpu_data>(
			initialMaterials, false, true,
			image_usage::general_texture,
			filter_mode::trilinear
		);

		mMaterialBuffer = context().create_buffer(
			memory_usage::device, {},
			storage_buffer_meta::create_from_data(gpuMaterials)
		);

		mImageSamplers = std::move(imageSamplers);

		context().record_and_submit_with_fence({
			mVertexBuffersOffsetsSizesCountBuffer->fill(&mVertexBuffersOffsetsSizesCountBuffer, 0),
			mVertexBuffersOffsetsSizesBuffer->fill(mVertexBuffersOffsetsSizes.data(), 0, 0, mVertexBuffersOffsetsSizes.size() * sizeof(mVertexBuffersOffsetsSizes[0])),
			matCommands,
		    mMaterialBuffer->fill(gpuMaterials.data(), 0)
		}, *mQueue)->wait_until_signalled();

		// One for each concurrent frame
		const auto concurrentFrames = context().main_window()->number_of_frames_in_flight();
		for (int i = 0; i < concurrentFrames; ++i) {
			mFrameDataBuffers.push_back(context().create_buffer(
				memory_usage::host_coherent, {},
				uniform_buffer_meta::create_from_data(frame_data_ubo{})
			));
		}
	}

	void load_sponza_and_terrain()
	{
		using namespace avk;

		if (!mDrawCalls.empty()) {
			LOG_WARNING("Sponza and Terrain already loaded");
			return;
		}

		std::vector<material_config> allMatConfigs; // <-- Gather all the materials to be used
		// But don't forget the standard materials defined in code:
		add_custom_materials(allMatConfigs);

		std::vector<loaded_model_data> dataForDrawCall;

		if (std::filesystem::exists("assets/sponza_and_terrain.fscene")) {
			glm::mat4 sceneTransform = glm::mat4(1.0f);
			sceneTransform = glm::translate(glm::vec3(2.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3{ 2.667f });

			auto orca = orca_scene_t::load_from_file("assets/sponza_and_terrain.fscene");
			// Get all the different materials from the whole scene:
		    auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();
			for (const auto& pair : distinctMaterialsOrca) {
			    allMatConfigs.push_back(pair.first);
			    const int matIndex = static_cast<int>(allMatConfigs.size()) - 1;

			    // The data in distinctMaterialsOrca encompasses all of the ORCA scene's models.
			    for (const auto& modelAndMeshIndices : pair.second) {
				    
				    auto& modelData = orca->model_at_index(modelAndMeshIndices.mModelIndex);
					auto& curModel = modelData.mLoadedModel;
					for (const auto& orcaInst : modelData.mInstances) {
						auto orcaInstTransform = avk::matrix_from_transforms(orcaInst.mTranslation, glm::quat(orcaInst.mRotation), orcaInst.mScaling);
						for (size_t mpos = 0; mpos < modelAndMeshIndices.mMeshIndices.size(); mpos++) {
					        auto meshIndex = modelAndMeshIndices.mMeshIndices[mpos];
					        std::string meshname = curModel->name_of_mesh(meshIndex);
							LOG_INFO(std::format("mesh[{}], mpos[{}], indices.size[{}]", meshname, mpos, modelAndMeshIndices.mMeshIndices.size()));
							
					        auto& drawCallData = dataForDrawCall.emplace_back();

					        drawCallData.mMaterialIndex = static_cast<int32_t>(matIndex);
					        drawCallData.mPixelsOnMeridian = 1;
					        drawCallData.mModelMatrix = sceneTransform * orcaInstTransform * curModel->transformation_matrix_for_mesh(meshIndex);

					        auto selection = make_model_references_and_mesh_indices_selection(curModel, meshIndex);
					        std::tie(drawCallData.mPositions, drawCallData.mIndices) = get_vertices_and_indices(selection);
								
							// Exclude one blue curtain (of a total of three) by modifying the loaded indices before uploading them to a GPU buffer:
							if (meshname == "sponza_326") {
								drawCallData.mIndices.erase(std::begin(drawCallData.mIndices), std::begin(drawCallData.mIndices) + 3 * 4864);
							}

					        drawCallData.mNormals = get_normals(selection);
					        drawCallData.mTexCoords = get_2d_texture_coordinates(selection, 0);
				        }
					}
			    }
		    }
		}

		// Update all the buffers for our drawcall data:
		add_draw_calls(dataForDrawCall);

		mNumMaterials = static_cast<int>(allMatConfigs.size());
		LOG_INFO_EM(std::format("Number of materials = {}", mNumMaterials));

		// For all the different materials, transfer them in structs which are well
		// suited for GPU-usage (proper alignment, and containing only the relevant data),
		// also load all the referenced images from file and provide access to them
		// via samplers; It all happens in `ak::convert_for_gpu_usage`:
		auto [gpuMaterials, imageSamplers, matCommands] = convert_for_gpu_usage<material_gpu_data>(
			allMatConfigs, false, true,
			image_usage::general_texture,
			filter_mode::trilinear
		);

		// Lifetime-handle existing material buffer, then overwrite with new one:
		context().main_window()->handle_lifetime(std::move(mMaterialBuffer));
		mMaterialBuffer = context().create_buffer(
			memory_usage::device, {},
			storage_buffer_meta::create_from_data(gpuMaterials)
		);

		// Lifetime-handle possibly existing image samplers, then overwrite with new ones:
		for (auto& existingImageSampler : mImageSamplers) {
			context().main_window()->handle_lifetime(std::move(existingImageSampler));
		}
		mImageSamplers.clear();
		mImageSamplers = std::move(imageSamplers);

		context().record_and_submit_with_fence({
			mVertexBuffersOffsetsSizesCountBuffer->fill(&mVertexBuffersOffsetsSizesCountBuffer, 0),
			mVertexBuffersOffsetsSizesBuffer->fill(mVertexBuffersOffsetsSizes.data(), 0, 0, mVertexBuffersOffsetsSizes.size() * sizeof(mVertexBuffersOffsetsSizes[0])),
			matCommands,
		    mMaterialBuffer->fill(gpuMaterials.data(), 0)
		}, *mQueue)->wait_until_signalled();
    }

	void load_sh_brain_dataset()
	{
		using namespace avk;

		auto datasetData = get_big_sh_dataset(SH_BRAIN_DATA_SIZE_X, SH_BRAIN_DATA_SIZE_Y);
		LOG_WARNING_EM(std::format("Loaded big dataset with {} elements ({}x{})", datasetData.size(), SH_BRAIN_DATA_SIZE_X, SH_BRAIN_DATA_SIZE_Y));
		assert(datasetData.size() == SH_BRAIN_DATA_SIZE_X * SH_BRAIN_DATA_SIZE_Y);
		mDatasetSize = datasetData.size();
		mBigDataset = context().create_buffer(
			memory_usage::device, {},
			storage_buffer_meta::create_from_data(datasetData)
		);
		context().record_and_submit_with_fence({
			mBigDataset->fill(datasetData.data(), 0)
		}, *mQueue)->wait_until_signalled();
	}

	/* // PC ... push constants type */
	// Ts ... optional addtional bindings
	template </*typename PC,*/ typename... Ts>
	auto create_compute_pipe_for_parametric(auto computeShader, Ts... args)
    {
		LOG_INFO_EM(std::format("Creating compute pipe for shader '{}'...", computeShader));

        using namespace avk;
        return context().create_compute_pipeline_for(
                compute_shader(computeShader),
                //push_constant_binding_data{shader_type::all, 0, sizeof(PC)},
				descriptor_binding(0, 0, mFrameDataBuffers[0]), 
			    // Buffer for some pipeline statistics:
			    descriptor_binding(1, 0, mCountersSsbo->as_storage_buffer()),
				// The buffers containing object and px fill data:
				descriptor_binding(2, 0, mObjectDataBuffer->as_storage_buffer()),
				descriptor_binding(2, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
				descriptor_binding(2, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
				// All possible other bindings:
				std::move(args)...
		);
	}

    void create_param_pipes()
    {
        using namespace avk;

		mInitPatchesComputePipe = create_compute_pipe_for_parametric(
			"shaders/pass1_init_patches.comp",
			descriptor_binding(3, 0, mPatchLodBufferPing->as_storage_buffer()), // Attention: Only ping will be used in pass 1
			descriptor_binding(3, 1, mPatchLodBufferPong->as_storage_buffer()), //            We don't need pong here.
			descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer())
		);
        mUpdater->on(shader_files_changed_event(mInitPatchesComputePipe.as_reference())).update(mInitPatchesComputePipe);

		mInitKnitYarnComputePipe = create_compute_pipe_for_parametric(
			"shaders/pass1_init_knityarn.comp",
			descriptor_binding(3, 0, mPatchLodBufferPing->as_storage_buffer()), // Attention: Only ping will be used in pass 1
			descriptor_binding(3, 1, mPatchLodBufferPong->as_storage_buffer()), //            We don't need pong here.
			descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer()),
			push_constant_binding_data{ shader_type::all, 0, sizeof(uint32_t) }
		);
        mUpdater->on(shader_files_changed_event(mInitKnitYarnComputePipe.as_reference())).update(mInitKnitYarnComputePipe);

		mInitShBrainComputePipe = create_compute_pipe_for_parametric(
			"shaders/pass1_init_shbrain.comp",
			descriptor_binding(3, 0, mPatchLodBufferPing->as_storage_buffer()), // Attention: Only ping will be used in pass 1
			descriptor_binding(3, 1, mPatchLodBufferPong->as_storage_buffer()), //            We don't need pong here.
			descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer()),
			push_constant_binding_data{ shader_type::all, 0, sizeof(uint32_t) }
		);
        mUpdater->on(shader_files_changed_event(mInitShBrainComputePipe.as_reference())).update(mInitShBrainComputePipe);

		mPatchLodComputePipe = create_compute_pipe_for_parametric(
			"shaders/pass2x_patch_lod.comp",
			// Ping-pong buffers:
			descriptor_binding(3, 0, mPatchLodBufferPing->as_storage_buffer()), // Attention: These...
			descriptor_binding(3, 1, mPatchLodBufferPong->as_storage_buffer()), //            ...two will be swapped for consecutive dispatch calls at pass2x-level.
			descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer()),
			descriptor_binding(4, 0, mBigDataset->as_storage_buffer()),
#if DRAW_PATCH_EVAL_DEBUG_VIS
            descriptor_binding(5, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
            descriptor_binding(5, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
#endif
			push_constant_binding_data{ shader_type::all, 0, sizeof(pass2x_push_constants) }
		);
        mUpdater->on(shader_files_changed_event(mPatchLodComputePipe.as_reference())).update(mPatchLodComputePipe);

		mPxFillComputePipe = create_compute_pipe_for_parametric(
			"shaders/pass3_px_fill.comp", 
			descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			descriptor_binding(0, 2, mMaterialBuffer), // TODO: Optimized descriptor set assignment would be cool
            // super druper image & heatmap image:
            descriptor_binding(3, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
            descriptor_binding(3, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
			descriptor_binding(4, 0, mBigDataset->as_storage_buffer()),
			push_constant_binding_data{ shader_type::all, 0, sizeof(pass3_push_constants) }
		);
        mUpdater->on(shader_files_changed_event(mPxFillComputePipe.as_reference())).update(mPxFillComputePipe);

		mPxFillLocalFbComputePipe = create_compute_pipe_for_parametric(
			"shaders/pass3_px_fill_local_fb.comp",
			descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			descriptor_binding(0, 2, mMaterialBuffer), // TODO: Optimized descriptor set assignment would be cool
            // super druper image & heatmap image:
            descriptor_binding(3, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
            descriptor_binding(3, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
			descriptor_binding(4, 0, mBigDataset->as_storage_buffer()),
#if SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
			descriptor_binding(5, 0, mTilePatchesBuffer->as_storage_buffer()),
#endif
			push_constant_binding_data{ shader_type::all, 0, sizeof(pass3_push_constants) }
		);
        mUpdater->on(shader_files_changed_event(mPxFillLocalFbComputePipe.as_reference())).update(mPxFillLocalFbComputePipe);

#if SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
		mSelectTilePatchesPipe = create_compute_pipe_for_parametric(
			"shaders/pass3pre_select_tile_patches.comp",
			descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			descriptor_binding(0, 2, mMaterialBuffer),
            descriptor_binding(3, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
            descriptor_binding(3, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
			descriptor_binding(4, 0, mTilePatchesBuffer->as_storage_buffer())
		);
        mUpdater->on(shader_files_changed_event(mSelectTilePatchesPipe.as_reference())).update(mSelectTilePatchesPipe);
#endif
    }

	avk::graphics_pipeline create_vertex_pipe()
	{
		using namespace avk;

		return context().create_graphics_pipeline_for(
                vertex_shader("shaders/simple.vert"),
				fragment_shader("shaders/frag_out.frag"),
			    // The next 3 lines define the format and location of the vertex shader inputs:
			    // (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			    from_buffer_binding(0) -> stream_per_vertex<glm::vec3>() -> to_location(0), // <-- corresponds to vertex shader's inPosition
			    from_buffer_binding(1) -> stream_per_vertex<glm::vec2>() -> to_location(1), // <-- corresponds to vertex shader's inTexCoord
			    from_buffer_binding(2) -> stream_per_vertex<glm::vec3>() -> to_location(2), // <-- corresponds to vertex shader's inNormal
                cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			    mBackfaceCullingOn ? cfg::culling_mode::cull_back_faces : cfg::culling_mode::disabled,

				cfg::viewport_depth_scissors_config::from_framebuffer(mFramebufferNoAA.as_reference()),
				mRenderpassNoAA, cfg::subpass_index{ 0 },
				cfg::shade_per_fragment(), 
			
				mDisableColorAttachmentOut 
					? cfg::color_blending_config { {}, true , cfg::color_channel::none }
					: cfg::color_blending_config { {}, false, cfg::color_channel::rgba }
				,
			    push_constant_binding_data{shader_type::all, 0, sizeof(vertex_pipe_push_constants)},
				descriptor_binding(0, 0, mFrameDataBuffers[0]),
			    descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			    descriptor_binding(0, 2, mMaterialBuffer), // TODO: Optimized descriptor set assignment would be cool
                // super druper image
                descriptor_binding(1, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
                descriptor_binding(1, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
                // Buffer for some pipeline statistics:
			    descriptor_binding(2, 0, mCountersSsbo->as_storage_buffer())
		);
	}

	avk::graphics_pipeline create_noaa_to_ms_pipe()
	{
		using namespace avk;

		return context().create_graphics_pipeline_for(
                vertex_shader("shaders/full_screen_quad.vert"),
				fragment_shader("shaders/full_screen_quad_from_combined_attachment.frag"),
                cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			    cfg::culling_mode::cull_back_faces,

				cfg::viewport_depth_scissors_config::from_framebuffer(mFramebufferMS.as_reference()),
				mRenderpassMS, cfg::subpass_index{ 0u }, 
				cfg::shade_per_fragment(),
						
				push_constant_binding_data{ shader_type::all, 0, sizeof(copy_to_backbuffer_push_constants) },
				descriptor_binding(0, 0, mFsQuadColorSampler),
				descriptor_binding(0, 1, mFsQuadDepthSampler),
				descriptor_binding(1, 0, mCombinedAttachmentView->as_storage_image(layout::general))
#if STATS_ENABLED
				, descriptor_binding(1, 1, mHeatMapImageView->as_storage_image(layout::general))
#endif
		);
	}

	avk::graphics_pipeline create_ms_to_ssms_pipe()
	{
		using namespace avk;

		return context().create_graphics_pipeline_for(
                vertex_shader("shaders/full_screen_quad.vert"),
				fragment_shader("shaders/full_screen_quad.frag"),
                cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			    cfg::culling_mode::cull_back_faces,

				cfg::viewport_depth_scissors_config::from_framebuffer(mFramebufferSSMS.as_reference()),
				mRenderpassSSMS, cfg::subpass_index{ 0u }, 
				cfg::shade_per_fragment(),
			
				descriptor_binding(0, 0, mFsQuadColorSampler),
				descriptor_binding(0, 1, mFsQuadDepthSampler),
			    descriptor_binding(0, 2, mFramebufferMS->image_view_at(0)->as_sampled_image(layout::shader_read_only_optimal)),
			    descriptor_binding(0, 3, mFramebufferMS->image_view_at(1)->as_sampled_image(layout::shader_read_only_optimal))
		);
	}

	// Ts ... optional addtional bindings
	template <typename... Ts>
	avk::graphics_pipeline create_tess_pipe(std::string aVert, std::string aTesc, std::string aTese, Ts... args)
	{
		using namespace avk;

		return context().create_graphics_pipeline_for(
                vertex_shader(aVert),
                tessellation_control_shader(aTesc),
                tessellation_evaluation_shader(aTese),
				fragment_shader("shaders/frag_out.frag"),
                cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			    mBackfaceCullingOn ? cfg::culling_mode::cull_back_faces : cfg::culling_mode::disabled,

				mDisableColorAttachmentOut 
					? cfg::color_blending_config { {}, true , cfg::color_channel::none }
					: cfg::color_blending_config { {}, false, cfg::color_channel::rgba }
				,
    			cfg::primitive_topology::patches,
	    		cfg::tessellation_patch_control_points{ 4u },

				descriptor_binding(0, 0, mFrameDataBuffers[0]),
			    descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			    descriptor_binding(0, 2, mMaterialBuffer), // TODO: Optimized descriptor set assignment would be cool
                // super druper image
                descriptor_binding(1, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
                descriptor_binding(1, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
			    // Buffer for some pipeline statistics:
			    descriptor_binding(2, 0, mCountersSsbo->as_storage_buffer()),
				// The buffer containing object data:
				descriptor_binding(3, 0, mObjectDataBuffer->as_storage_buffer()),
				descriptor_binding(3, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
				descriptor_binding(3, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
				descriptor_binding(4, 0, mBigDataset->as_storage_buffer()),
			    // All possible other bindings:
				std::move(args)...
		);
	}

	void create_framebuffer_and_auxiliary_images(std::span<const vk::Format> aAttachmentFormats)
    {
        using namespace avk;
        auto resolution    = context().main_window()->resolution();
		auto colorFormatMS = std::make_tuple(aAttachmentFormats[0], SAMPLE_COUNT);
		auto depthFormatMS = std::make_tuple(aAttachmentFormats[1], SAMPLE_COUNT);
        auto  tmpColorAttachment  = context().create_image(resolution.x, resolution.y, aAttachmentFormats[0], 1, memory_usage::device,
                                                          image_usage::color_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
        auto  colorAttachment     = context().create_image(resolution.x, resolution.y, aAttachmentFormats[0], 1, memory_usage::device,
                                                          image_usage::color_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
		auto  colorAttachmentMS   = context().create_image(resolution.x, resolution.y, colorFormatMS, 1, memory_usage::device,
                                                          image_usage::color_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
        auto  tmpDepthAttachment  = context().create_image(resolution.x, resolution.y, aAttachmentFormats[1], 1, memory_usage::device,
                                                          image_usage::depth_stencil_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
        auto  depthAttachment     = context().create_image(resolution.x, resolution.y, aAttachmentFormats[1], 1, memory_usage::device,
                                                          image_usage::depth_stencil_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
        auto  depthAttachmentMS   = context().create_image(resolution.x, resolution.y, depthFormatMS, 1, memory_usage::device,
                                                          image_usage::depth_stencil_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
        auto  combinedAttachment  = context().create_image(resolution.x, resolution.y, vk::Format::eR64Uint, 1, memory_usage::device, image_usage::shader_storage);

		auto  colorAttachmentSS   = context().create_image(resolution.x * SSAA_FACTOR.x, resolution.y * SSAA_FACTOR.y, aAttachmentFormats[0], 1, memory_usage::device,
                                                          image_usage::color_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
        auto  depthAttachmentSS   = context().create_image(resolution.x * SSAA_FACTOR.x, resolution.y * SSAA_FACTOR.y, aAttachmentFormats[1], 1, memory_usage::device,
                                                          image_usage::depth_stencil_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
		auto  colorAttachmentSSMS = context().create_image(resolution.x * SSAA_FACTOR.x, resolution.y * SSAA_FACTOR.y, colorFormatMS, 1, memory_usage::device,
                                                          image_usage::color_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
        auto  depthAttachmentSSMS = context().create_image(resolution.x * SSAA_FACTOR.x, resolution.y * SSAA_FACTOR.y, depthFormatMS, 1, memory_usage::device,
                                                          image_usage::depth_stencil_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);

#if STATS_ENABLED
		// Create an image for writing a pixel heat map to:
		auto heatMapImage = context().create_image(resolution.x, resolution.y, vk::Format::eR32Uint, 1, memory_usage::device, image_usage::shader_storage);
#endif

        // Transition the image layouts before using them:
		const auto currentFrameId = context().main_window()->current_frame();
		auto sem = context().record_and_submit_with_semaphore({
			sync::image_memory_barrier(combinedAttachment.as_reference(), stage::none >> stage::none).with_layout_transition(layout::undefined >> layout::general)
#if STATS_ENABLED
			, sync::image_memory_barrier(heatMapImage.as_reference(), stage::none >> stage::none).with_layout_transition(layout::undefined >> layout::general)
#endif
		}, *mQueue, stage::transfer);
		// Ensure that the transition is done before the next frame begins to render:
        context().main_window()->add_present_dependency_for_frame(std::move(sem), currentFrameId + 1); 

        // Create views for all them images:
		auto tmpColorAttachmentView  = context().create_image_view(std::move(tmpColorAttachment));
        auto colorAttachmentView     = context().create_image_view(std::move(colorAttachment));
		auto colorAttachmentViewMS   = context().create_image_view(std::move(colorAttachmentMS));
		auto colorAttachmentViewSS   = context().create_image_view(std::move(colorAttachmentSS));
		auto colorAttachmentViewSSMS = context().create_image_view(std::move(colorAttachmentSSMS));
        auto tmpDepthAttachmentView  = context().create_image_view(std::move(tmpDepthAttachment));
        auto depthAttachmentView     = context().create_image_view(std::move(depthAttachment));
        auto depthAttachmentViewMS   = context().create_image_view(std::move(depthAttachmentMS));
        auto depthAttachmentViewSS   = context().create_image_view(std::move(depthAttachmentSS));
        auto depthAttachmentViewSSMS = context().create_image_view(std::move(depthAttachmentSSMS));
        mCombinedAttachmentView      = context().create_image_view(std::move(combinedAttachment));
#if STATS_ENABLED
		mHeatMapImageView          = context().create_image_view(std::move(heatMapImage));
#endif

        mRenderpassNoAA = context().create_renderpass({
                attachment::declare(aAttachmentFormats[0], on_load::clear.from_previous_layout(layout::undefined), usage::color(0)     , on_store::store.in_layout(layout::shader_read_only_optimal)),
                attachment::declare(aAttachmentFormats[1], on_load::clear.from_previous_layout(layout::undefined), usage::depth_stencil, on_store::store.in_layout(layout::shader_read_only_optimal)),
            }, {
				subpass_dependency{subpass::external >> subpass::index(0),
					stage::none    >>   stage::all_graphics,
					access::none   >>   access::color_attachment_write | access::depth_stencil_attachment_read | access::depth_stencil_attachment_write
				},
				subpass_dependency{subpass::index(0) >> subpass::external, 
					stage::color_attachment_output | stage::early_fragment_tests | stage::late_fragment_tests                        >>  stage::compute_shader                                     ,
					access::color_attachment_write | access::depth_stencil_attachment_read | access::depth_stencil_attachment_write  >>  access::shader_storage_read | access::shader_storage_write
				}
			});

        mFramebufferNoAA = context().create_framebuffer(mRenderpassNoAA, make_vector(
			tmpColorAttachmentView, 
			tmpDepthAttachmentView
		));

        mRenderpassMS = context().create_renderpass({
                attachment::declare(aAttachmentFormats[0], on_load::clear.from_previous_layout(layout::undefined), usage::unused        >> usage::unused                              , on_store::store
#if SSAA_ENABLED
					.in_layout(layout::shader_read_only_optimal)
#else
					.in_layout(layout::transfer_src)
#endif
				),
                attachment::declare(aAttachmentFormats[1], on_load::clear.from_previous_layout(layout::undefined), usage::unused        >> usage::unused                              , on_store::store.in_layout(layout::shader_read_only_optimal)),
                attachment::declare(colorFormatMS        , on_load::clear.from_previous_layout(layout::undefined), usage::color(0)      >> usage::color(0)      + usage::resolve_to(0), on_store::dont_care),
                attachment::declare(depthFormatMS        , on_load::clear.from_previous_layout(layout::undefined), usage::depth_stencil >> usage::depth_stencil + usage::resolve_to(1), on_store::dont_care)
            }, {
				subpass_dependency{subpass::external >> subpass::index(0),
					stage::none    >>   stage::color_attachment_output | stage::early_fragment_tests | stage::late_fragment_tests,
					access::none   >>   access::color_attachment_write | access::depth_stencil_attachment_read | access::depth_stencil_attachment_write
				},
				subpass_dependency{subpass::index(0) >> subpass::index(1),
					stage::color_attachment_output | stage::early_fragment_tests | stage::late_fragment_tests  >>   stage::color_attachment_output | stage::early_fragment_tests | stage::late_fragment_tests,
					access::color_attachment_write | access::depth_stencil_attachment_write                    >>   access::color_attachment_write | access::depth_stencil_attachment_read | access::depth_stencil_attachment_write
				},
				subpass_dependency{subpass::index(1) >> subpass::external, // MS rendering
					stage::color_attachment_output | stage::early_fragment_tests | stage::late_fragment_tests   >>   stage::color_attachment_output | stage::early_fragment_tests | stage::late_fragment_tests,
					access::color_attachment_write | access::depth_stencil_attachment_write                     >>   access::color_attachment_write | access::depth_stencil_attachment_read | access::depth_stencil_attachment_write
				}
			});

        mFramebufferMS = context().create_framebuffer(mRenderpassMS, make_vector(
			colorAttachmentView, 
			depthAttachmentView,
			colorAttachmentViewMS,
			depthAttachmentViewMS
		));

        mRenderpassSSMS = context().create_renderpass({
                attachment::declare(aAttachmentFormats[0], on_load::clear.from_previous_layout(layout::undefined), usage::unused        >> usage::unused                               , on_store::store.in_layout(layout::transfer_src)),
                attachment::declare(aAttachmentFormats[1], on_load::clear.from_previous_layout(layout::undefined), usage::unused        >> usage::unused                               , on_store::store.in_layout(layout::transfer_src)),
                attachment::declare(colorFormatMS        , on_load::clear.from_previous_layout(layout::undefined), usage::color(0)      >> usage::color(0)      + usage::resolve_to(0) , on_store::dont_care),
                attachment::declare(depthFormatMS        , on_load::clear.from_previous_layout(layout::undefined), usage::depth_stencil >> usage::depth_stencil + usage::resolve_to(1) , on_store::dont_care)
            }, {
				subpass_dependency{subpass::external >> subpass::index(0),
					stage::color_attachment_output | stage::early_fragment_tests | stage::late_fragment_tests   >>   stage::color_attachment_output | stage::early_fragment_tests | stage::late_fragment_tests,
					access::color_attachment_write | access::depth_stencil_attachment_write                     >>   access::color_attachment_write | access::depth_stencil_attachment_read | access::depth_stencil_attachment_write
				},
				subpass_dependency{subpass::index(0) >> subpass::index(1), // Full-screen quad NoAA -> MS
					stage::color_attachment_output | stage::early_fragment_tests | stage::late_fragment_tests   >>   stage::color_attachment_output | stage::early_fragment_tests | stage::late_fragment_tests,
					access::color_attachment_write | access::depth_stencil_attachment_write                     >>   access::color_attachment_write | access::depth_stencil_attachment_read | access::depth_stencil_attachment_write
				},
				subpass_dependency{subpass::index(1) >> subpass::external,
					stage::color_attachment_output  >>  stage::transfer,
					access::color_attachment_write  >>  access::memory_read
				}
			});

        mFramebufferSSMS = context().create_framebuffer(mRenderpassSSMS, make_vector(
			colorAttachmentViewSS,
			depthAttachmentViewSS,
			colorAttachmentViewSSMS,
			depthAttachmentViewSSMS
		));
    }

	bool is_knit_yarn(parametric_object_type pot)
	{
		return pot == parametric_object_type::SingleYarnCurve
			|| pot == parametric_object_type::SingleFiberCurve
			|| pot == parametric_object_type::CurtainYarnCurves
			|| pot == parametric_object_type::CurtainFiberCurves;
	}

	bool is_sh_brain(parametric_object_type pot)
	{
		return pot == parametric_object_type::SHBrain;
	}

	bool is_giant_worm(parametric_object_type pot)
	{
		return pot == parametric_object_type::GiantWorm;
	}

	void fill_object_data_buffer()
	{
		using namespace avk;

		mObjectData.resize(MAX_OBJECTS, object_data{});
		mKnitYarnObjectDataIndices.clear();
		mShBrainObjectDataIndices.clear();
		std::vector<object_data> knitYarnSavedForLater;
		std::vector<object_data> shBrainSavedForLater;

		uint32_t i = 0; 
	    for (const auto& po : mParametricObjects) {
            if (!po.is_enabled()) {
                continue;
            }

			object_data tmp;
            tmp.mParams = po.uv_param_ranges();
			tmp.mCurveIndex = po.curve_index();
			tmp.mDetailEvalDims = po.eval_dims();
			if (tmp.mDetailEvalDims.x > 8 || tmp.mDetailEvalDims.y > 4) {
				LOG_WARNING(std::format("Eval dimensions higher than 8x4 are not supported (due to to pass1_init_patches.comp's implementation). The dimensions specified for {} are {}x{}.", po.name(), tmp.mDetailEvalDims[0], tmp.mDetailEvalDims[1]));
				tmp.mDetailEvalDims.x = std::clamp(tmp.mDetailEvalDims.x, 1u, 8u);
				tmp.mDetailEvalDims.x = std::clamp(tmp.mDetailEvalDims.x, 1u, 4u);
			}
            tmp.mTransformationMatrix = po.transformation_matrix();
			tmp.mMaterialIndex = po.material_index();
			tmp.mUseAdaptiveDetail = po.adaptive_rendering_on() ? 1 : 0;
			tmp.mRenderingVariantIndex = get_rendering_variant_index(po.how_to_render());

			tmp.mLodAndRenderSettings = { 
				po.parameters_epsilon()[0], po.parameters_epsilon()[1],
				po.screen_distance_threshold() / get_screen_space_threshold_divisor(po.how_to_render()),
				0xDAD
			};

			tmp.mTessAndSamplingSettings = {
				po.tessellation_levels().x, po.tessellation_levels().y,
				po.sampling_factors().x   , po.sampling_factors().y
			};
			
			tmp.mUserData = { 0, 0, 0, 0 };

			if (is_knit_yarn(po.param_obj_type())) {
				knitYarnSavedForLater.push_back(tmp);
			}
			else if (is_sh_brain(po.param_obj_type())) {
				shBrainSavedForLater.push_back(tmp);
			}
			else if (is_giant_worm(po.param_obj_type())) {
				auto get_material_idx = [baseMatIdx = po.material_index(), this](int offset) {
					if (baseMatIdx <= -5 || (baseMatIdx >= 0 && baseMatIdx < mNumMaterials + NUM_PREDEFINED_MATERIALS - 3)) {
						return baseMatIdx + offset; 
					}
					return baseMatIdx;
				};
				constexpr int red = 0, green = 1, blue = 2;
				// Giant worm body:
				tmp.mCurveIndex = 14;
				tmp.mMaterialIndex = get_material_idx(blue);
				mObjectData[i++] = tmp;
				// Giant worm mouth piece (inside) x3:
				tmp.mCurveIndex = 15;
				tmp.mMaterialIndex = get_material_idx(red);
				tmp.mUserData.x = 0;
				mObjectData[i++] = tmp;
				tmp.mUserData.x = 1;
				mObjectData[i++] = tmp;
				tmp.mUserData.x = 2;
				mObjectData[i++] = tmp;
				// Giant worm mouth piece (outside) x3:
				tmp.mCurveIndex = 16;
				tmp.mMaterialIndex = get_material_idx(blue);
				tmp.mUserData.x = 0;
				mObjectData[i++] = tmp;
				tmp.mUserData.x = 1;
				mObjectData[i++] = tmp;
				tmp.mUserData.x = 2;
				mObjectData[i++] = tmp;
				// Giant worm tongue:
				tmp.mCurveIndex = 17;
				tmp.mMaterialIndex = get_material_idx(red);
				mObjectData[i++] = tmp;
				// Giant worm teeth:
				tmp.mCurveIndex = 18;
				tmp.mMaterialIndex = get_material_idx(green);
				for (int a = 0; a < 3; ++a) {
					for (int b = 0; b < 19; ++b) {
						tmp.mUserData.x = a;
						tmp.mUserData.y = b;
						mObjectData[i++] = tmp;
					}
				}
			}
			else {
				mObjectData[i] = tmp;
				++i;
			}
        }

		mNumEnabledObjects = i;

		for (const auto& ky : knitYarnSavedForLater) {
			mObjectData[i] = ky;
			mKnitYarnObjectDataIndices.push_back(i);
			++i;
		}

		const auto numEnabledKnitYarnsIndex = i;
		mNumEnabledKnitYarnObjects = numEnabledKnitYarnsIndex - mNumEnabledObjects;

		for (const auto& shb : shBrainSavedForLater) {
			mObjectData[i] = shb;
			mShBrainObjectDataIndices.push_back(i);
			++i;
		}

		const auto numEnabledShBrainsIndex = i;
		mNumEnabledShBrainObjects = numEnabledShBrainsIndex - numEnabledKnitYarnsIndex;

		// Submit:
		const auto currentFrameId = context().main_window()->current_frame();
        auto sem = context().record_and_submit_with_semaphore({
            mObjectDataBuffer->fill(mObjectData.data(), 0)
		}, *mQueue, stage::transfer);
			
		// Ensure that the copy is done before the next frame begins to render:
        context().main_window()->add_present_dependency_for_frame(std::move(sem), currentFrameId + 1); 
	}

	void draw_parametric_objects_ui(avk::imgui_manager* aImguiManager)
	{
		using namespace avk;

		// Draw some ImGuizmo?!
		auto res = glm::vec2{ context().main_window()->resolution() };
		ImGuizmo::BeginFrame();
		ImGuizmo::SetRect(0, 0, res.x, res.y);
		//ImGuizmo::AllowAxisFlip(true);
		//ImGuizmo::Enable(false);

		auto viewMatrix = mOrbitCam.view_matrix();
		auto projMatrix = mOrbitCam.projection_matrix();
		projMatrix[1][1] *= -1;


		bool updateObjects = false;
		// Draw table of parametric objects available:
		const float wndWdth = context().main_window()->resolution().x;
		float tableColumnWidth  = wndWdth * 0.9f / mParametricObjects.size();
		float previewImageWidth = glm::min(wndWdth * 0.9f / mParametricObjects.size(), 100.0f);
		ImGui::Begin("Parametric Objects", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove);
		ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(wndWdth, 300), ImGuiCond_Once);
		if (ImGui::BeginTable("Parametric Objects Table", mParametricObjects.size())) {
			for (auto& po : mParametricObjects) {
				ImGui::TableSetupColumn(po.name(), ImGuiTableColumnFlags_WidthFixed, tableColumnWidth);
			}
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();
			for (auto& po : mParametricObjects) {
				ImGui::TableNextColumn();
				ImTextureID inputTexId = aImguiManager->get_or_create_texture_descriptor(mPreviewImageSamplers[po.preview_image_path()].as_reference(), avk::layout::shader_read_only_optimal);
				ImGui::Image(inputTexId, ImVec2(previewImageWidth, previewImageWidth), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
			}

			// One table row for checkboxes to enable/disable DRAWING, and for showing ImGuizmo for MODificiation:
			int modifyIndex = -1;
			ImGui::TableNextRow();
			int poId  = 0;
			for (auto& po : mParametricObjects) {
				ImGui::TableNextColumn();
				ImGui::PushID(poId++);
				bool enabled = po.is_enabled();
				if (bool changed = ImGui::Checkbox("Draw", &enabled)) {
					po.set_enabled(enabled);
					updateObjects = true;

					if (po.name() == "Blue Curtain") {
						// Gotta ensure that Sponza is loaded:
						load_sponza_and_terrain();
					}
				}
				ImGui::SameLine();
				bool modifying = po.is_modifying();
				if (bool changed = ImGui::Checkbox("Mod.", &modifying)) {
					modifyIndex = poId - 1;
					po.set_modifying(modifying);
				}
				ImGui::PopID();
			}
			if (modifyIndex >= 0) {
				poId = 0;
				for (auto& po : mParametricObjects) {
					if (modifyIndex != poId) {
						po.set_modifying(false);
					}
					poId++;
				}
			}
			for (auto& po : mParametricObjects) {
				bool modifying = po.is_modifying();
				if (modifying) {
					glm::mat4 modelMatrix = po.transformation_matrix();
					if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix), ImGuizmo::UNIVERSAL, ImGuizmo::LOCAL, glm::value_ptr(modelMatrix))) {
						po.set_transformation_matrix(modelMatrix);
						updateObjects = true;
					}
				}
			}

			// One table row for setting the rendering mdethod:
			ImGui::TableNextRow();
			poId = 0;
			for (auto& po : mParametricObjects) {
				ImGui::TableNextColumn();
				ImGui::PushID(poId++);
				ImGui::Text("Rendering method:");
				ImGui::PopID();
			}
			ImGui::TableNextRow();
			poId = 0;
			for (auto& po : mParametricObjects) {
				ImGui::TableNextColumn();
				ImGui::PushID(poId);
				auto howRendered = po.how_to_render();
#if ENABLE_HYBRID_TECHNIQUE
				if (ImGui::Combo("##renderingmethod", reinterpret_cast<int*>(&howRendered), "Tess. noAA\0Tess. 8xSS (sample shading)\0Tess. 4xSS+8xMS\0Point rendering ~1spp direct\0Point rendering ~4spp local fb.\0Hybrid\0")) {
#else
				if (ImGui::Combo("##renderingmethod", reinterpret_cast<int*>(&howRendered), "Tess. noAA\0Tess. 8xSS (sample shading)\0Tess. 4xSS+8xMS\0Point rendering ~1spp direct\0Point rendering ~4spp local fb.\0")) {
#endif
					po.set_how_to_render(howRendered);
					updateObjects = true;
				}
				poId++;
				ImGui::PopID();
			}

			// One table row for some buttons about duplicating/deleting objects:
			ImGui::TableNextRow();
			int duplicateIndex = -1;
			int deleteIndex = -1;
			poId = 0;
			for (auto& po : mParametricObjects) {
				ImGui::TableNextColumn();
				ImGui::PushID(poId++);
				if (ImGui::Button("Dupl.")) {
					duplicateIndex = poId - 1;
				}
				ImGui::SameLine();
				if (ImGui::Button("Del.")) {
					deleteIndex = poId - 1;
				}
				ImGui::PopID();
			}
			// Perform dupl. or del. action:
			if (duplicateIndex >= 0) {
				mParametricObjects.insert(mParametricObjects.begin() + duplicateIndex + 1, mParametricObjects[duplicateIndex]);
				updateObjects = true;
			}
			if (deleteIndex >= 0) {
				mParametricObjects.erase(mParametricObjects.begin() + deleteIndex);
				updateObjects = true;
			}

			// One table row for setting the material index:
			ImGui::TableNextRow();
			poId = 0;
			for (auto& po : mParametricObjects) {
				ImGui::TableNextColumn();
				auto matIndex = po.material_index() + NUM_PREDEFINED_MATERIALS;
				ImGui::PushID(poId++);
				if (ImGui::SliderInt("Material Index", &matIndex, 0, mNumMaterials + NUM_PREDEFINED_MATERIALS - 1)) {
					const auto newActualMatIndex = matIndex - NUM_PREDEFINED_MATERIALS;
					LOG_INFO(std::format("Set {}'s (actual) material index to: {}", po.name(), newActualMatIndex));
					po.set_material_index(newActualMatIndex);
					updateObjects = true;
				}
				ImGui::PopID();
			}

			// One more row just for SH glyphs:
			ImGui::TableNextRow();
			poId = 0;
			for (auto& po : mParametricObjects) {
				ImGui::TableNextColumn();
				ImGui::PushID(poId++);
				if (po.param_obj_type() == parametric_object_type::SHGlyph) {
					int tmpOrder = mDebugSlidersi[0];
					ImGui::SliderInt("SH Order", &tmpOrder, 2, 12);
					tmpOrder = (tmpOrder / 2) * 2;
					mDebugSlidersi[0] = tmpOrder;
				}
				if (po.param_obj_type() == parametric_object_type::SHBrain) {
					int tmpOrder = mDebugSlidersi[1];
					ImGui::SliderInt("SH Order", &tmpOrder, 2, 12);
					tmpOrder = (tmpOrder / 2) * 2;
					mDebugSlidersi[1] = tmpOrder;
				}
				ImGui::PopID();
			}

			ImGui::EndTable();
		}
		ImGui::End();

		for (auto& po : mParametricObjects) {
			bool modifying = po.is_modifying();
			if (modifying) {
				ImGui::Begin("Mod. Parameters", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove);
				ImGui::SetWindowPos(ImVec2(wndWdth - 300, 302), ImGuiCond_Once);
				ImGui::SetWindowSize(ImVec2(300, 200), ImGuiCond_Once);

                auto curveIndex = po.curve_index();
                if (ImGui::Combo("Curve Type", &curveIndex, PARAMETRIC_OBJECT_TYPE_UI_STRING)) {
                    po.set_curve_index(curveIndex);
                    updateObjects = true;
                }
                auto uParams = po.u_param_range();
                if (ImGui::DragFloat2("Param u from..to", &uParams[0], 0.1f)) {
                    po.set_u_param_range(uParams);
                    updateObjects = true;
                }
                auto vParams = po.v_param_range();
                if (ImGui::DragFloat2("Param v from..to", &vParams[0], 0.1f)) {
                    po.set_v_param_range(vParams);
                    updateObjects = true;
                }
                auto evalDims = glm::ivec4{po.eval_dims()};
                if (ImGui::SliderInt2("Eval. dimensions", &evalDims[0], 1, 16)) {
                    po.set_eval_dims(evalDims);
                    updateObjects = true;
                }

				auto t = po.screen_distance_threshold();
				if (ImGui::SliderFloat("Screen Distance Threshold", &t, 10.0f, 500.0f)) {
					po.set_screen_distance_threshold(t);
					updateObjects = true;
				}

				auto eps = po.parameters_epsilon();
				if (ImGui::DragFloat2("Parameters Epsilons", &eps[0], 0.001f)) {
					po.set_parameters_epsilon(eps);
					updateObjects = true;
				}

				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Tessellation variants only:");
				auto tesslvls = po.tessellation_levels();
				if (ImGui::SliderFloat2("Tess. levels: inner | outer", &tesslvls[0], 1.0f, 64.0f, "%.1f")) {
					po.set_tessellation_levels(tesslvls);
					updateObjects = true;
				}

				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Point rendering variants only:");
				auto f = po.sampling_factors();
				if (ImGui::DragFloat2("Sampling Factors (u, v)", &f[0], 0.01f, 0.1f, 10.0f)) {
					po.set_sampling_factors(f);
					updateObjects = true;
				}

				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Applies to all variants:");
				auto adaptive = po.adaptive_rendering_on();
				if (ImGui::Checkbox("Adaptive tessellation/sampling density", &adaptive)) {
					po.set_adaptive_rendering_on(adaptive);
					updateObjects = true;
				}

				auto [transl, rot, scale] = avk::transforms_from_matrix(po.transformation_matrix());
                if (ImGui::DragFloat3("Translation", &transl[0], 0.01f)) {
                    po.set_transformation_matrix(avk::matrix_from_transforms(transl, rot, scale));
                    updateObjects = true;
                }
				auto euler = glm::eulerAngles(rot);
                if (ImGui::DragFloat4("Rotation", &euler[0], 0.01f)) {
					rot = glm::quat(euler);
                    po.set_transformation_matrix(avk::matrix_from_transforms(transl, rot, scale));
                    updateObjects = true;
                }
                if (ImGui::DragFloat3("Scale", &scale[0], 0.01f)) {
                    po.set_transformation_matrix(avk::matrix_from_transforms(transl, rot, scale));
                    updateObjects = true;
                }

				ImGui::End();
			}
		}

		if (updateObjects) {
			fill_object_data_buffer();
		}
	}

    void initialize() override
	{
		using namespace avk;

		// use helper functions to create ImGui elements
		auto surfaceCap = context().physical_device().getSurfaceCapabilitiesKHR(context().main_window()->surface());

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = context().create_descriptor_cache();

		create_buffers();
		load_sponza_and_terrain();
		load_sh_brain_dataset();

		// Define formats for the framebuffer attachments:
        constexpr auto attachmentFormats = make_array<vk::Format>(vk::Format::eB8G8R8A8Unorm, vk::Format::eD32Sfloat);

		// Create a framebuffer and our Int64 auxiliary image (which is key):
        create_framebuffer_and_auxiliary_images(attachmentFormats);

		// Create an SSBO containing 4 counters for whatever usage
		mCountersSsbo = context().create_buffer(
			memory_usage::device,
#if STATS_ENABLED
			vk::BufferUsageFlagBits::eTransferSrc,
#else
			{},
#endif
			storage_buffer_meta::create_from_data(mCounterValues)
		);

	    // Create an updater, it is going to be used in create_meshlet_pipes, and also further down in initialize():
        mUpdater.emplace();

        // Create the graphics mesh pipelines (EXT-one and NV-one):
        // Before creating a pipeline, let's query the VK_EXT_mesh_shader-specific device properties:
        // Also, just out of curiosity, query the subgroup properties too:
        vk::PhysicalDeviceMeshShaderPropertiesEXT meshShaderProps{};
        vk::PhysicalDeviceSubgroupProperties      subgroupProps;
        vk::PhysicalDeviceProperties2             phProps2{};
        phProps2.pNext        = &meshShaderProps;
        meshShaderProps.pNext = &subgroupProps;
        context().physical_device().getProperties2(&phProps2);
        LOG_INFO(std::format("Max. preferred task threads is {}, mesh threads is {}, subgroup size is {}.", meshShaderProps.maxPreferredTaskWorkGroupInvocations,
            meshShaderProps.maxPreferredMeshWorkGroupInvocations, subgroupProps.subgroupSize));
        LOG_INFO(std::format("This device supports the following subgroup operations: {}", vk::to_string(subgroupProps.supportedOperations)));
        LOG_INFO(std::format("This device supports subgroup operations in the following stages: {}", vk::to_string(subgroupProps.supportedStages)));
        mTaskInvocationsExt = meshShaderProps.maxPreferredTaskWorkGroupInvocations;
        mMeshInvocationsExt = meshShaderProps.maxPreferredMeshWorkGroupInvocations;

		// Create an SSBO for MAX_OBJECTS objects:
		mObjectDataBuffer = context().create_buffer(
			memory_usage::device, {},
			storage_buffer_meta::create_from_size(MAX_OBJECTS * sizeof(object_data))
		);
		fill_object_data_buffer();

		// Create buffers for the patch LOD steps ("ping/pong"):
		mPatchLodBufferPing = context().create_buffer(
			memory_usage::device, {},
			storage_buffer_meta::create_from_size(MAX_LOD_PATCHES * sizeof(px_fill_data))
		);
		mPatchLodBufferPong = context().create_buffer(
			memory_usage::device, {},
			storage_buffer_meta::create_from_size(MAX_LOD_PATCHES * sizeof(px_fill_data))
		);
		mPatchLodCountBuffer = context().create_buffer(
			memory_usage::device,
#if STATS_ENABLED
			vk::BufferUsageFlagBits::eTransferSrc,
#else 
			{},
#endif
			indirect_buffer_meta::create_from_num_elements(MAX_PATCH_SUBDIV_STEPS,  sizeof(glm::uvec4)),
			storage_buffer_meta::create_from_size(         MAX_PATCH_SUBDIV_STEPS * sizeof(glm::uvec4)) // 1 counter per "ping/pong" compute invocation
		);

#if SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
		mTilePatchesBuffer = context().create_buffer(
			memory_usage::device, {},
			storage_buffer_meta::create_from_size(TILE_PATCHES_BUFFER_ELEMENTS * sizeof(uint32_t))
		);
#endif

		// Create buffers for the indirect draw calls of the parametric pipelines:
        mIndirectPxFillParamsBuffer = context().create_buffer(
			memory_usage::device, {}, 
			storage_buffer_meta::create_from_size(NUM_DIFFERENT_RENDER_VARIANTS * MAX_INDIRECT_DISPATCHES * sizeof(px_fill_data))
		);
		// ATTENTION: We're going to use the COUNT buffer for both, the compute-based pixel fill method, and also for
		//            the "patch into tessellation" method. For the latter, we need a full VkDrawIndirectCommand structure,
		//            for the former, the INSTANCE COUNT contains the #draw calls.
        mIndirectPxFillCountBuffer = context().create_buffer(
			memory_usage::device, 
#if STATS_ENABLED
			vk::BufferUsageFlagBits::eTransferSrc,
#else 
			{},
#endif
			// NOTE: We're creating NUM_DIFFERENT_RENDER_VARIANTS-many such  vvv  entries, because: [0] = Tess_noAA, [1] = Tess_8xMS, [2] = Tess_4xSS_8xMS, [3] = PointRendered_direct, [4] = PointRendered_4xSS_local_fb
			indirect_buffer_meta::create_from_num_elements(NUM_DIFFERENT_RENDER_VARIANTS,  sizeof(VkDrawIndirectCommand)),
			storage_buffer_meta::create_from_size(         NUM_DIFFERENT_RENDER_VARIANTS * sizeof(VkDrawIndirectCommand))
		);

		// PARAMETRIC PIPES:
        create_param_pipes();

		// VERTEX and TESS. PIPES:
		mVertexPipeline = create_vertex_pipe();
		mUpdater->on(shader_files_changed_event(mVertexPipeline.as_reference())).update(mVertexPipeline);

		mFsQuadColorSampler = avk::context().create_sampler(avk::filter_mode::trilinear, avk::border_handling_mode::clamp_to_border, 0.0f);
		mFsQuadDepthSampler = avk::context().create_sampler(avk::filter_mode::trilinear , avk::border_handling_mode::clamp_to_border, 0.0f);
		mFsQuadNoAAtoMSPipe = create_noaa_to_ms_pipe();
		mFsQuadMStoSSPipe   = create_ms_to_ssms_pipe();
		mUpdater->on(shader_files_changed_event(mFsQuadNoAAtoMSPipe.as_reference())).update(mFsQuadNoAAtoMSPipe);
		mUpdater->on(shader_files_changed_event(mFsQuadMStoSSPipe.as_reference())).update(mFsQuadMStoSSPipe);

		mTessPipelinePxFillNoaa = create_tess_pipe(
			"shaders/px-fill-tess/patch_ready.vert", 
			"shaders/px-fill-tess/patch_set.tesc", 
			"shaders/px-fill-tess/patch_go.tese",
			push_constant_binding_data{shader_type::all, 0, sizeof(patch_into_tess_push_constants)},
			cfg::shade_per_fragment(),
			cfg::viewport_depth_scissors_config::from_framebuffer(mFramebufferNoAA.as_reference()),
			mRenderpassNoAA, cfg::subpass_index{ 0 }
		);
		mUpdater->on(shader_files_changed_event(mTessPipelinePxFillNoaa.as_reference())).update(mTessPipelinePxFillNoaa);

		// Create an (almost identical) pipeline to render the scene in wireframe mode
		mTessPipelinePxFillNoaaWireframe = context().create_graphics_pipeline_from_template(mTessPipelinePxFillNoaa.as_reference(), [](graphics_pipeline_t& p) {
			p.rasterization_state_create_info().setPolygonMode(vk::PolygonMode::eLine);
		});
		mUpdater->on(shader_files_changed_event(mTessPipelinePxFillNoaaWireframe.as_reference())).update(mTessPipelinePxFillNoaaWireframe);

		mTessPipelinePxFillMultisampled = create_tess_pipe(
			"shaders/px-fill-tess/patch_ready.vert", 
			"shaders/px-fill-tess/patch_set.tesc", 
			"shaders/px-fill-tess/patch_go.tese",
			push_constant_binding_data{shader_type::all, 0, sizeof(patch_into_tess_push_constants)},
			mEnableSampleShadingFor8xMS ? cfg::shade_per_sample() : cfg::shade_per_fragment(),
			cfg::viewport_depth_scissors_config::from_framebuffer(mFramebufferMS.as_reference()),
			mRenderpassMS, cfg::subpass_index{ 1 }
		);
		mUpdater->on(shader_files_changed_event(mTessPipelinePxFillMultisampled.as_reference())).update(mTessPipelinePxFillMultisampled);

		// Create an (almost identical) pipeline to render the scene in wireframe mode
		mTessPipelinePxFillMultisampledWireframe = context().create_graphics_pipeline_from_template(mTessPipelinePxFillMultisampled.as_reference(), [](graphics_pipeline_t& p) {
			p.rasterization_state_create_info().setPolygonMode(vk::PolygonMode::eLine);
		});
		mUpdater->on(shader_files_changed_event(mTessPipelinePxFillMultisampledWireframe.as_reference())).update(mTessPipelinePxFillMultisampledWireframe);

		mTessPipelinePxFillSupersampled = create_tess_pipe(
			"shaders/px-fill-tess/patch_ready.vert", 
			"shaders/px-fill-tess/patch_set.tesc", 
			"shaders/px-fill-tess/patch_go.tese",
			push_constant_binding_data{shader_type::all, 0, sizeof(patch_into_tess_push_constants)},
			mEnableSampleShadingFor4xSS8xMS ? cfg::shade_per_sample() : cfg::shade_per_fragment(),
			cfg::viewport_depth_scissors_config::from_framebuffer(mFramebufferSSMS.as_reference()),
			mRenderpassSSMS, cfg::subpass_index{ 1 }
		);
		mUpdater->on(shader_files_changed_event(mTessPipelinePxFillSupersampled.as_reference())).update(mTessPipelinePxFillSupersampled);

		// Create an (almost identical) pipeline to render the scene in wireframe mode
		mTessPipelinePxFillSupersampledWireframe = context().create_graphics_pipeline_from_template(mTessPipelinePxFillSupersampled.as_reference(), [](graphics_pipeline_t& p) {
			p.rasterization_state_create_info().setPolygonMode(vk::PolygonMode::eLine);
		});
		mUpdater->on(shader_files_changed_event(mTessPipelinePxFillSupersampledWireframe.as_reference())).update(mTessPipelinePxFillSupersampledWireframe);

		mCopyToBackufferPipe = context().create_compute_pipeline_for(
			"shaders/copy_to_backbuffer.comp",
			push_constant_binding_data{ shader_type::all, 0, sizeof(copy_to_backbuffer_push_constants) },
			descriptor_binding(0, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
			descriptor_binding(0, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
			descriptor_binding(1, 0, context().main_window()->backbuffer_at_index(0)->image_view_at(0)->as_storage_image(layout::general))
		);
		mUpdater->on(shader_files_changed_event(mCopyToBackufferPipe.as_reference())).update(mCopyToBackufferPipe);
		
		mClearCombinedAttachmentPipe = context().create_compute_pipeline_for(
			"shaders/clear_r64_image.comp",
			descriptor_binding(0, 0, mCountersSsbo->as_storage_buffer()),
			// The buffers containing object and px fill data:
			descriptor_binding(1, 0, mObjectDataBuffer->as_storage_buffer()),
			descriptor_binding(1, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
			descriptor_binding(1, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
            // super druper image & heatmap image:
            descriptor_binding(2, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
            descriptor_binding(2, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
			// Ping/pong buffers:
			descriptor_binding(3, 0, mPatchLodBufferPing->as_storage_buffer()),
			descriptor_binding(3, 1, mPatchLodBufferPong->as_storage_buffer()),
			descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer())
#if SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
			, descriptor_binding(4, 0, mTilePatchesBuffer->as_storage_buffer())
#endif
		);
        mUpdater->on(shader_files_changed_event(mClearCombinedAttachmentPipe.as_reference())).update(mClearCombinedAttachmentPipe);

		mCopyToCombinedAttachmentPipe = context().create_compute_pipeline_for(
			"shaders/copy_to_combined_attachment.comp",
            descriptor_binding(0, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
			descriptor_binding(1, 0, mFramebufferNoAA->image_view_at(0)->as_sampled_image(layout::shader_read_only_optimal)),
			descriptor_binding(1, 1, mFramebufferNoAA->image_view_at(1)->as_sampled_image(layout::shader_read_only_optimal))
		);
        mUpdater->on(shader_files_changed_event(mCopyToCombinedAttachmentPipe.as_reference())).update(mCopyToCombinedAttachmentPipe);

		mUpdater->on(swapchain_resized_event(context().main_window())).invoke([this]() {
			// Recreate all the resources (framebuffer, aux. image):
            create_framebuffer_and_auxiliary_images(attachmentFormats);
            this->mQuakeCam.set_aspect_ratio(context().main_window()->aspect_ratio());
            this->mOrbitCam.set_aspect_ratio(context().main_window()->aspect_ratio());
		}).update(mCopyToBackufferPipe, mClearCombinedAttachmentPipe);

		// Create a couple of spheres in the scene:
		constexpr int SPHERE_FACTOR = ((NUM_MODELS_PER_DIM-1) * 3) / 2;
		for (int x = -SPHERE_FACTOR; x <= SPHERE_FACTOR; x += 3) {
			for (int z = -SPHERE_FACTOR; z <= SPHERE_FACTOR; z += 3) {
				mSpherePositions.push_back(glm::vec3(x, 0.f, z));
			}
		}
		
		// Add the camera to the composition (and let it handle the updates)
		auto camPos = glm::angleAxis(0.f, up()) * glm::vec3{ 0.0f, 4.0f, 20.0f };
		mOrbitCam.set_translation(camPos);
		mOrbitCam.look_at(glm::vec3{0.0f});
		mOrbitCam.set_pivot_distance(glm::length(mOrbitCam.translation()));
		mOrbitCam.set_perspective_projection(glm::radians(45.0f), context().main_window()->aspect_ratio(), 1.0f, 60000.0f);
		mQuakeCam.set_perspective_projection(glm::radians(45.0f), context().main_window()->aspect_ratio(), 1.0f, 60000.0f);
		current_composition()->add_element(mOrbitCam);
		current_composition()->add_element(mQuakeCam);
		mQuakeCam.disable();

		// One sampler for all the preview images:
		auto previewImageSampler = avk::context().create_sampler(avk::filter_mode::bilinear, avk::border_handling_mode::clamp_to_edge);
		// Upload all the preview images:
		std::vector<avk::recorded_commands_t> imageUploadCommands;
		for (auto& po : PredefinedParametricObjects) {
			if (mPreviewImageSamplers.contains(po.preview_image_path())) {
				continue;
			}

			// Load an image from file, upload it and then create a view and a sampler for it for usage in shaders:
			auto [previewImage, uploadInputImageCommand] = avk::create_image_from_file(
				po.preview_image_path(), false, false, false, 4,
				avk::layout::shader_read_only_optimal, // For now, transfer the image into transfer_dst layout, because we'll need to copy from it:
				avk::memory_usage::device, // The device shall be stored in (fast) device-local memory. For this reason, the function will also return commands that we need to execute later
				avk::image_usage::general_storage_image // Note: We could bind the image as a texture instead of a (readonly) storage image, then we would not need the "storage_image" usage flag 
			);
			// The uploadInputImageCommand will contain a copy operation from a temporary host-coherent buffer into a device-local buffer.
			// We schedule it for execution a bit further down.		
			mPreviewImageSamplers[po.preview_image_path()] = avk::context().create_image_sampler(avk::context().create_image_view(previewImage), previewImageSampler);
			imageUploadCommands.push_back(std::move(uploadInputImageCommand));
		}
		// Upload everything to the GPU:
		context().record_and_submit_with_fence({ std::move(imageUploadCommands) }, *mQueue)->wait_until_signalled();

		std::locale::global(std::locale("en_US.UTF-8"));
		auto imguiManager = current_composition()->element_by_type<imgui_manager>();
        if (nullptr != imguiManager) {
			imguiManager->add_callback([
				this, imguiManager,
				timestampPeriod = std::invoke([]() {
					// get timestamp period from physical device, could be different for other GPUs
					auto props = context().physical_device().getProperties();
					return static_cast<double>(props.limits.timestampPeriod);
				}),
				lastClearDuration				 = 0.0,
				lastInitDuration				 = 0.0,
				lastLodStageDuration			 = 0.0,
				last3dModelsRenderDuration		 = 0.0,
				lastTessNoaaDuration			 = 0.0,
				lastCopyToCombinedAttachmentDur	 = 0.0,
				lastPatchToTileStepDuration		 = 0.0,
				lastPointRenderingNoaaDuration	 = 0.0,
				lastPointRenderingLocalFbDur	 = 0.0,
				lastFSQbefore8xSSDuration		 = 0.0,
				lastTess8xSSDuration			 = 0.0,
				lastFSQbefore4xSS8xMSDuration	 = 0.0,
				lastTess4xSS8xMSDuration		 = 0.0,
				lastTotalRenderDuration			 = 0.0,
				lastTotalStepsDuration			 = 0.0
			]() mutable {
				if (mMeasurementInProgress) {
					return;
				}

				auto numDrawCallsBefore = mDrawCalls.size();
				draw_parametric_objects_ui(imguiManager);
				auto numDrawCallsAfter  = mDrawCalls.size();
				bool rasterPipesNeedRecreation = numDrawCallsBefore != numDrawCallsAfter;

				ImGui::Begin("Info & Settings");
			    const auto imGuiWindowWidth = ImGui::GetWindowWidth();

				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::SetWindowSize(ImVec2(661, 643) , ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::Text("%.1lf ms/render() CPU time", mRenderDurationMs);
				ImGui::Separator();
				
				if (!mDrawCalls.empty()) {
				    ImGui::Separator();
				    ImGui::Checkbox("Render Sponza + Terrain", &mRenderExtra3DModel);
				}

				ImGui::Separator();
				bool quakeCamEnabled = mQuakeCam.is_enabled();
				if (ImGui::Checkbox("Enable Quake Camera", &quakeCamEnabled)) {
					if (quakeCamEnabled) { // => should be enabled
						mQuakeCam.set_matrix(mOrbitCam.matrix());
						mQuakeCam.enable();
						mOrbitCam.disable();
					}
				}
				if (quakeCamEnabled) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), "[Esc] to exit Quake Camera navigation");
					if (input().key_pressed(key_code::escape)) {
						mOrbitCam.set_matrix(mQuakeCam.matrix());
						mOrbitCam.enable();
						mQuakeCam.disable();
					}
                }
                else {
                    ImGui::TextColored(ImVec4(.8f, .4f, .4f, 1.f), "[Esc] to exit application");
                }
				if (imguiManager->begin_wanting_to_occupy_mouse() && mOrbitCam.is_enabled()) {
					mOrbitCam.disable();
				}
				if (imguiManager->end_wanting_to_occupy_mouse() && !mQuakeCam.is_enabled()) {
					mOrbitCam.enable();
				}
				ImGui::Separator();

				ImGui::Checkbox("Frustum Culling ON", &mFrustumCullingOn);
				rasterPipesNeedRecreation = ImGui::Checkbox("Backface Culling ON", &mBackfaceCullingOn) || rasterPipesNeedRecreation;
				rasterPipesNeedRecreation = ImGui::Checkbox("Disable Color Attachment Output" , &mDisableColorAttachmentOut) || rasterPipesNeedRecreation;

				ImGui::Text("Enable/disable render result transfers:");
				ImGui::Indent();
				ImGui::Checkbox("Tess. noAA -> Point-based", &mRenderVariantDataTransferEnabled[0]);
				ImGui::Checkbox("Point-based -> 8xMS", &mRenderVariantDataTransferEnabled[1]);
				ImGui::Checkbox("8xMS -> 4xSS+8xMS", &mRenderVariantDataTransferEnabled[2]);
				ImGui::Unindent();

				ImGui::Combo("-> results -> backbuffer", &mOutputResultOfRenderVariantIndex, "After tess. noAA\0After point-based\0After 8xMS\0After 4xSS+8xMS\0");

				ImGui::Text("Enable/disable sample shading:");
				ImGui::Indent();
				rasterPipesNeedRecreation = ImGui::Checkbox("Sample shading enabled for 8xMS", &mEnableSampleShadingFor8xMS) || rasterPipesNeedRecreation;
				if (mEnableSampleShadingFor8xMS) {
					ImGui::SameLine(); ImGui::TextColored(ImVec4(.4f, .4f, .8f, 1.f), "-> 8xSS");
				}
				rasterPipesNeedRecreation = ImGui::Checkbox("Sample shading enabled for 4xSS+8xMS", &mEnableSampleShadingFor4xSS8xMS) || rasterPipesNeedRecreation;
				if (mEnableSampleShadingFor4xSS8xMS) {
					ImGui::SameLine(); ImGui::TextColored(ImVec4(.4f, .4f, .8f, 1.f), "-> 32xSS");
				}
				ImGui::Unindent();

				ImGui::Checkbox("Render in wireframe mode ([F1] to toggle)", &mWireframeModeOn);

				if (ImGui::Checkbox("Animations paused (freeze time)", &mAnimationPaused)) {
                    if (mAnimationPaused) {
                        mAnimationPauseTime = time().absolute_time();
                    }
                    else {
						mTimeToSubtract += time().absolute_time() - mAnimationPauseTime;
                    }
                }

				//// Just some Debug stuff:
				//ImGui::Separator();
				//ImGui::Text("DEBUG SLIDERS:");
    //            ImGui::PushItemWidth(imGuiWindowWidth * 0.6f);
				//ImGui::SliderFloat("LERP SH <-> Sphere (aka float dbg slider #1)", &mDebugSliders[0], 0.0f, 1.0f);
				//ImGui::SliderInt("SH Band           (aka int dbg slider #1)", &mDebugSlidersi[0],  0, 31);
				//ImGui::SliderInt("SH Basis Function (aka int dbg slider #2)", &mDebugSlidersi[1], -mDebugSlidersi[0], mDebugSlidersi[0]);
				//ImGui::Text("##lololo");
				//ImGui::SliderFloat("Terrain height (aka float dbg slider #2)", &mDebugSliders[1], 0.0f, 10.0f);
				//ImGui::PopItemWidth();

				// Automatic performance measurement, camera flight:
				ImGui::Separator();

#if TEST_MODE_ON 
				ImGui::Checkbox("Start performance measurement (as configured with TEST_MODE_ON) [Space]", &mStartMeasurement);
#else 
				ImGui::Checkbox("Start performance measurement [Space]", &mStartMeasurement);
				ImGui::SliderFloat("Camera distance from origin", &mDistanceFromOrigin, 5.0f, 100.0f, "%.0f");
				ImGui::Checkbox("Move camera in steps during measurements", &mMoveCameraDuringMeasurements);
				ImGui::SliderInt("#steps to move the camera closer/away", &mMeasurementMoveCameraSteps, 0, 10);
				ImGui::DragFloat("Delta by how much to move the camera with each step", &mMeasurementMoveCameraDelta, 0.1f);
#endif

				ImGui::Separator();

ImGui::TextColored(ImVec4(.5f, .3f, .4f, 1.f), "Timestamp Period: %.3f ns", timestampPeriod);

#if STATS_ENABLED
				// Timestamps are gathered regardless of pipeline stats:
				lastClearDuration				= glm::mix(lastClearDuration				, mLastClearDuration				* 1e-6 * timestampPeriod, 0.05);
				lastInitDuration				= glm::mix(lastInitDuration					, mLastInitDuration					* 1e-6 * timestampPeriod, 0.05);
				lastLodStageDuration			= glm::mix(lastLodStageDuration				, mLastLodStageDuration				* 1e-6 * timestampPeriod, 0.05);
				last3dModelsRenderDuration		= glm::mix(last3dModelsRenderDuration		, mLast3dModelsRenderDuration		* 1e-6 * timestampPeriod, 0.05);
				lastTessNoaaDuration			= glm::mix(lastTessNoaaDuration				, mLastTessNoaaDuration				* 1e-6 * timestampPeriod, 0.05);
				lastCopyToCombinedAttachmentDur	= glm::mix(lastCopyToCombinedAttachmentDur	, mLastCopyToCombinedAttachmentDur	* 1e-6 * timestampPeriod, 0.05);
				lastPatchToTileStepDuration		= glm::mix(lastPatchToTileStepDuration		, mLastPatchToTileStepDuration		* 1e-6 * timestampPeriod, 0.05);
				lastPointRenderingNoaaDuration	= glm::mix(lastPointRenderingNoaaDuration	, mLastPointRenderingNoaaDuration	* 1e-6 * timestampPeriod, 0.05);
				lastPointRenderingLocalFbDur	= glm::mix(lastPointRenderingLocalFbDur		, mLastPointRenderingLocalFbDur		* 1e-6 * timestampPeriod, 0.05);
				lastFSQbefore8xSSDuration		= glm::mix(lastFSQbefore8xSSDuration		, mLastFSQbefore8xSSDuration		* 1e-6 * timestampPeriod, 0.05);
				lastTess8xSSDuration			= glm::mix(lastTess8xSSDuration				, mLastTess8xSSDuration				* 1e-6 * timestampPeriod, 0.05);
				lastFSQbefore4xSS8xMSDuration	= glm::mix(lastFSQbefore4xSS8xMSDuration	, mLastFSQbefore4xSS8xMSDuration	* 1e-6 * timestampPeriod, 0.05);
				lastTess4xSS8xMSDuration		= glm::mix(lastTess4xSS8xMSDuration			, mLastTess4xSS8xMSDuration			* 1e-6 * timestampPeriod, 0.05);
				lastTotalRenderDuration			= glm::mix(lastTotalRenderDuration			, mLastTotalStepsDuration			* 1e-6 * timestampPeriod, 0.05);
				lastTotalStepsDuration			= glm::mix(lastTotalStepsDuration			, mLastTotalStepsDuration			* 1e-6 * timestampPeriod, 0.05);

				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Total frame time        : %.3lf ms (timer queries)", lastTotalStepsDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Clearing                : %.3lf ms (timer queries)", lastClearDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Initialization stage    : %.3lf ms (timer queries)", lastInitDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "LOD stage               : %.3lf ms (timer queries)", lastLodStageDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Rendering Sponza        : %.3lf ms (timer queries)", last3dModelsRenderDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Tess. noAA              : %.3lf ms (timer queries)", lastTessNoaaDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "  -> FB -> combined att.: %.3lf ms (timer queries)", lastCopyToCombinedAttachmentDur);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "(Assign to local tiles) : %.3lf ms (timer queries)", lastPatchToTileStepDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Point rendering ~1spp   : %.3lf ms (timer queries)", lastPointRenderingNoaaDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Point r. ~4spp local fb.: %.3lf ms (timer queries)", lastPointRenderingLocalFbDur);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "   ^ TOTAL (with assign): %.3lf ms (timer queries)", lastPatchToTileStepDuration + lastPointRenderingLocalFbDur);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "  -> results -> FSQ     : %.3lf ms (timer queries)", lastFSQbefore8xSSDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Tess. 8xSS (sample shd.): %.3lf ms (timer queries)", lastTess8xSSDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "  -> results -> FSQ     : %.3lf ms (timer queries)", lastFSQbefore4xSS8xMSDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Tess. 4xSS+8xMS         : %.3lf ms (timer queries)", lastTess4xSS8xMSDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Total render duration   : %.3lf ms (timer queries)", lastTotalRenderDuration);

				// ... pipeline statistics only if the option is ticked:
			    if (ImGui::Checkbox("Gather pipeline stats.", &mGatherPipelineStats)) {
					mPipelineStats = {0,0};
				}
				if (mGatherPipelineStats) {
				    ImGui::Text(std::format(                       "pipeline stats #0   : {:12L} clipping    invocations", mPipelineStats[0]).c_str());
				    ImGui::Text(std::format(                       "pipeline stats #1   : {:12L} frag shader invocations", mPipelineStats[1]).c_str());
				    ImGui::Text(std::format(                       "pipeline stats #2   : {:12L} comp shader invocations", mPipelineStats[2]).c_str());

				}
				ImGui::Separator();
                ImGui::Combo("-> into backbuffer", &mWhatToCopyToBackbuffer, "[u] Uint64->color\0[h] Heat map\0[b] Uint64->color (but keep heat map enabled)\0[f] FB->color\0");
#else
				ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1.0), "Pipeline stats disabled project-wide.");
				ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1.0), "Heat map write disabled project-wide too (just fyi).");
#endif

				ImGui::Separator();

				if (mGatherPipelineStats) {
			        ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Parametric func. evaluations and pixel writes:");
				    std::array<std::string, 3> counterDescriptions = { {
				        "total #parametric function evaluations (lod + culling + rendering)",
				        "#unique pixels written across the screen (not counting overdraw)",
				        "#total pixels written (including overdraw)",
				    }};
			        for (int i = 0; i < counterDescriptions.size(); ++i) {
					    ImGui::Text(std::format("{:12L} {}", mCounterValues[i], counterDescriptions[i]).c_str());
				    }

					auto overdrawFactor = mCounterValues[1] == 0 ? 0 : mCounterValues[2] / mCounterValues[1];
					ImGui::TextColored(overdrawFactor < 10 ? ImVec4(0.6f, 1.0f, 0.6f, 1.0f) : overdrawFactor < 20 ? ImVec4(0.9f, 1.0f, 0.6f, 1.0f) : ImVec4(1.0f, 0.6f, 0.6f, 1.0f),
						std::format("{:12L}x overdraw factor", overdrawFactor).c_str());

					ImGui::Separator();
			        ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Patch stats:");
                    for (int i = 0; i < mPatchesCreatedPerLevel.size(); ++i) {
                        ImGui::Text(std::format("{:12L} patches created at LOD level #{}", mPatchesCreatedPerLevel[i], i).c_str());
					}
					ImGui::Separator();
					ImGui::Text(std::format("{:12L} pixel fill patches spawned[0]", mNumPxFillPatchesCreated[0]).c_str());
					ImGui::Text(std::format("{:12L} pixel fill patches spawned[1]", mNumPxFillPatchesCreated[1]).c_str());
					ImGui::Text(std::format("{:12L} pixel fill patches spawned[2]", mNumPxFillPatchesCreated[2]).c_str());
					ImGui::Text(std::format("{:12L} pixel fill patches spawned[3]", mNumPxFillPatchesCreated[3]).c_str());
					ImGui::Text(std::format("{:12L} pixel fill patches spawned[4]", mNumPxFillPatchesCreated[4]).c_str());
				}

				ImGui::End();


				// Do we need to recreate the vertex pipe?
				if (rasterPipesNeedRecreation) {
					auto newVertexPipe = create_vertex_pipe();
					std::swap(*mVertexPipeline, *newVertexPipe); // new pipe is now old pipe
					context().main_window()->handle_lifetime(std::move(newVertexPipe));

					{
						auto newFsQuadNoaaToMsPipe = create_noaa_to_ms_pipe();
						std::swap(*mFsQuadNoAAtoMSPipe, *newFsQuadNoaaToMsPipe);
						context().main_window()->handle_lifetime(std::move(newFsQuadNoaaToMsPipe));
					}

					{
						auto newFsQuadMsToSsPipe = create_ms_to_ssms_pipe();
						std::swap(*mFsQuadMStoSSPipe, *newFsQuadMsToSsPipe);
						context().main_window()->handle_lifetime(std::move(newFsQuadMsToSsPipe));
					}

					{
						auto newTessPipePxFill = create_tess_pipe(
							"shaders/px-fill-tess/patch_ready.vert", 
							"shaders/px-fill-tess/patch_set.tesc", 
							"shaders/px-fill-tess/patch_go.tese",
							push_constant_binding_data{shader_type::all, 0, sizeof(patch_into_tess_push_constants)},
							cfg::shade_per_fragment(),
							cfg::viewport_depth_scissors_config::from_framebuffer(mFramebufferNoAA.as_reference()),
							mRenderpassNoAA, cfg::subpass_index{ 0 }
						);
						std::swap(*mTessPipelinePxFillNoaa, *newTessPipePxFill);
						context().main_window()->handle_lifetime(std::move(newTessPipePxFill));

						auto newTessPipePxFillWire = context().create_graphics_pipeline_from_template(mTessPipelinePxFillNoaa.as_reference(), [](graphics_pipeline_t& p) {
							p.rasterization_state_create_info().setPolygonMode(vk::PolygonMode::eLine);
						});
						std::swap(*mTessPipelinePxFillNoaaWireframe, *newTessPipePxFillWire);
						context().main_window()->handle_lifetime(std::move(newTessPipePxFillWire));
					}

					{
						auto newTessPipePxFillSpS = create_tess_pipe(
							"shaders/px-fill-tess/patch_ready.vert", 
							"shaders/px-fill-tess/patch_set.tesc", 
							"shaders/px-fill-tess/patch_go.tese",
							push_constant_binding_data{shader_type::all, 0, sizeof(patch_into_tess_push_constants)},
							mEnableSampleShadingFor8xMS ? cfg::shade_per_sample() : cfg::shade_per_fragment(),
							cfg::viewport_depth_scissors_config::from_framebuffer(mFramebufferMS.as_reference()),
							mRenderpassMS, cfg::subpass_index{ 1 }
						);
						std::swap(*mTessPipelinePxFillMultisampled, *newTessPipePxFillSpS);
						context().main_window()->handle_lifetime(std::move(newTessPipePxFillSpS));

						auto newTessPipePxFillWireSpS = context().create_graphics_pipeline_from_template(mTessPipelinePxFillMultisampled.as_reference(), [](graphics_pipeline_t& p) {
							p.rasterization_state_create_info().setPolygonMode(vk::PolygonMode::eLine);
						});
						std::swap(*mTessPipelinePxFillMultisampledWireframe, *newTessPipePxFillWireSpS);
						context().main_window()->handle_lifetime(std::move(newTessPipePxFillWireSpS));
					}

					{
						auto newTessPipePxFillSuSa = create_tess_pipe(
							"shaders/px-fill-tess/patch_ready.vert", 
							"shaders/px-fill-tess/patch_set.tesc", 
							"shaders/px-fill-tess/patch_go.tese",
							push_constant_binding_data{shader_type::all, 0, sizeof(patch_into_tess_push_constants)},
							mEnableSampleShadingFor4xSS8xMS ? cfg::shade_per_sample() : cfg::shade_per_fragment(),
							cfg::viewport_depth_scissors_config::from_framebuffer(mFramebufferSSMS.as_reference()),
							mRenderpassSSMS, cfg::subpass_index{ 1 }
						);
						std::swap(*mTessPipelinePxFillSupersampled, *newTessPipePxFillSuSa);
						context().main_window()->handle_lifetime(std::move(newTessPipePxFillSuSa));

						auto newTessPipePxFillWireSuSa = context().create_graphics_pipeline_from_template(mTessPipelinePxFillSupersampled.as_reference(), [](graphics_pipeline_t& p) {
							p.rasterization_state_create_info().setPolygonMode(vk::PolygonMode::eLine);
						});
						std::swap(*mTessPipelinePxFillSupersampledWireframe, *newTessPipePxFillWireSuSa);
						context().main_window()->handle_lifetime(std::move(newTessPipePxFillWireSuSa));
					}

				}
			});
		}

#if STATS_ENABLED
		mTimestampPool = context().create_query_pool_for_timestamp_queries(
			static_cast<uint32_t>(context().main_window()->number_of_frames_in_flight()) * NUM_TIMESTAMP_QUERIES
		);

		mPipelineStatsPool = context().create_query_pool_for_pipeline_statistics_queries(
			    vk::QueryPipelineStatisticFlagBits::eClippingInvocations       |
			    vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations |
                vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations  ,
			context().main_window()->number_of_frames_in_flight()
		);
#endif
	}
	
	void update() override
	{
		using namespace avk;

		if (!mQuakeCam.is_enabled() && input().key_pressed(key_code::q)) {
			mQuakeCam.set_matrix(mOrbitCam.matrix());
			mQuakeCam.enable();
			mOrbitCam.disable();
        }

		// F6 ... toggle wireframe mode
		if (input().key_pressed(key_code::f1)) {
			mWireframeModeOn = !mWireframeModeOn;
		}

		if (input().key_pressed(key_code::f3)) {
			mWhatToCopyToBackbuffer = 0;
        }
		if (input().key_pressed(key_code::f4)) {
			mWhatToCopyToBackbuffer = 1;
        }
		if (input().key_pressed(key_code::f5)) {
			mWhatToCopyToBackbuffer = 2;
        }
		if (input().key_pressed(key_code::f6)) {
			mWhatToCopyToBackbuffer = 3;
        }

		if (avk::input().key_pressed(avk::key_code::i)) {
			LOG_INFO(std::format("O orbitCam pos: {:.5}f, {:.5}f, {:.5}f | distance from origin: {:.5}f", 
				mOrbitCam.translation().x, mOrbitCam.translation().y, mOrbitCam.translation().z, glm::length(mOrbitCam.translation())));
		}

        if (avk::input().key_pressed(avk::key_code::escape) && !mQuakeCam.is_enabled() && !mMeasurementInProgress || avk::context().main_window()->should_be_closed()) {
			// Stop the current composition:
			current_composition()->stop();
		}

		if (avk::input().key_pressed(avk::key_code::space)) {
			mStartMeasurement = true;
		}
#if TEST_DURATION_PER_STEP
		const float MeasureSecsPerStep = TEST_DURATION_PER_STEP;
#else
		const float MeasureSecsPerStep = 2.5f;
#endif
		if (mStartMeasurement) {
#if TEST_MODE_ON
			for (int ii = 0; ii < mParametricObjects.size(); ++ii) {
				if (ii == TEST_ENABLE_OBJ_IDX) {
					mParametricObjects[ii].set_enabled(true);
					mParametricObjects[ii].set_tessellation_levels({ static_cast<float>(TEST_INNER_TESS_LEVEL), static_cast<float>(TEST_OUTER_TESS_LEVEL) });
					mParametricObjects[ii].set_screen_distance_threshold(static_cast<float>(TEST_SCREEN_DISTANCE_THRESHOLD));
					mParametricObjects[ii].set_adaptive_rendering_on(1 == TEST_USEINDIVIDUAL_PATCH_RES);
					mParametricObjects[ii].set_how_to_render(TEST_RENDERING_METHOD);

					mParametricObjects[ii].set_transformation_matrix(glm::mat4{ 1.0f }); // TODO: Use an appropriate one

					LOG_INFO("The following settings were active during the test:");
					LOG_INFO(std::format(" - Rendering mode: '{}'", get_rendering_variant_description(mParametricObjects[ii].how_to_render())));
					LOG_INFO(std::format(" - Adaptive fill mode: {}", mParametricObjects[ii].adaptive_rendering_on() ? std::string("ON") : std::string("OFF")));
					switch (mParametricObjects[ii].how_to_render()) {
					case rendering_variant::PointRendered_4xSS_local_fb: 
						LOG_INFO(std::format(" - Tile size:     {}x{}", PX_FILL_LOCAL_FB_TILE_SIZE_X, PX_FILL_LOCAL_FB_TILE_SIZE_Y));
						LOG_INFO(std::format(" - Tile factor:   {}x{}", TILE_FACTOR_X, TILE_FACTOR_Y));
						LOG_INFO(std::format(" - Local FB size: {}x{}", LOCAL_FB_X, LOCAL_FB_Y));
						break;
					case rendering_variant::Tess_8xSS:
					case rendering_variant::Tess_4xSS_8xMS:
						LOG_INFO(std::format(" - SAMPLE_COUNT: {}", vk::to_string(SAMPLE_COUNT)));
					case rendering_variant::Tess_noAA:
						LOG_INFO(std::format(" - Inner Tessellation Level: {}", mParametricObjects[ii].tessellation_levels()[0]));
						LOG_INFO(std::format(" - Outer Tessellation Level: {}", mParametricObjects[ii].tessellation_levels()[1]));
						break;
					}
				}
				else {
					mParametricObjects[ii].set_enabled(false);
				}
			}
			fill_object_data_buffer();
			mRenderExtra3DModel = 1 == TEST_ENABLE_3D_MODEL;
			mGatherPipelineStats = 1 == TEST_GATHER_PIPELINE_STATS;
#endif
#if TEST_MODE_ON && !TEST_ALLOW_GATHER_STATS
			if(mGatherPipelineStats) {
				LOG_ERROR_EM("TEST_MODE_ON but also gathering pipeline stats despite TEST_ALLOW_GATHER_STATS not enabled.");
			}
#endif
#if TEST_MODE_ON && TEST_ALLOW_GATHER_STATS
			if(!mGatherPipelineStats) {
				LOG_WARNING_EM("TEST_MODE_ON and TEST_ALLOW_GATHER_STATS enabled, but not gathering pipeline stats.");
			}
#endif
			mStartMeasurement = false;
			mMeasurementFrameCounters.clear();
			mMeasurementMoveCameraSteps = glm::max(mMeasurementMoveCameraSteps, 0);
			mMeasurementFrameCounters.resize(mMeasurementMoveCameraSteps + 1, std::make_tuple(0.0, 0.0f, 0, 0, 0));
#if TEST_GATHER_PATCH_COUNTS
			mMeasurementPatchCounts.clear();
			mMeasurementPatchCounts.resize(mMeasurementMoveCameraSteps + 1);
			for (auto& a : mMeasurementPatchCounts) {
				for (int b = 0; b < MAX_PATCH_SUBDIV_STEPS; ++b) {
					a[b] = 0u;
				}
			}
#endif
			//auto imguiManager = current_composition()->element_by_type<imgui_manager>();
			//imguiManager->disable();
			mMeasurementInProgress = true;
			auto curTime = time().absolute_time_dp();
			mMeasurementStartTime = curTime;
			mMeasurementEndTime = curTime + MeasureSecsPerStep * (mMoveCameraDuringMeasurements ? static_cast<double>(mMeasurementMoveCameraSteps + 1) : 1.0);
			std::get<double>(mMeasurementFrameCounters[0]) = curTime;
			std::get<float>(mMeasurementFrameCounters[0]) = mDistanceFromOrigin;
			mMeasurementIndexLastFrame = 0;
		}
		if (mMeasurementInProgress) {
			auto curTime = time().absolute_time_dp();
			if (avk::input().key_pressed(avk::key_code::escape)) {
				mMeasurementEndTime = curTime;
			}
		    if (curTime >= mMeasurementEndTime) {
				std::get<double>(mMeasurementFrameCounters.back()) = curTime;
			    //auto imguiManager = current_composition()->element_by_type<imgui_manager>();
			    //imguiManager->enable();
			    mMeasurementInProgress = false;
				LOG_INFO_EM(std::format("{} frames rendered during {} sec. measurement time frame.", std::get<int>(mMeasurementFrameCounters.back()), mMeasurementEndTime - mMeasurementStartTime));
#if STATS_ENABLED
				LOG_INFO(std::format(" - mGatherPipelineStats: {}", mGatherPipelineStats));
#else
				LOG_INFO(            " - mGatherPipelineStats not included in build (!STATS_ENABLED)");
#endif

				for (int i = mMeasurementFrameCounters.size() - 1; i > 0; --i) {
					std::get<double>(mMeasurementFrameCounters[i]) -= std::get<double>(mMeasurementFrameCounters[i - 1]);
				}
				std::get<double>(mMeasurementFrameCounters[0]) -= mMeasurementStartTime;
#if TEST_GATHER_TIMER_QUERIES && STATS_ENABLED
				LOG_INFO("Measurement results (elapsed time, camera distance, avg. frame duration, avg. LOD stage duration, ratio LOD/frame (%), FPS):");
				for (auto [elapsedTime, dist, frameMs, patchLodMs, cnt] : mMeasurementFrameCounters) {
					if (cnt != 0 && elapsedTime != 0.0) {
						auto avgFrameMs = frameMs / cnt;
						auto avgPatchLodMs = patchLodMs / cnt;
						LOG_INFO(std::format("({:5.2f}, \t{:5.1f}, \t{:12}, \t{:12}, \t{:7.1f} \t{:7.2f}", 
							elapsedTime, dist, avgFrameMs, avgPatchLodMs, static_cast<double>(avgPatchLodMs)/static_cast<double>(avgFrameMs) * 100.0, cnt / elapsedTime));
					}
					else {
						LOG_WARNING(std::format("Not printing measurement results because a denominator is 0. cnt[{}], elapsedTime[{}]", cnt, elapsedTime));
					}
			    }
#else
				LOG_INFO("Measurement results (elapsed time, camera distance, unique pixels (avg.), num patches out to render (avg.), FPS):");
			    for (auto [elapsedTime, dist, numGlyphs, numPatches, cnt] : mMeasurementFrameCounters) {
					if (cnt != 0 && elapsedTime != 0.0) {
						LOG_INFO(std::format("({:5.2f}, \t{:5.1f}, \t{:12}, \t{:12}, \t{:7.2f}", elapsedTime, dist, numGlyphs / cnt, numPatches / cnt, cnt / elapsedTime));
					}
					else {
						LOG_WARNING(std::format("Not printing measurement results because a denominator is 0. cnt[{}], elapsedTime[{}]", cnt, elapsedTime));
					}
			    }
#endif 
#if TEST_GATHER_PATCH_COUNTS
				LOG_INFO_EM("Patch counts:");
				for (int a = 0; a < mMeasurementPatchCounts.size(); ++a) {
					LOG_INFO(std::format("Measurement #{}", a));
					for (int b = 0; b < MAX_PATCH_SUBDIV_STEPS; ++b) {
						LOG_INFO(std::format("    {:12}", mMeasurementPatchCounts[a][b] / static_cast<uint32_t>(std::get<int>(mMeasurementFrameCounters[a]))));
					}
				}
#endif
			}
            else {
				// FLY & MEASURE!
				auto curIdx = static_cast<int>((time().absolute_time_dp() - mMeasurementStartTime)/(mMeasurementEndTime - mMeasurementStartTime) * (mMeasurementMoveCameraSteps + 1));
				auto curTime = time().absolute_time_dp();
				if (curIdx != mMeasurementIndexLastFrame) {
					std::get<double>(mMeasurementFrameCounters[curIdx-1]) = curTime;
					std::get<float>(mMeasurementFrameCounters[curIdx]) = glm::length(glm::vec3{ mOrbitCam.translation().x, 0.0f, mOrbitCam.translation().z });
				}
#if TEST_GATHER_TIMER_QUERIES && STATS_ENABLED
				std::get<2>(mMeasurementFrameCounters[curIdx]) += mLastTotalRenderDuration;
				std::get<3>(mMeasurementFrameCounters[curIdx]) += mLastLodStageDuration;
#else
				std::get<2>(mMeasurementFrameCounters[curIdx]) += mCounterValues[1];
				std::get<3>(mMeasurementFrameCounters[curIdx]) += mNumPxFillPatchesCreated[0] + mNumPxFillPatchesCreated[1] + mNumPxFillPatchesCreated[2] + mNumPxFillPatchesCreated[3] + mNumPxFillPatchesCreated[4];
#endif 
#if TEST_GATHER_PATCH_COUNTS
				for (int b = 0; b < MAX_PATCH_SUBDIV_STEPS; ++b) {
					mMeasurementPatchCounts[curIdx][b] += mPatchesCreatedPerLevel[b];
				}
#endif
				std::get<int>(mMeasurementFrameCounters[curIdx]) += 1;
            	auto f = static_cast<float>((curTime - curIdx * MeasureSecsPerStep - mMeasurementStartTime) / MeasureSecsPerStep);

#if TEST_MODE_ON
            	float camCoords = TEST_CAMDIST * glm::pow(TEST_CAMERA_DELTA_POW, static_cast<float>(curIdx));
				bool translateY = TEST_TRANSLATE_Y;
				bool translateZ = TEST_TRANSLATE_Z;
				auto camPos = glm::angleAxis(f * glm::two_pi<float>(), avk::up())
            						* glm::vec3{ 0.0f, translateY ? camCoords : TEST_CAM_Y_SHIFT, translateZ ? camCoords : 0.0f };
				//auto camPos = glm::vec3{ 0.0f, translateY ? camCoords : TEST_CAM_Y_SHIFT, translateZ ? camCoords : 0.0f };
#else
				auto curDist = (mDistanceFromOrigin + mMeasurementMoveCameraDelta * curIdx);
				auto camPos = glm::angleAxis(f * glm::two_pi<float>(), up()) * glm::vec3{ 0.0f, 4.0f, 20.0f };
				camPos *= curDist / glm::length(camPos);
#endif

				mOrbitCam.set_translation(camPos);
				mOrbitCam.set_pivot_distance(glm::length(camPos));
#if TEST_MODE_ON
				mOrbitCam.look_at(glm::vec3{0.0f, TEST_CAM_Y_SHIFT, 0.0f});
#else
				mOrbitCam.look_at(glm::vec3{0.0f});
#endif
				mMeasurementIndexLastFrame = curIdx;
			}
		}
	}

	std::tuple<std::vector<avk::recorded_commands_t>, avk::buffer*, avk::buffer*> get_lod_stage_commands()
	{
		using namespace avk;
		auto mainWnd = context().main_window();
		auto inFlightIndex = mainWnd->current_in_flight_index();

		std::vector<recorded_commands_t> pass2xCommands;
        pass2xCommands = command::gather(
            command::bind_pipeline(mPatchLodComputePipe.as_reference()),
            command::bind_descriptors(mPatchLodComputePipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
	            descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]), 
	            descriptor_binding(1, 0, mCountersSsbo->as_storage_buffer()),
	            descriptor_binding(2, 0, mObjectDataBuffer->as_storage_buffer()),
	            descriptor_binding(2, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
	            descriptor_binding(2, 2, mIndirectPxFillCountBuffer->as_storage_buffer())
            }))
		);
		buffer* firstPing = nullptr;
		buffer* finalPong = nullptr;
        for (uint32_t lodLevel = 0; lodLevel < MAX_PATCH_SUBDIV_STEPS; ++lodLevel) {
			// ping-pong:
			bool evenOdd = (lodLevel & 0x1) == 0x0;
			buffer* ping = evenOdd ? &mPatchLodBufferPing : &mPatchLodBufferPong;
			buffer* pong = evenOdd ? &mPatchLodBufferPong : &mPatchLodBufferPing;

			if (nullptr == firstPing) {
				firstPing = ping;
            }

			auto moreCommands = command::gather(
                command::bind_descriptors(mPatchLodComputePipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
	                descriptor_binding(3, 0, (*ping)->as_storage_buffer()),
	                descriptor_binding(3, 1, (*pong)->as_storage_buffer()),
	                descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer()),
					descriptor_binding(4, 0, mBigDataset->as_storage_buffer())
#if DRAW_PATCH_EVAL_DEBUG_VIS
					, descriptor_binding(5, 0, mCombinedAttachmentView->as_storage_image(layout::general))
#if STATS_ENABLED
					, descriptor_binding(5, 1, mHeatMapImageView->as_storage_image(layout::general))
#endif
#endif
                })),

				command::push_constants(mPatchLodComputePipe->layout(), pass2x_push_constants{ 
					mGatherPipelineStats ? VK_TRUE : VK_FALSE,
				    lodLevel,
					mFrustumCullingOn ? VK_TRUE : VK_FALSE
				}),

                command::dispatch_indirect(mPatchLodCountBuffer, sizeof(glm::uvec4) * lodLevel),

                sync::global_memory_barrier(stage::compute_shader + access::memory_write >> stage::compute_shader + access::memory_read)
			);
			add_commands(pass2xCommands, moreCommands);

			finalPong = pong;
        }
		assert(nullptr != firstPing);
		assert(nullptr != finalPong);
		assert((MAX_PATCH_SUBDIV_STEPS %2 == 0 && firstPing == finalPong) || (MAX_PATCH_SUBDIV_STEPS %2 == 1 && firstPing != finalPong));

		return std::forward_as_tuple(std::move(pass2xCommands), firstPing, finalPong);
	}

	std::vector<avk::recorded_commands_t> get_init_commands(const avk::buffer* firstPing, const avk::buffer* finalPong) // TODO: I think we can clean this up further, right? => delete firstPing and finalPong?!
	{
		using namespace avk;
		auto mainWnd = context().main_window();
		auto inFlightIndex = mainWnd->current_in_flight_index();

		return command::gather(
			// Initialize:
			command::bind_pipeline(mInitPatchesComputePipe.as_reference()),
			command::bind_descriptors(mInitPatchesComputePipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
				descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]), 
			    descriptor_binding(1, 0, mCountersSsbo->as_storage_buffer()),
				descriptor_binding(2, 0, mObjectDataBuffer->as_storage_buffer()),
				descriptor_binding(2, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
				descriptor_binding(2, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
	            descriptor_binding(3, 0, (*firstPing)->as_storage_buffer()),
	            descriptor_binding(3, 1, (*finalPong)->as_storage_buffer()),
	            descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer())
			})),
				
			// 1st pass compute shader: density estimation PER OBJECT
			command::dispatch(mNumEnabledObjects, 1u, 1u),
			
			// Initialize knit yarn (they need special treatment):
			command::many_for_each(mKnitYarnObjectDataIndices, [&, this] (auto kyIndex) {
				return command::gather(
					command::bind_pipeline(mInitKnitYarnComputePipe.as_reference()),
					command::bind_descriptors(mInitKnitYarnComputePipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]), 
						descriptor_binding(1, 0, mCountersSsbo->as_storage_buffer()),
						descriptor_binding(2, 0, mObjectDataBuffer->as_storage_buffer()),
						descriptor_binding(2, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
						descriptor_binding(2, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
						descriptor_binding(3, 0, (*firstPing)->as_storage_buffer()),
						descriptor_binding(3, 1, (*finalPong)->as_storage_buffer()),
						descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer())
					})),
					command::push_constants(mInitKnitYarnComputePipe->layout(), uint32_t{ kyIndex }),
					// We "abuse" the "to"-parameters for the dispatch dimensions:
					command::dispatch(
						(glm::clamp(static_cast<uint32_t>(mObjectData[kyIndex].mParams[0]), 1u, 1000u) + 15u) / 16u /* <-- u to */, 
						(glm::clamp(static_cast<uint32_t>(mObjectData[kyIndex].mParams[1]), 1u, 1000u) + 15u) / 16u /* <-- v to */, 
						1u
					)
    			);
			}),

			// Initialize brain scan SH glyphs (they need special treatment):
			command::many_for_each(mShBrainObjectDataIndices, [&, this] (auto shBrainIndex) {
				return command::gather(
					command::bind_pipeline(mInitShBrainComputePipe.as_reference()),
					command::bind_descriptors(mInitShBrainComputePipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]), 
						descriptor_binding(1, 0, mCountersSsbo->as_storage_buffer()),
						descriptor_binding(2, 0, mObjectDataBuffer->as_storage_buffer()),
						descriptor_binding(2, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
						descriptor_binding(2, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
						descriptor_binding(3, 0, (*firstPing)->as_storage_buffer()),
						descriptor_binding(3, 1, (*finalPong)->as_storage_buffer()),
						descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer())
					})),
					command::push_constants(mInitShBrainComputePipe->layout(), uint32_t{ shBrainIndex }),
					command::dispatch( 
						mObjectData[shBrainIndex].mDetailEvalDims[2] * mObjectData[shBrainIndex].mDetailEvalDims[3], 
						1u, 
						1u
					)
    			);
			})

		);
	}

	void render() override
	{
		using namespace avk;
		std::chrono::steady_clock::time_point renderBegin = std::chrono::steady_clock::now();

		auto mainWnd = context().main_window();
		//auto resolution = mainWnd->resolution();
		auto resolution = glm::uvec2(mFramebufferNoAA->image_at(0).width(), mFramebufferNoAA->image_at(0).height());
		auto inFlightIndex = mainWnd->current_in_flight_index();

		glm::mat4 projMat = mQuakeCam.is_enabled() ? mQuakeCam.projection_matrix() : mOrbitCam.projection_matrix();

		frame_data_ubo uboData;
		if (mQuakeCam.is_enabled()) {
            uboData.mViewMatrix        = mQuakeCam.view_matrix();
            uboData.mViewProjMatrix    = mQuakeCam.projection_and_view_matrix();
			uboData.mInverseProjMatrix = glm::inverse(mQuakeCam.projection_matrix());
			uboData.mCameraTransform   = mQuakeCam.global_transformation_matrix();
		}
        else {
            uboData.mViewMatrix        = mOrbitCam.view_matrix();
            uboData.mViewProjMatrix    = mOrbitCam.projection_and_view_matrix();
			uboData.mInverseProjMatrix = glm::inverse(mOrbitCam.projection_matrix());
			uboData.mCameraTransform   = mQuakeCam.global_transformation_matrix();
        }
        uboData.mResolutionAndCulling                      = glm::uvec4(resolution, mBackfaceCullingOn ? 1u : 0u, 0u);
        uboData.mDebugSliders                              = mDebugSliders;
        uboData.mDebugSlidersi                             = mDebugSlidersi;
		uboData.mHeatMapEnabled                            = mWhatToCopyToBackbuffer == 1 || mWhatToCopyToBackbuffer == 2 ? VK_TRUE : VK_FALSE;
		uboData.mWriteToCombinedAttachmentInFragmentShader = 3 != mWhatToCopyToBackbuffer ? VK_TRUE : VK_FALSE;
        uboData.mGatherPipelineStats                       = mGatherPipelineStats;
        uboData.mAbsoluteTime                              = (mAnimationPaused ? mAnimationPauseTime : time().absolute_time()) - mTimeToSubtract;
        uboData.mDeltaTime                                 = time().delta_time();
		// Update in host-coherent buffer:
        auto emptyCmd = mFrameDataBuffers[inFlightIndex]->fill(&uboData, 0);
		
		// Get a command pool to allocate command buffers from:
		auto& commandPool = context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

#if STATS_ENABLED
		const auto firstQueryIndex = static_cast<uint32_t>(inFlightIndex) * NUM_TIMESTAMP_QUERIES;
		if (mainWnd->current_frame() >= mainWnd->number_of_frames_in_flight()) // otherwise we will wait forever
		{
			auto timers = mTimestampPool->get_results<uint64_t, NUM_TIMESTAMP_QUERIES>(
				firstQueryIndex, NUM_TIMESTAMP_QUERIES, vk::QueryResultFlagBits::eWait // => ensure that the results are available
			);

			auto get_duration = [&timers](int index) {
				return timers[index] - timers[index-1];
			};

			mLastClearDuration					= get_duration(1); // + 1, stage::compute_shader), // measure after clearing
			mLastInitDuration					= get_duration(2); // + 2, stage::compute_shader), // measure after init
			mLastLodStageDuration				= get_duration(3); // + 3, stage::compute_shader), // measure after LOD stage
			mLast3dModelsRenderDuration			= get_duration(4); // + 4, stage::color_attachment_output), // measure after rendering sponza
			mLastTessNoaaDuration				= get_duration(5); // + 5, stage::color_attachment_output), // measure after Tess. noAA
			mLastCopyToCombinedAttachmentDur	= get_duration(6); // + 6, stage::compute_shader), // measure after copy-over to combined attachment
			mLastPatchToTileStepDuration		= get_duration(7); // + 7, stage::compute_shader), // measure after patch to tile step
			mLastPointRenderingNoaaDuration		= get_duration(8); // + 8, stage::compute_shader), // measure after px fill (the version with the holes ^^)
			mLastPointRenderingLocalFbDur		= get_duration(9); // + 9, stage::compute_shader), // measure after point rendering into local fb
			mLastFSQbefore8xSSDuration			= get_duration(10); // + 10, stage::color_attachment_output), // measure after copy-over combined attachment back into a framebuffer image
			mLastTess8xSSDuration				= get_duration(11); // + 11, stage::color_attachment_output), // measure after Tess. 8xSS
			mLastFSQbefore4xSS8xMSDuration		= get_duration(12); // + 12, stage::color_attachment_output), // measure after copy-over combined attachment back into a framebuffer image
			mLastTess4xSS8xMSDuration			= get_duration(13); // + 13, stage::color_attachment_output), // measure after rendering with Tess. 4xSS+8xMS
			mLastTotalRenderDuration			= timers[13] - timers[4];
			mLastTotalStepsDuration				= timers[13] - timers[0];

			if (mGatherPipelineStats) {
                mPipelineStats = mPipelineStatsPool->get_results<uint64_t, 3>(inFlightIndex, 1, vk::QueryResultFlagBits::e64);
            }
		}

		// Read the counter values:
		if (mGatherPipelineStats) {
			std::array<VkDrawIndirectCommand, NUM_DIFFERENT_RENDER_VARIANTS> pxFillCountBufferContents;
			std::array<glm::uvec4, MAX_PATCH_SUBDIV_STEPS> patchLodCountBufferContents;
			context().record_and_submit_with_fence({
				mCountersSsbo->read_into(mCounterValues.data(), 0),
		        mIndirectPxFillCountBuffer->read_into(pxFillCountBufferContents.data(), 0),
			    mPatchLodCountBuffer->read_into(patchLodCountBufferContents.data(), 0)
			}, *mQueue)->wait_until_signalled();
			mNumPxFillPatchesCreated[0] = pxFillCountBufferContents[0].instanceCount;
			mNumPxFillPatchesCreated[1] = pxFillCountBufferContents[1].instanceCount;
			mNumPxFillPatchesCreated[2] = pxFillCountBufferContents[2].instanceCount;
			mNumPxFillPatchesCreated[3] = pxFillCountBufferContents[3].instanceCount;
			mNumPxFillPatchesCreated[4] = pxFillCountBufferContents[4].instanceCount;
			for (int i = 0; i < patchLodCountBufferContents.size(); ++i) {
			    mPatchesCreatedPerLevel[i] = patchLodCountBufferContents[i].x;
			}
		}

		std::vector<recorded_commands_t> commandsBeginStats;
		std::vector<recorded_commands_t> commandsEndStats;
		if (mGatherPipelineStats)
		{
		    commandsBeginStats = command::gather(
				mPipelineStatsPool->reset(inFlightIndex, 1),
				mPipelineStatsPool->begin_query(inFlightIndex)
			);
			commandsEndStats = command::gather(
			    mPipelineStatsPool->end_query(inFlightIndex)
			);
		}
#endif

		// Perform the LOD stage:
		auto [lodStageCommands, firstPing, finalPong] = get_lod_stage_commands();

		auto initCommands = get_init_commands(firstPing, finalPong);

		// Now render everything:
		graphics_pipeline& tessPipePxFillNoaaToBeUsed           = mWireframeModeOn ? mTessPipelinePxFillNoaaWireframe : mTessPipelinePxFillNoaa;
		graphics_pipeline& tessPipePxFillMultisampledToBeUsed   = mWireframeModeOn ? mTessPipelinePxFillMultisampledWireframe : mTessPipelinePxFillMultisampled;
		graphics_pipeline& tessPipePxFillSupersampledToBeUsed   = mWireframeModeOn ? mTessPipelinePxFillSupersampledWireframe : mTessPipelinePxFillSupersampled;
		auto gatheredCommands = command::gather(
#if STATS_ENABLED
			    commandsBeginStats,
				mTimestampPool->reset(firstQueryIndex, NUM_TIMESTAMP_QUERIES), // reset the four values relevant for the current frame in flight
				mTimestampPool->write_timestamp(firstQueryIndex + 0, stage::all_commands), // measure before everything starts
#endif

				command::bind_pipeline(mClearCombinedAttachmentPipe.as_reference()),
				command::bind_descriptors(mClearCombinedAttachmentPipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
			        descriptor_binding(0, 0, mCountersSsbo->as_storage_buffer()),
			        descriptor_binding(1, 0, mObjectDataBuffer->as_storage_buffer()),
			        descriptor_binding(1, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
			        descriptor_binding(1, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
                    descriptor_binding(2, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
                    descriptor_binding(2, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
                    descriptor_binding(3, 0, mPatchLodBufferPing->as_storage_buffer()),
                    descriptor_binding(3, 1, mPatchLodBufferPong->as_storage_buffer()),
                    descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer())
#if SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
					, descriptor_binding(4, 0, mTilePatchesBuffer->as_storage_buffer())
#endif
				})),
				command::dispatch((resolution.x + 15u) / 16u, (resolution.y + 15u) / 16u, 1u),

		        sync::global_memory_barrier(stage::all_commands >> stage::all_commands, access::memory_write >> access::memory_write | access::memory_read),
#if STATS_ENABLED
				mTimestampPool->write_timestamp(firstQueryIndex + 1, stage::compute_shader), // measure after clearing
#endif
				
			// 1) Initialize:
			initCommands,

			sync::global_memory_barrier(stage::compute_shader + access::memory_write >> stage::compute_shader + access::memory_read),
#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 2, stage::compute_shader), // measure after init
#endif
				
			// 2) LOD Stage:
            lodStageCommands,
		
            sync::global_memory_barrier(stage::compute_shader + access::memory_write >> stage::all_graphics + access::memory_read),
#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 3, stage::compute_shader), // measure after LOD stage
#endif

			// 3) Render patches:

			command::render_pass(mRenderpassNoAA.as_reference(), mFramebufferNoAA.as_reference(), command::gather(

				// 3.1) Render extra 3D models (noAA):
				command::conditional(
					[this]() { return mRenderExtra3DModel; },
					[this, inFlightIndex]() { return command::gather(
						command::bind_pipeline(mVertexPipeline.as_reference()),
						command::bind_descriptors(mVertexPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
							descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]),
							descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
							descriptor_binding(0, 2, mMaterialBuffer),
							descriptor_binding(1, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
							descriptor_binding(1, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
							descriptor_binding(2, 0, mCountersSsbo->as_storage_buffer())
						})),
						command::many_n_times(static_cast<int>(mDrawCalls.size()), [this](int i) {
							return command::gather(
								command::push_constants(mVertexPipeline->layout(), vertex_pipe_push_constants{ 
									mDrawCalls[i].mModelMatrix,
									mDrawCalls[i].mMaterialIndex
								}),
								command::draw_indexed(
									// Bind and use the index buffer:
									std::forward_as_tuple(mIndexBuffer.as_reference(), size_t{mDrawCalls[i].mIndexBufferOffset}, mDrawCalls[i].mNumElements),
									// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
									std::forward_as_tuple(mPositionsBuffer.as_reference(), size_t{mDrawCalls[i].mPositionsBufferOffset}), 
									std::forward_as_tuple(mTexCoordsBuffer.as_reference(), size_t{mDrawCalls[i].mTexCoordsBufferOffset}),
									std::forward_as_tuple(mNormalsBuffer.as_reference()  , size_t{mDrawCalls[i].mNormalsBufferOffset})
								)
							);
						} )
					); }
				),

#if STATS_ENABLED
				mTimestampPool->write_timestamp(firstQueryIndex + 4, stage::color_attachment_output), // measure after rendering sponza
#endif
				// 3.2) Render tessellated patches with noAA:
                command::bind_pipeline(tessPipePxFillNoaaToBeUsed.as_reference()),
				command::bind_descriptors(tessPipePxFillNoaaToBeUsed->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]),
			        descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			        descriptor_binding(0, 2, mMaterialBuffer),
					descriptor_binding(1, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
					descriptor_binding(1, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
			        descriptor_binding(2, 0, mCountersSsbo->as_storage_buffer()),
				    descriptor_binding(3, 0, mObjectDataBuffer->as_storage_buffer()),
					descriptor_binding(3, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
				    descriptor_binding(3, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
					descriptor_binding(4, 0, mBigDataset->as_storage_buffer())
				})),
					
				command::push_constants(tessPipePxFillNoaaToBeUsed->layout(), patch_into_tess_push_constants{ 
					get_rendering_variant_index(rendering_variant::Tess_noAA) * MAX_INDIRECT_DISPATCHES 
				}),
				command::draw_vertices_indirect(
					mIndirectPxFillCountBuffer.as_reference(), 
					get_rendering_variant_index(rendering_variant::Tess_noAA) * sizeof(VkDrawIndirectCommand), 
					sizeof(VkDrawIndirectCommand), 
					1u) // <-- Exactly ONE draw (but potentially a lot of instances), use the one at [0]
				
			)),

#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 5, stage::color_attachment_output), // measure after Tess. noAA
#endif

			// 3.3) Copy whatever was rendered in noAA-fashion so far into combined attachment, s.t. point rendering can be added:

			sync::global_memory_barrier(
				stage::early_fragment_tests | stage::late_fragment_tests | stage::color_attachment_output  >>  stage::compute_shader,
				access::depth_stencil_attachment_write                   | access::color_attachment_write  >>  access::shader_read
			), 

			command::conditional([this] { return mRenderVariantDataTransferEnabled[0]; }, [this, resolution] { return command::gather(
				command::bind_pipeline(mCopyToCombinedAttachmentPipe.as_reference()),
				command::bind_descriptors(mCopyToCombinedAttachmentPipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					descriptor_binding(0, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
					descriptor_binding(1, 0, mFramebufferNoAA->image_view_at(0)->as_sampled_image(layout::shader_read_only_optimal)),
					descriptor_binding(1, 1, mFramebufferNoAA->image_view_at(1)->as_sampled_image(layout::shader_read_only_optimal))
					})),
				command::dispatch((resolution.x + 15u) / 16u, (resolution.y + 15u) / 16u, 1u),

				sync::global_memory_barrier(stage::compute_shader + access::shader_write >> stage::compute_shader + (access::shader_read | access::shader_write))
			); }),
				
#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 6, stage::compute_shader), // measure after copy-over to combined attachment
#endif

			// => Perform point rendering into combined attachment 
#if SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
			// Inject another intermediate pass:
			command::bind_pipeline(mSelectTilePatchesPipe.as_reference()),
			command::bind_descriptors(mSelectTilePatchesPipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
				descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]), 
			    descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			    descriptor_binding(0, 2, mMaterialBuffer),
			    descriptor_binding(1, 0, mCountersSsbo->as_storage_buffer()),
				descriptor_binding(2, 0, mObjectDataBuffer->as_storage_buffer()),
				descriptor_binding(2, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
				descriptor_binding(2, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
                descriptor_binding(3, 0, mCombinedAttachmentView->as_storage_image(layout::general))
#if STATS_ENABLED
                , descriptor_binding(3, 1, mHeatMapImageView->as_storage_image(layout::general))
#endif
				, descriptor_binding(4, 0, mTilePatchesBuffer->as_storage_buffer())
			})),

			command::dispatch(128u /* <--- TODO: num tiles / 32 */, 1u, 1u),

			sync::global_memory_barrier(stage::compute_shader + access::memory_write >> stage::compute_shader + access::memory_read),
#endif
#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 7, stage::compute_shader), // measure after patch to tile step
#endif

			// 3.4) Pixel fill directly into combined attachment ~1spp
            command::bind_pipeline(mPxFillComputePipe.as_reference()),
			command::bind_descriptors(mPxFillComputePipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
				descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]), 
			    descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			    descriptor_binding(0, 2, mMaterialBuffer),
			    descriptor_binding(1, 0, mCountersSsbo->as_storage_buffer()),
				descriptor_binding(2, 0, mObjectDataBuffer->as_storage_buffer()),
				descriptor_binding(2, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
				descriptor_binding(2, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
                descriptor_binding(3, 0, mCombinedAttachmentView->as_storage_image(layout::general))
#if STATS_ENABLED
                , descriptor_binding(3, 1, mHeatMapImageView->as_storage_image(layout::general))
#endif
				, descriptor_binding(4, 0, mBigDataset->as_storage_buffer())
			})),
			command::push_constants(mPxFillComputePipe->layout(), pass3_push_constants{
				mGatherPipelineStats ? VK_TRUE : VK_FALSE
			}),
			command::dispatch_indirect(
				mIndirectPxFillCountBuffer, 
				get_rendering_variant_index(rendering_variant::PointRendered_direct) * sizeof(VkDrawIndirectCommand) 
				/* offset to the right struct member: */  + sizeof(VkDrawIndirectCommand::vertexCount)
			), // => in order to use the instanceCount!

#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 8, stage::compute_shader), // measure after px fill (the version with the holes ^^)
#endif

			// 3.5) Pixel fill into local framebuffers (~4spp) then into combined attachment
            command::bind_pipeline(mPxFillLocalFbComputePipe.as_reference()),
			command::bind_descriptors(mPxFillLocalFbComputePipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
				descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]), 
			    descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			    descriptor_binding(0, 2, mMaterialBuffer),
			    descriptor_binding(1, 0, mCountersSsbo->as_storage_buffer()),
				descriptor_binding(2, 0, mObjectDataBuffer->as_storage_buffer()),
				descriptor_binding(2, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
				descriptor_binding(2, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
                descriptor_binding(3, 0, mCombinedAttachmentView->as_storage_image(layout::general))
#if STATS_ENABLED
                , descriptor_binding(3, 1, mHeatMapImageView->as_storage_image(layout::general))
#endif
				, descriptor_binding(4, 0, mBigDataset->as_storage_buffer())
#if SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
				, descriptor_binding(5, 0, mTilePatchesBuffer->as_storage_buffer())
#endif
			})),
			command::push_constants(mPxFillLocalFbComputePipe->layout(), pass3_push_constants{
				mGatherPipelineStats ? VK_TRUE : VK_FALSE
			}),
			command::dispatch((resolution.x + PX_FILL_LOCAL_FB_TILE_SIZE_X - 1) / PX_FILL_LOCAL_FB_TILE_SIZE_X, (resolution.y + PX_FILL_LOCAL_FB_TILE_SIZE_Y - 1) / PX_FILL_LOCAL_FB_TILE_SIZE_Y, 1),

#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 9, stage::compute_shader), // measure after point rendering into local fb
#endif

			sync::global_memory_barrier(stage::compute_shader + access::shader_write >> stage::fragment_shader + access::shader_read),

			command::render_pass(mRenderpassMS.as_reference(), mFramebufferMS.as_reference(), command::gather(

				command::conditional([this] { return mRenderVariantDataTransferEnabled[1]; }, [this] { return command::gather(
					// 3.6) Full-screen quad noAA->MS
					command::bind_pipeline(mFsQuadNoAAtoMSPipe.as_reference()),
					command::push_constants(mFsQuadNoAAtoMSPipe->layout(), copy_to_backbuffer_push_constants{
						mWhatToCopyToBackbuffer > 1 ? 0 : mWhatToCopyToBackbuffer // 0 => 0, 1 => 1, 2 => 0
						}),
					command::bind_descriptors(mFsQuadNoAAtoMSPipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						descriptor_binding(0, 0, mFsQuadColorSampler),
						descriptor_binding(0, 1, mFsQuadDepthSampler),
						descriptor_binding(1, 0, mCombinedAttachmentView->as_storage_image(layout::general))
#if STATS_ENABLED
						, descriptor_binding(1, 1, mHeatMapImageView->as_storage_image(layout::general))
#endif
						})),
					// Draw a a full-screen quad:
					command::draw(6, 1, 0, 1)
				); }),
				
				command::next_subpass(),

#if STATS_ENABLED
				mTimestampPool->write_timestamp(firstQueryIndex + 10, stage::color_attachment_output), // measure after copy-over combined attachment back into a framebuffer image
#endif

				// 3.7) Render tessellated patches with 8xMS + sample shading => i.e., actually this is 8xSS
                command::bind_pipeline(tessPipePxFillMultisampledToBeUsed.as_reference()),
				command::bind_descriptors(tessPipePxFillMultisampledToBeUsed->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]),
			        descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			        descriptor_binding(0, 2, mMaterialBuffer),
					descriptor_binding(1, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
					descriptor_binding(1, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
			        descriptor_binding(2, 0, mCountersSsbo->as_storage_buffer()),
				    descriptor_binding(3, 0, mObjectDataBuffer->as_storage_buffer()),
					descriptor_binding(3, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
				    descriptor_binding(3, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
					descriptor_binding(4, 0, mBigDataset->as_storage_buffer())
				})),
					
				command::push_constants(tessPipePxFillMultisampledToBeUsed->layout(), patch_into_tess_push_constants{ 
					get_rendering_variant_index(rendering_variant::Tess_8xMS) * MAX_INDIRECT_DISPATCHES 
				}),
				command::draw_vertices_indirect(
					mIndirectPxFillCountBuffer.as_reference(), 
					get_rendering_variant_index(rendering_variant::Tess_8xMS) * sizeof(VkDrawIndirectCommand), 
					sizeof(VkDrawIndirectCommand), 
					1u) // <-- Exactly ONE draw (but potentially a lot of instances), use the one at [1]

			)),


#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 11, stage::color_attachment_output), // measure after Tess. 8xSS
#endif

#if SSAA_ENABLED
			command::render_pass(mRenderpassSSMS.as_reference(), mFramebufferSSMS.as_reference(), command::gather(

				command::conditional([this] { return mRenderVariantDataTransferEnabled[2]; }, [this] { return command::gather(
					// 3.8) Full-screen quad MS->SSMS
					command::bind_pipeline(mFsQuadMStoSSPipe.as_reference()),
					command::bind_descriptors(mFsQuadMStoSSPipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						descriptor_binding(0, 0, mFsQuadColorSampler),
						descriptor_binding(0, 1, mFsQuadDepthSampler),
						descriptor_binding(0, 2, mFramebufferMS->image_view_at(0)->as_sampled_image(layout::shader_read_only_optimal)),
						descriptor_binding(0, 3, mFramebufferMS->image_view_at(1)->as_sampled_image(layout::shader_read_only_optimal))
						})),
					// Draw a a full-screen quad:
					command::draw(6, 1, 0, 1)
				); }),
				
				command::next_subpass(),

#if STATS_ENABLED
				mTimestampPool->write_timestamp(firstQueryIndex + 12, stage::color_attachment_output), // measure after copy-over combined attachment back into a framebuffer image
#endif

				// 3.9) Render tessellated patches with SS
                command::bind_pipeline(tessPipePxFillSupersampledToBeUsed.as_reference()),
				command::bind_descriptors(tessPipePxFillSupersampledToBeUsed->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					descriptor_binding(0, 0, mFrameDataBuffers[inFlightIndex]),
			        descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			        descriptor_binding(0, 2, mMaterialBuffer),
					descriptor_binding(1, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
					descriptor_binding(1, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
			        descriptor_binding(2, 0, mCountersSsbo->as_storage_buffer()),
				    descriptor_binding(3, 0, mObjectDataBuffer->as_storage_buffer()),
					descriptor_binding(3, 1, mIndirectPxFillParamsBuffer->as_storage_buffer()),
				    descriptor_binding(3, 2, mIndirectPxFillCountBuffer->as_storage_buffer()),
					descriptor_binding(4, 0, mBigDataset->as_storage_buffer())
				})),

				command::push_constants(tessPipePxFillSupersampledToBeUsed->layout(), patch_into_tess_push_constants{ 
					get_rendering_variant_index(rendering_variant::Tess_4xSS_8xMS) * MAX_INDIRECT_DISPATCHES 
				}),
				command::draw_vertices_indirect(
					mIndirectPxFillCountBuffer.as_reference(), 
					get_rendering_variant_index(rendering_variant::Tess_4xSS_8xMS) * sizeof(VkDrawIndirectCommand), 
					sizeof(VkDrawIndirectCommand), 
					1u) // <-- Exactly ONE draw (but potentially a lot of instances), use the one at [1]

			)),
#else 
#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 12, stage::color_attachment_output),
#endif
#endif

#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 13, stage::color_attachment_output), // measure after rendering with Tess. 4xSS+8xMS

			commandsEndStats,
#else
			// Fascinating: Without that following barrier, we get rendering artifacts in the ABSENCE of the timestamp write!
			// TODO:            ^ investigate further!
			sync::global_memory_barrier(stage::fragment_shader | stage::compute_shader >> stage::compute_shader, access::shader_write >> access::shader_read),
#endif

			// //That worked:
			sync::image_memory_barrier(context().main_window()->current_backbuffer()->image_at(0),  // Window's back buffer's color attachment
					                    stage::none          >>   stage::blit,
					                    access::none         >>   access::transfer_write)
				.with_layout_transition(layout::undefined   >>   layout::transfer_dst),             // Don't care about the previous layout

#if SSAA_ENABLED
			command::conditional([this] { return mOutputResultOfRenderVariantIndex == 0; }, [this] { return 
				sync::image_memory_barrier(mFramebufferNoAA->image_at(0),
					                    stage::color_attachment_output     >>   stage::blit, 
					                    access::color_attachment_write     >>   access::transfer_read | access::transfer_write)
				.with_layout_transition(layout::shader_read_only_optimal   >>   layout::transfer_src);
			}),
			command::conditional([this] { return mOutputResultOfRenderVariantIndex == 2; }, [this] { return 
				sync::image_memory_barrier(mFramebufferMS->image_at(0),
					                    stage::color_attachment_output     >>   stage::blit, 
					                    access::color_attachment_write     >>   access::transfer_read | access::transfer_write)
				.with_layout_transition(layout::shader_read_only_optimal   >>   layout::transfer_src);
			}),

			command::conditional([this] { return mOutputResultOfRenderVariantIndex != 1; }, [this] { return command::gather(
				blit_image(   mOutputResultOfRenderVariantIndex == 0 ? mFramebufferNoAA->image_at(0)
							: mOutputResultOfRenderVariantIndex == 2 ? mFramebufferMS->image_at(0)  
							:                                          mFramebufferSSMS->image_at(0), 
							layout::transfer_src, context().main_window()->current_backbuffer()->image_at(0), layout::transfer_dst, 
							vk::ImageAspectFlagBits::eColor, vk::Filter::eLinear),

				sync::image_memory_barrier(context().main_window()->current_backbuffer()->image_at(0),  // Window's back buffer's color attachment
											stage::blit                    >>   stage::color_attachment_output, // <-- for ImGui, which draws afterwards
											access::transfer_write         >>   access::color_attachment_read | access::color_attachment_write)
					.with_layout_transition(       layout::transfer_dst   >>   layout::color_attachment_optimal)
			); }, [this, resolution] { return command::gather(
				// Copy from Uint64 image into back buffer:
				sync::image_memory_barrier(context().main_window()->current_backbuffer()->image_at(0),  // Window's back buffer's color attachment
											stage::none                        >>   stage::compute_shader,
											access::none                       >>   access::shader_storage_write)
					.with_layout_transition(layout::transfer_dst               >>   layout::general),                  // Don't care about the previous layout
						
				command::bind_pipeline(mCopyToBackufferPipe.as_reference()),
				command::push_constants(mCopyToBackufferPipe->layout(), copy_to_backbuffer_push_constants{ 
					mWhatToCopyToBackbuffer > 1 ? 0 : mWhatToCopyToBackbuffer // 0 => 0, 1 => 1, 2 => 0
				}),
				command::bind_descriptors(mCopyToBackufferPipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					descriptor_binding(0, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
					descriptor_binding(0, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
					descriptor_binding(1, 0, context().main_window()->current_backbuffer()->image_view_at(0)->as_storage_image(layout::general))
				})),
				command::dispatch((resolution.x + 15u) / 16u, (resolution.y + 15u) / 16u, 1u),

				sync::image_memory_barrier(context().main_window()->current_backbuffer()->image_at(0),  // Window's back buffer's color attachment
											stage::compute_shader          >>   stage::color_attachment_output, // <-- for ImGui, which draws afterwards
											access::shader_storage_write   >>   access::color_attachment_read | access::color_attachment_write)
					.with_layout_transition(            layout::general   >>   layout::color_attachment_optimal)
			); })
#else 
			blit_image(mFramebufferMS->image_at(0), layout::transfer_src, context().main_window()->current_backbuffer()->image_at(0), layout::transfer_dst, 
						vk::ImageAspectFlagBits::eColor, vk::Filter::eLinear),
			sync::image_memory_barrier(context().main_window()->current_backbuffer()->image_at(0),  // Window's back buffer's color attachment
					                    stage::blit                    >>   stage::color_attachment_output, // <-- for ImGui, which draws afterwards
					                    access::transfer_write         >>   access::color_attachment_read | access::color_attachment_write)
				.with_layout_transition(       layout::transfer_dst   >>   layout::color_attachment_optimal)
#endif


		);

		// Now, with all the commands recorded => submit all the work to the GPU:
		auto batch = context().record(gatheredCommands)
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mQueue)
			// Do not start to render before the image has become available:
			.waiting_for(imageAvailableSemaphore >> stage::color_attachment_output)
			.store_for_now();

		if (mAdditionalSemaphoreDependency.has_value()) {
			batch.waiting_for(mAdditionalSemaphoreDependency.value() >> stage::all_graphics);
		}

		batch.submit();
		
		mainWnd->handle_lifetime(std::move(cmdBfr));
		if (mAdditionalSemaphoreDependency.has_value()) {
			mainWnd->handle_lifetime(std::move(mAdditionalSemaphoreDependency.value()));
			mAdditionalSemaphoreDependency.reset();
		}

		std::chrono::steady_clock::time_point renderEnd = std::chrono::steady_clock::now();
		auto renderMs = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(renderEnd - renderBegin).count());
		mRenderDurationMs = glm::mix(mRenderDurationMs, renderMs, 0.05);
	}

private: // v== Member variables ==v

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;

	std::vector<avk::buffer> mFrameDataBuffers;
	avk::buffer mMaterialBuffer;
	std::vector<avk::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;

	avk::compute_pipeline mInitPatchesComputePipe;
	avk::compute_pipeline mInitKnitYarnComputePipe;
	avk::compute_pipeline mInitShBrainComputePipe;
	avk::compute_pipeline mPatchLodComputePipe;
	avk::compute_pipeline mPxFillComputePipe;
	avk::compute_pipeline mPxFillLocalFbComputePipe;
#if SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
	avk::compute_pipeline mSelectTilePatchesPipe;
#endif

	avk::orbit_camera mOrbitCam;
	avk::quake_camera mQuakeCam;

    uint32_t mTaskInvocationsExt;
    uint32_t mMeshInvocationsExt;

	std::vector<parametric_object> mParametricObjects;
	std::vector<bool> mParametricObjectsVisibilitiesSaved;
	std::vector<object_data> mObjectData;
	uint32_t mNumEnabledObjects;
	uint32_t mNumEnabledKnitYarnObjects;
	uint32_t mNumEnabledShBrainObjects;
	std::vector<uint32_t> mKnitYarnObjectDataIndices;
	std::vector<uint32_t> mShBrainObjectDataIndices;

	// UI-controllable parameters:
    //std::optional<bool> mUseNvPipeline     = {};
    // Parametric parameters:
	int                        mFirstPassPipelineSelection = 0;
    int                        mParamDim        = 0; // 0...1D, 1...2D
    std::array<int, 2>         mNumTaskGroups   = {{2, 2}};
    int                        mTaskTileSize1D  = 0;
    int                        mTaskTileSize2D  = 0;
    int                        mMeshTileSize1D  = 0;
    int                        mMeshTileSize2D  = 0;
    int                        mCurveIndex      = 1;
	// Debug stuff:
	glm::vec4                  mDebugSliders    = {0.f, 0.f, 0.f, 0.f};
	glm::ivec4                 mDebugSlidersi   = {12, 12, 12, 12};

#if STATS_ENABLED
    avk::query_pool mTimestampPool;

	uint64_t mLastClearDuration = 0;				// + 1, stage::compute_shader), // measure after clearing
	uint64_t mLastInitDuration = 0;					// + 2, stage::compute_shader), // measure after init
	uint64_t mLastLodStageDuration = 0;				// + 3, stage::compute_shader), // measure after LOD stage
	uint64_t mLast3dModelsRenderDuration = 0;		// + 4, stage::color_attachment_output), // measure after rendering sponza
	uint64_t mLastTessNoaaDuration = 0;				// + 5, stage::color_attachment_output), // measure after Tess. noAA
	uint64_t mLastCopyToCombinedAttachmentDur = 0;	// + 6, stage::compute_shader), // measure after copy-over to combined attachment
	uint64_t mLastPatchToTileStepDuration = 0;		// + 7, stage::compute_shader), // measure after patch to tile step
	uint64_t mLastPointRenderingNoaaDuration = 0;	// + 8, stage::compute_shader), // measure after px fill (the version with the holes ^^)
	uint64_t mLastPointRenderingLocalFbDur = 0;		// + 9, stage::compute_shader), // measure after point rendering into local fb
	uint64_t mLastFSQbefore8xSSDuration = 0;		// + 10, stage::color_attachment_output), // measure after copy-over combined attachment back into a framebuffer image
	uint64_t mLastTess8xSSDuration = 0;				// + 11, stage::color_attachment_output), // measure after Tess. 8xSS
	uint64_t mLastFSQbefore4xSS8xMSDuration = 0;	// + 12, stage::color_attachment_output), // measure after copy-over combined attachment back into a framebuffer image
	uint64_t mLastTess4xSS8xMSDuration = 0;			// + 13, stage::color_attachment_output), // measure after rendering with Tess. 4xSS+8xMS
	uint64_t mLastTotalRenderDuration = 0;
	uint64_t mLastTotalStepsDuration = 0;

	avk::query_pool mPipelineStatsPool;
	std::array<uint64_t, 3> mPipelineStats;
#endif
	
	avk::renderpass  mRenderpassNoAA;
	avk::renderpass  mRenderpassMS;
	avk::renderpass  mRenderpassSSMS;
	avk::framebuffer mFramebufferNoAA;
	avk::framebuffer mFramebufferMS;
	avk::framebuffer mFramebufferSSMS;
	avk::image_view  mCombinedAttachmentView;
    avk::compute_pipeline mCopyToBackufferPipe;
    avk::compute_pipeline mClearCombinedAttachmentPipe;
    avk::compute_pipeline mCopyToCombinedAttachmentPipe;
	avk::buffer mObjectDataBuffer;
	avk::buffer mPatchLodBufferPing;
	avk::buffer mPatchLodBufferPong;
	avk::buffer mPatchLodCountBuffer;
#if SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
	avk::buffer mTilePatchesBuffer;
#endif
	avk::buffer mIndirectPxFillParamsBuffer;
	avk::buffer mIndirectPxFillCountBuffer;

    avk::graphics_pipeline mVertexPipeline;
    avk::graphics_pipeline mTessPipelinePxFillNoaa;
    avk::graphics_pipeline mTessPipelinePxFillMultisampled;
    avk::graphics_pipeline mTessPipelinePxFillSupersampled;
    avk::graphics_pipeline mTessPipelinePxFillNoaaWireframe;
    avk::graphics_pipeline mTessPipelinePxFillMultisampledWireframe;
    avk::graphics_pipeline mTessPipelinePxFillSupersampledWireframe;
	avk::graphics_pipeline mFsQuadNoAAtoMSPipe;
	avk::graphics_pipeline mFsQuadMStoSSPipe;
	avk::sampler mFsQuadColorSampler;
	avk::sampler mFsQuadDepthSampler;

#if STATS_ENABLED
    bool mGatherPipelineStats      = true;
	avk::image_view  mHeatMapImageView;
#else
    bool mGatherPipelineStats      = false;
#endif
	int mWhatToCopyToBackbuffer = 0; // 0 ... rendering output, 1 ... heat map, 2 ... generate both, but copy rendering output

	std::vector<glm::vec3> mSpherePositions;

	bool mStartMeasurement = false;
	float mDistanceFromOrigin = 22.0f;
	bool mMeasurementInProgress = false;
	double mMeasurementStartTime = 0.0;
	double mMeasurementEndTime = 0.0;
	std::vector<std::tuple<double, float, uint64_t, uint64_t, int>> mMeasurementFrameCounters;
	bool mMoveCameraDuringMeasurements = true;
	int mMeasurementMoveCameraSteps = 6;
	float mMeasurementMoveCameraDelta = -3.0f;
	int mMeasurementIndexLastFrame = -1;

	avk::buffer mCountersSsbo;
	std::array<uint32_t, 4> mCounterValues;
	std::array<uint32_t, MAX_PATCH_SUBDIV_STEPS> mPatchesCreatedPerLevel;
#if TEST_GATHER_PATCH_COUNTS
	std::vector<std::array<uint32_t, MAX_PATCH_SUBDIV_STEPS>> mMeasurementPatchCounts;
#endif
	std::array<uint32_t, NUM_DIFFERENT_RENDER_VARIANTS> mNumPxFillPatchesCreated;

	// Other, global settings, stored in common UBO:
    bool m2ndPassEnabled = false;

	// Buffers for vertex pipelines:
	avk::buffer mPositionsBuffer;
	avk::buffer mTexCoordsBuffer;
	avk::buffer mNormalsBuffer;
	avk::buffer mIndexBuffer;
	std::vector<vertex_buffers_offsets_sizes> mVertexBuffersOffsetsSizes;
	avk::buffer mVertexBuffersOffsetsSizesBuffer;
	glm::uvec4  mVertexBuffersOffsetsSizesCount = glm::uvec4{0}; // only .x is used
	avk::buffer mVertexBuffersOffsetsSizesCountBuffer;

	bool mFrustumCullingOn = true;
    bool mBackfaceCullingOn = true;

	int mNumMaterials = 1;
	bool mRenderExtra3DModel = true;
	double mRenderDurationMs = 1;

	bool mAnimationPaused = false;
	float mAnimationPauseTime;
	float mTimeToSubtract = 0.0f;

	bool mWireframeModeOn = false;

	bool mWarnedLastFrame = false;
	bool mSingleObjectOverrideTranslation = true;
	glm::vec3 mSingleObjectTranslationOverride = glm::vec3(0.0f, 1.0f, 0.0f);
	bool mSingleObjectOverrideMaterial = false;
	int  mSingleObjectMaterialOverride = -1;
	bool mDisableColorAttachmentOut = false;

	std::unordered_map<std::string, avk::image_sampler> mPreviewImageSamplers;
	std::optional<avk::semaphore> mAdditionalSemaphoreDependency;

	avk::buffer mBigDataset;
	uint32_t mDatasetSize = 0;

	bool mEnableSampleShadingFor8xMS;
	bool mEnableSampleShadingFor4xSS8xMS;
	std::array<bool, 3> mRenderVariantDataTransferEnabled;
	int  mOutputResultOfRenderVariantIndex;

}; // vk_parametric_curves_app


int main() // <== Starting point ==
{
	float clearDepth = 1.0;
	glm::vec4  clearColor = glm::vec4(0.0);
	uint64_t depthEncoded = depth_to_ui64(clearDepth);
	uint64_t colorEncoded = color_to_ui64(clearColor);
	uint64_t data = combine_depth_and_color(depthEncoded, colorEncoded);

	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Fast Rendering of Parametric Objects on Modern GPUs");

		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->request_srgb_framebuffer(false);
		mainWnd->enable_resizing(true);
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->set_image_usage_properties(avk::image_usage::general_storage_image | avk::image_usage::color_attachment);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);

		// Create an instance of our main avk::element which contains all the functionality:
		auto app = vk_parametric_curves_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);
		//ui.set_custom_font("assets/JetBrainsMono-Regular.ttf");

		auto image64ext = vk::PhysicalDeviceShaderImageAtomicInt64FeaturesEXT{}
			.setShaderImageInt64Atomics(VK_TRUE);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Fast Rendering of Parametric Objects on Modern GPUs"),
#if defined(_DEBUG)
			// Enable debug printf:
			[](avk::validation_layers& validationLayers) {
				validationLayers.enable_feature(vk::ValidationFeatureEnableEXT::eDebugPrintf);
			},
            avk::required_device_extensions(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME),
#endif
			avk::required_device_extensions(VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME),
			avk::required_device_extensions(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME),
			avk::physical_device_features_pNext_chain_entry{ &image64ext },
			[](vk::PhysicalDeviceFeatures& features) {
				features.setPipelineStatisticsQuery(VK_TRUE);
				features.setShaderInt64(VK_TRUE);
			},
			[](vk::PhysicalDeviceVulkan11Features& features) {
				features.setShaderDrawParameters(VK_TRUE);
			},
			[](vk::PhysicalDeviceVulkan12Features& features) {
				features.setUniformAndStorageBuffer8BitAccess(VK_TRUE);
				features.setStorageBuffer8BitAccess(VK_TRUE);
				features.setShaderBufferInt64Atomics(VK_TRUE);
				features.setShaderSharedInt64Atomics(VK_TRUE);
				features.setDrawIndirectCount(VK_TRUE);
			},
			// Pass windows:
			mainWnd,
			// Pass invokees:
			app, ui
		);

		// Create an invoker object, which defines the way how invokees/elements are invoked
		// (In this case, just sequentially in their execution order):
		avk::sequential_invoker invoker;

		// With everything configured, let us start our render loop:
		composition.start_render_loop(
			// Callback in the case of update:
			[&invoker](const std::vector<avk::invokee*>& aToBeInvoked) {
				// Call all the update() callbacks:
				invoker.invoke_updates(aToBeInvoked);
			},
			// Callback in the case of render:
			[&invoker](const std::vector<avk::invokee*>& aToBeInvoked) {
				// Sync (wait for fences and so) per window BEFORE executing render callbacks
				avk::context().execute_for_each_window([](avk::window* wnd) {
					wnd->sync_before_render();
				});

				// Call all the render() callbacks:
				invoker.invoke_renders(aToBeInvoked);

				// Render per window:
				avk::context().execute_for_each_window([](avk::window* wnd) {
					wnd->render_frame();
				});
			}
		); // This is a blocking call, which loops until avk::current_composition()->stop(); has been called (see update())
	
		result = EXIT_SUCCESS;
	}
	catch (avk::logic_error&) {}
	catch (avk::runtime_error&) {}
	return result;
}
