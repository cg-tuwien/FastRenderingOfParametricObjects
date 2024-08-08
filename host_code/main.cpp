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

#define NUM_TIMESTAMP_QUERIES 5
#define EXTRA_3D_MODEL "sponza_and_terrain"
//#define EXTRA_3D_MODEL "SunTemple"

// Sample count possible values:
// vk::SampleCountFlagBits::e1
// vk::SampleCountFlagBits::e2
// vk::SampleCountFlagBits::e4
// vk::SampleCountFlagBits::e8
// vk::SampleCountFlagBits::e16
// vk::SampleCountFlagBits::e32
// vk::SampleCountFlagBits::e64
#define SAMPLE_COUNT vk::SampleCountFlagBits::e4

#define TEST_MODE_ON                   1
#define TEST_GATHER_PIPELINE_STATS     1
#define TEST_ALLOW_GATHER_STATS        1
#define TEST_DURATION_PER_STEP          2.5f
#define TEST_GATHER_TIMER_QUERIES      1

// UNCOMMENT FOR KNIT YARN MEASUREMENT:
#define TEST_CAMDIST                    0.4f
#define TEST_CAMERA_DELTA_POW           2.2f
#define TEST_TRANSLATE_Y               false
#define TEST_TRANSLATE_Z               true
#define TEST_CAM_Y_SHIFT               4.08f
#define TEST_ENABLE_OBJ_IDX            18
#define TEST_INNER_TESS_LEVEL          19
#define TEST_OUTER_TESS_LEVEL          19
#define TEST_SCREEN_DISTANCE_THRESHOLD 62
#define TEST_USEINDIVIDUAL_PATCH_RES   1
#define TEST_ENABLE_3D_MODEL           0
#define POINT_RENDERIN_PATCH_RES_S_X   1.2f
#define POINT_RENDERIN_PATCH_RES_S_Y   1.2f

//// UNCOMMENT FOR FIBER CURVES MEASUREMENT:
//#define TEST_CAMDIST                    0.4f
//#define TEST_CAMERA_DELTA_POW           2.2f
//#define TEST_TRANSLATE_Y               false
//#define TEST_TRANSLATE_Z               true
//#define TEST_CAM_Y_SHIFT               4.08f
//#define TEST_ENABLE_OBJ_IDX            19
//#define TEST_INNER_TESS_LEVEL          19
//#define TEST_OUTER_TESS_LEVEL          19
//#define TEST_SCREEN_DISTANCE_THRESHOLD 62
//#define TEST_USEINDIVIDUAL_PATCH_RES   1
//#define TEST_ENABLE_3D_MODEL           0
//#define POINT_RENDERIN_PATCH_RES_S_X   1.2f
//#define POINT_RENDERIN_PATCH_RES_S_Y   1.2f

//// UNCOMMENT FOR SEASHELL MEASUREMENT:
//#define TEST_CAMDIST                    4.0f
//#define TEST_CAMERA_DELTA_POW           1.5f
//#define TEST_TRANSLATE_Y               false
//#define TEST_TRANSLATE_Z               true
//#define TEST_CAM_Y_SHIFT               0.0f
//#define TEST_ENABLE_OBJ_IDX            20
//#define TEST_INNER_TESS_LEVEL          64
//#define TEST_OUTER_TESS_LEVEL          64
//#define TEST_SCREEN_DISTANCE_THRESHOLD 62
//#define TEST_USEINDIVIDUAL_PATCH_RES   0
//#define TEST_ENABLE_3D_MODEL           0
//#define POINT_RENDERIN_PATCH_RES_S_X   1.5f
//#define POINT_RENDERIN_PATCH_RES_S_Y   1.5f

// ================= UNCOMMENT FOR PATCH->TESS RENDERING METHOD ======================
// Values: rendering_method::point_rendered | rendering_method::tessellated_rasterized
#define TEST_RENDERING_METHOD          rendering_method::tessellated_rasterized
// Values: 0 = Uint64 image | 3 = framebuffer (must use that for SSAA or MSAA)
#define TEST_TARGET_IMAGE              3

//// ================= UNCOMMENT FOR POINT->PATCH RENDERING METHOD =====================
//// Values: rendering_method::point_rendered | rendering_method::tessellated_rasterized
//#define TEST_RENDERING_METHOD          rendering_method::point_rendered
//// Values: 0 = Uint64 image | 3 = framebuffer (must use that for SSAA or MSAA)
//#define TEST_TARGET_IMAGE              0

static std::array<parametric_object, 13> PredefinedParametricObjects {{
	parametric_object{"Sphere"      , "assets/po-sphere-patches.png",     true , parametric_object_type::Sphere,                 0.0f, glm::pi<float>(),  0.0f,  glm::two_pi<float>(), glm::translate(glm::vec3{ 0.f,  0.f,  0.f})},
	parametric_object{"Johi's Heart", "assets/po-johis-heart.png",        true, parametric_object_type::JohisHeart,             0.0f, glm::pi<float>(),  0.0f,  glm::two_pi<float>(), glm::translate(glm::vec3{ 0.f,  0.f, -2.f})},
	parametric_object{"Spiky Heart" , "assets/po-johis-heart.png",        false, parametric_object_type::SpikyHeart,             0.0f, glm::pi<float>(),  0.0f,  glm::two_pi<float>(), glm::translate(glm::vec3{ 4.f,  0.f, -2.f})},
	parametric_object{"Seashell 1"  , "assets/po-seashell1.png",          false, parametric_object_type::Seashell1,              glm::two_pi<float>() * 8.0f,/* -> */0.0f,   0.0f,/* -> */glm::two_pi<float>(), glm::mat4{ 1.0f }, 2},
	parametric_object{"Seashell 2"  , "assets/po-seashell2.png",          false, parametric_object_type::Seashell2,              glm::two_pi<float>() * 8.0f,/* -> */0.0f,   0.0f,/* -> */glm::two_pi<float>(), glm::translate(glm::vec3{-4.5f, 7.0f, 0.0f }), 2},
	parametric_object{"Seashell 3"  , "assets/po-seashell3.png",          false, parametric_object_type::Seashell3,              glm::two_pi<float>() * 8.0f,/* -> */0.0f,   0.0f,/* -> */glm::two_pi<float>(), glm::translate(glm::vec3{ 4.5f, 7.0f, 0.0f }), 2},
	parametric_object{"Yarn Curve"  , "assets/po-yarn-curve-single.png",  false, parametric_object_type::YarnCurve,  1.0f, 1.0f, /* <-- yarn dimensions | #fibers --> */ 4.f, 1.f, glm::translate(glm::vec3{-3.35f, 0.08f, 5.32f}) * glm::scale(glm::vec3{ 0.005f }), 17},
	parametric_object{"Fiber Curve" , "assets/po-fiber-curve-single.png", false, parametric_object_type::FiberCurve, 1.0f, 1.0f, /* <-- yarn dimensions | #fibers --> */ 4.f, 1.f, glm::translate(glm::vec3{-3.35f, 0.08f, 5.32f}) * glm::scale(glm::vec3{ 0.005f }), 17},
	parametric_object{"Blue Curtain", "assets/po-blue-curtain.png",		  false, parametric_object_type::YarnCurve,  235.0f, 254.0f, /* <-- yarn dimensions | #fibers --> */ 4.f, 1.f, glm::translate(glm::vec3{-3.35f, 0.08f, 5.32f}) * glm::scale(glm::vec3{ 0.005f }), 17},
	parametric_object{"Palm Tree"   , "assets/po-palm-tree.png",		  false, parametric_object_type::PalmTreeTrunk,          0.0f,   1.0f,            0.0f,  glm::two_pi<float>(), glm::translate(glm::vec3{ 0.f,  0.f, -4.f})},
	parametric_object{"Giant Worm"  , "assets/po-giant-worm.png",		  false, parametric_object_type::PalmTreeTrunk,  235.0f, 254.0f, /* <-- yarn dimensions | #fibers --> */ 4.f, 1.f, glm::translate(glm::vec3{-3.35f, 0.08f, 5.32f}) * glm::scale(glm::vec3{ 0.005f }), 17},
	parametric_object{"SH Glyph"    , "assets/po-single-sh-glyph.png",    false, parametric_object_type::SHGlyph,                0.0f, glm::pi<float>(),  0.0f,  glm::two_pi<float>()},
	parametric_object{"Brain Scan"  , "assets/po-sh-brain.png",           false, parametric_object_type::SHGlyph,                0.0f, glm::pi<float>(),  0.0f,  glm::two_pi<float>()}
}};

class vk_parametric_curves_app : public avk::invokee
{

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	vk_parametric_curves_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
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

	void load_models_and_materials()
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

		std::vector<std::tuple<model, int32_t>> loadedModels;

		for (const auto& [m, pxMeridian] : loadedModels) {
			AVK_LOG_INFO(std::format("Loaded sphere model with {} vertices ({} thereof along its meridian) and {} triangles.", m->number_of_vertices_for_mesh(0), pxMeridian, m->number_of_indices_for_mesh(0) / 3));
		}

		std::vector<material_config> allMatConfigs; // <-- Gather the material config from all models to be loaded
		std::vector<loaded_model_data> dataForDrawCall;

		vk::PhysicalDeviceFeatures2 supportedExtFeatures;
		auto meshShaderFeatures = vk::PhysicalDeviceMeshShaderFeaturesEXT{};
		supportedExtFeatures.pNext = &meshShaderFeatures;
		context().physical_device().getFeatures2(&supportedExtFeatures);

		for (size_t i = 0; i < loadedModels.size(); ++i) {
			auto& [curModel, pxMeridian] = loadedModels[i];

			const auto meshIndicesInOrder = curModel->select_all_meshes();

			auto distinctMaterials = curModel->distinct_material_configs();
			const auto matOffset = allMatConfigs.size();
			// add all the materials of the model
			for (auto& pair : distinctMaterials) {
				allMatConfigs.push_back(pair.first);
			}

			for (size_t mpos = 0; mpos < meshIndicesInOrder.size(); mpos++) {
				auto meshIndex = meshIndicesInOrder[mpos];
				std::string meshname = curModel->name_of_mesh(mpos);

				auto& drawCallData = dataForDrawCall.emplace_back();

				drawCallData.mMaterialIndex = static_cast<int32_t>(matOffset);
				drawCallData.mPixelsOnMeridian = pxMeridian;
				drawCallData.mModelMatrix = curModel->transformation_matrix_for_mesh(meshIndex);
				// Find and assign the correct material in the allMatConfigs vector
				for (auto pair : distinctMaterials) {
					if (std::end(pair.second) != std::ranges::find(pair.second, meshIndex)) {
						break;
					}
					drawCallData.mMaterialIndex++;
				}

				auto selection = make_model_references_and_mesh_indices_selection(curModel, meshIndex);
				std::tie(drawCallData.mPositions, drawCallData.mIndices) = get_vertices_and_indices(selection);
				drawCallData.mNormals = get_normals(selection);
				drawCallData.mTexCoords = get_2d_texture_coordinates(selection, 0);
			}
		} // for (size_t i = 0; i < loadedModels.size(); ++i)

#ifdef EXTRA_3D_MODEL
		if (std::filesystem::exists("assets/" EXTRA_3D_MODEL ".fscene")) {
			mExtra3DModelBeginIndex = dataForDrawCall.size();
			glm::mat4 sceneTransform = glm::mat4(1.0f);
			if constexpr (std::string("SunTemple") == EXTRA_3D_MODEL) {
				sceneTransform = glm::translate(glm::vec3(0.0f, -4.0f, -15.0f));
			}
			if constexpr (std::string("sponza_and_terrain") == EXTRA_3D_MODEL) {
				sceneTransform = glm::translate(glm::vec3(2.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3{ 2.667f });
			}

			auto orca = orca_scene_t::load_from_file("assets/" EXTRA_3D_MODEL ".fscene");
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
							if constexpr (std::string("sponza_and_terrain") == EXTRA_3D_MODEL) {
								// Excluding one blue curtain (of a total of three) by modifying the loaded indices before uploading them to a GPU buffer:
								if (meshname == "sponza_326") {
									drawCallData.mIndices.erase(std::begin(drawCallData.mIndices), std::begin(drawCallData.mIndices) + 3 * 4864);
								}
							}
					        drawCallData.mNormals = get_normals(selection);
					        drawCallData.mTexCoords = get_2d_texture_coordinates(selection, 0);
				        }
					}
			    }
		    }
		}
#endif

		// create all the buffers for our drawcall data
		add_draw_calls(dataForDrawCall);
		//   ^ With all the offsets and sizes data gathered => transfer also that data into the respective buffers (see below at record_and_submit_with_fence).

		mNumMaterials = static_cast<int>(allMatConfigs.size());
		LOG_INFO_EM(std::format("Number of custom materials = {}", mNumMaterials));

		// For all the different materials, transfer them in structs which are well
		// suited for GPU-usage (proper alignment, and containing only the relevant data),
		// also load all the referenced images from file and provide access to them
		// via samplers; It all happens in `ak::convert_for_gpu_usage`:
		auto [gpuMaterials, imageSamplers, matCommands] = convert_for_gpu_usage<material_gpu_data>(
			allMatConfigs, false, true,
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

		mPatchLodComputePipe = create_compute_pipe_for_parametric(
			"shaders/pass2x_patch_lod.comp",
			// Ping-pong buffers:
			descriptor_binding(3, 0, mPatchLodBufferPing->as_storage_buffer()), // Attention: These...
			descriptor_binding(3, 1, mPatchLodBufferPong->as_storage_buffer()), //            ...two will be swapped for consecutive dispatch calls at pass2x-level.
			descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer()),
#if DRAW_PATCH_EVAL_DEBUG_VIS
            descriptor_binding(4, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
            descriptor_binding(4, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
#endif
			push_constant_binding_data{ shader_type::all, 0, sizeof(pass2x_push_constants) }
		);
        mUpdater->on(shader_files_changed_event(mPatchLodComputePipe.as_reference())).update(mPatchLodComputePipe);

		mPxFillComputePipe = create_compute_pipe_for_parametric(
#if PX_FILL_LOCAL_FB
			"shaders/pass3_px_fill_local_fb.comp",
#else
			"shaders/pass3_px_fill.comp", 
#endif
			descriptor_binding(0, 1, as_combined_image_samplers(mImageSamplers, layout::shader_read_only_optimal)),
			descriptor_binding(0, 2, mMaterialBuffer), // TODO: Optimized descriptor set assignment would be cool
            // super druper image & heatmap image:
            descriptor_binding(3, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
#if STATS_ENABLED
            descriptor_binding(3, 1, mHeatMapImageView->as_storage_image(layout::general)),
#endif
#if PX_FILL_LOCAL_FB && SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
			descriptor_binding(4, 0, mTilePatchesBuffer->as_storage_buffer()),
#endif
			push_constant_binding_data{ shader_type::all, 0, sizeof(pass3_push_constants) }
		);
        mUpdater->on(shader_files_changed_event(mPxFillComputePipe.as_reference())).update(mPxFillComputePipe);

#if PX_FILL_LOCAL_FB && SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
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
				cfg::viewport_depth_scissors_config::from_framebuffer(mFramebuffer.as_reference()),

				mRenderpass,
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
				cfg::viewport_depth_scissors_config::from_framebuffer(mFramebuffer.as_reference()),

                mRenderpass,

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
        auto  colorAttachment    = context().create_image(resolution.x, resolution.y, aAttachmentFormats[0], 1, memory_usage::device,
                                                          image_usage::color_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
		auto  colorAttachmentMS  = context().create_image(resolution.x, resolution.y, colorFormatMS, 1, memory_usage::device,
                                                          image_usage::color_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
        auto  depthAttachment    = context().create_image(resolution.x, resolution.y, aAttachmentFormats[1], 1, memory_usage::device,
                                                          image_usage::depth_stencil_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
        auto  depthAttachmentMS   = context().create_image(resolution.x, resolution.y, depthFormatMS, 1, memory_usage::device,
                                                          image_usage::depth_stencil_attachment | image_usage::input_attachment | image_usage::sampled | image_usage::tiling_optimal | image_usage::transfer_source);
        auto  combinedAttachment = context().create_image(resolution.x, resolution.y, vk::Format::eR64Uint, 1, memory_usage::device, image_usage::shader_storage);

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
        mColorAttachmentView       = context().create_image_view(std::move(colorAttachment));
		auto colorAttachmentViewMS = context().create_image_view(std::move(colorAttachmentMS));
        mDepthAttachmentView       = context().create_image_view(std::move(depthAttachment));
        auto depthAttachmentViewMS = context().create_image_view(std::move(depthAttachmentMS));
        mCombinedAttachmentView    = context().create_image_view(std::move(combinedAttachment));
#if STATS_ENABLED
		mHeatMapImageView          = context().create_image_view(std::move(heatMapImage));
#endif

        mRenderpass = context().create_renderpass({// Define attachments and sub pass usages:                          vvv  Note: All the draw calls are in the same (first) subpass: SS, noAA, simple-stupid vertex pipe (because... why not?!)
                attachment::declare(aAttachmentFormats[0], on_load::clear.from_previous_layout(layout::undefined), usage::unused                              , on_store::store.in_layout(layout::shader_read_only_optimal)),
                attachment::declare(aAttachmentFormats[1], on_load::clear.from_previous_layout(layout::undefined), usage::unused                              , on_store::store.in_layout(layout::shader_read_only_optimal)),
                attachment::declare(colorFormatMS        , on_load::clear.from_previous_layout(layout::undefined), usage::color(0)      + usage::resolve_to(0), on_store::dont_care),
                attachment::declare(depthFormatMS        , on_load::clear.from_previous_layout(layout::undefined), usage::depth_stencil + usage::resolve_to(1), on_store::dont_care)
            }, {
				subpass_dependency{subpass::external >> subpass::index(0),
					stage::none    >>   stage::all_graphics,
					access::none   >>   access::color_attachment_write | access::depth_stencil_attachment_read | access::depth_stencil_attachment_write
				},
				subpass_dependency{subpass::index(0) >> subpass::external,
					stage::fragment_shader       | stage::color_attachment_output  >>   stage::compute_shader                                      | stage::transfer,
					access::shader_storage_write | access::color_attachment_write  >>   access::shader_storage_read | access::shader_storage_write | access::memory_read
				}
			});

        mFramebuffer = context().create_framebuffer(mRenderpass, make_vector(
			mColorAttachmentView, 
			mDepthAttachmentView,
			colorAttachmentViewMS,
			depthAttachmentViewMS
		));
    }

	bool is_knit_yarn(parametric_object_type pot)
	{
		return pot == parametric_object_type::YarnCurve
			|| pot == parametric_object_type::YarnCurveAnimated
			|| pot == parametric_object_type::FiberCurve
			|| pot == parametric_object_type::FiberCurveAnimated;
	}

	void fill_object_data_buffer()
	{
		using namespace avk;

		mObjectData.resize(MAX_OBJECTS, object_data{});
		mKnitYarnObjectDataIndices.clear();
		std::vector<object_data> knitYarnSavedForLater;

		uint32_t i = 0; 
	    for (const auto& po : mParametricObjects) {
            if (!po.is_enabled()) {
                continue;
            }

			object_data tmp;
            tmp.mParams = po.uv_param_ranges();
			tmp.mCurveIndex = po.curve_index();
			tmp.mDetailEvalDims = po.eval_dims();
            tmp.mTransformationMatrix = po.transformation_matrix();
			tmp.mMaterialIndex = po.material_index();
			
			if (!is_knit_yarn(po.param_obj_type())) {
				mObjectData[i] = tmp;
				++i;
			}
			else {
				knitYarnSavedForLater.push_back(tmp);
			}
        }

		mNumEnabledObjects = i;

		for (const auto& ky : knitYarnSavedForLater) {
			mObjectData[i] = ky;
			mKnitYarnObjectDataIndices.push_back(i);
			++i;
		}

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
			std::vector<int> howRendered(mParametricObjects.size(), 0);
			ImGui::TableNextRow();
			poId = 0;
			for (auto& po : mParametricObjects) {
				howRendered[poId] = po.how_to_render() == rendering_method::point_rendered ? 0 : 1;
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
				if (ImGui::Combo("##renderingmethod", reinterpret_cast<int*>(&howRendered), "Point-rendered\0Tessellated -> rasterized\0(Wireframe) Tessellated -> rasterized\0Hybrid\0")) {
					po.set_how_to_render(howRendered);
				}
				poId++;
				ImGui::PopID();
			}
			// ...and another (related) table row for enabling/disabling super sampling:
			poId = 0;
			for (auto& po : mParametricObjects) {
				ImGui::TableNextColumn();
				ImGui::PushID(poId++);
				bool ss = po.super_sampling_on();
				if (ImGui::Checkbox("Super-Sampled", &ss)) {
					po.set_super_sampling(ss);
				}
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
				auto matIndex = po.material_index();
				if (ImGui::SliderInt("Material Index", &matIndex, -1, mNumMaterials - 1)) {
					po.set_material_index(matIndex);
					updateObjects = true;
				}
			}
			ImGui::EndTable();
		}
		ImGui::End();
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

		// Don't really need those for our parametric adventures, but we'll see:
		load_models_and_materials();

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

#if PX_FILL_LOCAL_FB && SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
		mTilePatchesBuffer = context().create_buffer(
			memory_usage::device, {},
			storage_buffer_meta::create_from_size(TILE_PATCHES_BUFFER_ELEMENTS * sizeof(uint32_t))
		);
#endif

		// Create buffers for the indirect draw calls of the parametric pipelines:
        mIndirectPxFillParamsBuffer = context().create_buffer(
			memory_usage::device, {}, 
			storage_buffer_meta::create_from_size(         MAX_INDIRECT_DISPATCHES * sizeof(px_fill_data))
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
			indirect_buffer_meta::create_from_num_elements(1, sizeof(VkDrawIndirectCommand)),
			storage_buffer_meta::create_from_size(            sizeof(VkDrawIndirectCommand))
		);

		// PARAMETRIC PIPES:
        create_param_pipes();

		// VERTEX and TESS. PIPES:
		mVertexPipeline = create_vertex_pipe();
		mUpdater->on(shader_files_changed_event(mVertexPipeline.as_reference())).update(mVertexPipeline);

		mTessPipelinePxFill = create_tess_pipe(
			"shaders/px-fill-tess/patch_ready.vert", 
			"shaders/px-fill-tess/patch_set.tesc", 
			"shaders/px-fill-tess/patch_go.tese",
			push_constant_binding_data{shader_type::all, 0, sizeof(patch_into_tess_push_constants)},
			cfg::shade_per_fragment()
		);
		mUpdater->on(shader_files_changed_event(mTessPipelinePxFill.as_reference())).update(mTessPipelinePxFill);

		// Create an (almost identical) pipeline to render the scene in wireframe mode
		mTessPipelinePxFillWireframe = context().create_graphics_pipeline_from_template(mTessPipelinePxFill.as_reference(), [](graphics_pipeline_t& p) {
			p.rasterization_state_create_info().setPolygonMode(vk::PolygonMode::eLine);
		});
		mUpdater->on(shader_files_changed_event(mTessPipelinePxFillWireframe.as_reference())).update(mTessPipelinePxFillWireframe);

		mTessPipelinePxFillSS = create_tess_pipe(
			"shaders/px-fill-tess/patch_ready.vert", 
			"shaders/px-fill-tess/patch_set.tesc", 
			"shaders/px-fill-tess/patch_go.tese",
			push_constant_binding_data{shader_type::all, 0, sizeof(patch_into_tess_push_constants)},
			cfg::shade_per_sample()
		);
		mUpdater->on(shader_files_changed_event(mTessPipelinePxFillSS.as_reference())).update(mTessPipelinePxFillSS);

		// Create an (almost identical) pipeline to render the scene in wireframe mode
		mTessPipelinePxFillWireframeSS = context().create_graphics_pipeline_from_template(mTessPipelinePxFillSS.as_reference(), [](graphics_pipeline_t& p) {
			p.rasterization_state_create_info().setPolygonMode(vk::PolygonMode::eLine);
		});
		mUpdater->on(shader_files_changed_event(mTessPipelinePxFillWireframeSS.as_reference())).update(mTessPipelinePxFillWireframeSS);

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
#if PX_FILL_LOCAL_FB && SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
			, descriptor_binding(4, 0, mTilePatchesBuffer->as_storage_buffer())
#endif
		);
        mUpdater->on(shader_files_changed_event(mClearCombinedAttachmentPipe.as_reference())).update(mClearCombinedAttachmentPipe);

		mCopyToCombinedAttachmentPipe = context().create_compute_pipeline_for(
			"shaders/copy_to_combined_attachment.comp",
            descriptor_binding(0, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
			descriptor_binding(1, 0, mColorAttachmentView->as_sampled_image(layout::shader_read_only_optimal)),
			descriptor_binding(1, 1, mDepthAttachmentView->as_sampled_image(layout::shader_read_only_optimal))
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
		mOrbitCam.set_perspective_projection(glm::radians(45.0f), context().main_window()->aspect_ratio(), 0.15f, 100.0f);
		mQuakeCam.set_perspective_projection(glm::radians(45.0f), context().main_window()->aspect_ratio(), 0.15f, 100.0f);
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
				lastFrameDurationMs = 0.0,
				lastClearDuration = 0.0,
				lastPatchLodDuration = 0.0,
				lastPatchToTileDuration = 0.0,
				lastRenderduration = 0.0,
				lastBeginToRenderEndDuration = 0.0
			]() mutable {
				if (mMeasurementInProgress) {
					return;
				}

				draw_parametric_objects_ui(imguiManager);

				ImGui::Begin("Info & Settings");
			    const auto imGuiWindowWidth = ImGui::GetWindowWidth();

				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::SetWindowSize(ImVec2(661, 643) , ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::Text("%.1lf ms/render() CPU time", mRenderDurationMs);
				ImGui::Separator();
				ImGui::TextColored(ImVec4(.5f, .3f, .4f, 1.f), "Timestamp Period: %.3f ns", timestampPeriod);

#if STATS_ENABLED
				// Timestamps are gathered regardless of pipeline stats:
			    lastFrameDurationMs =          glm::mix(lastFrameDurationMs         , mLastFrameDuration             * 1e-6 * timestampPeriod, 0.05);
				lastClearDuration =            glm::mix(lastClearDuration           , mLastClearDuration             * 1e-6 * timestampPeriod, 0.05);
				lastPatchLodDuration =         glm::mix(lastPatchLodDuration        , mLastPatchLodDuration          * 1e-6 * timestampPeriod, 0.05);
				lastPatchToTileDuration =      glm::mix(lastPatchToTileDuration     , mLastPatchToTileDuration       * 1e-6 * timestampPeriod, 0.05);
				lastRenderduration =           glm::mix(lastRenderduration          , mLastRenderDuration            * 1e-6 * timestampPeriod, 0.05);
				lastBeginToRenderEndDuration = glm::mix(lastBeginToRenderEndDuration, mLastBeginToParametricDuration * 1e-6 * timestampPeriod, 0.05);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Total frame time       : %.3lf ms (timer queries)", lastFrameDurationMs);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Clearing took          : %.3lf ms (timer queries)", lastClearDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Patch LOD step took    : %.3lf ms (timer queries)", lastPatchLodDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Patch->tile step took  : %.3lf ms (timer queries)", lastPatchToTileDuration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Rendering took         : %.3lf ms (timer queries)", lastRenderduration);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Frame begin->render end: %.3lf ms (timer queries)", lastBeginToRenderEndDuration);

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

#ifdef EXTRA_3D_MODEL
				if (mExtra3DModelBeginIndex >= 0) {
				    ImGui::Separator();
				    ImGui::Checkbox("Render " EXTRA_3D_MODEL, &mRenderExtra3DModel);
				}
#endif

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

				//ImGui::Separator();
				//if (mUseNvPipeline.has_value()) {
				//	int choice = mUseNvPipeline.value() ? 1 : 0;
				//	ImGui::Combo("Pipeline", &choice, "VK_EXT_mesh_shader\0VK_NV_mesh_shader\0");
				//	mUseNvPipeline = (choice == 1);
				//	ImGui::Separator();
				//}

				// Some parameters to pass to the GPU:
                ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "Pipeline Config:");
                ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "PxFill shader config params (besides ^ screen threshold)");
					ImGui::Indent();
                    ImGui::PushItemWidth(imGuiWindowWidth * 0.6f);
					ImGui::SliderFloat2("target resolution scaler", mPxFillPatchTargetResolutionScaler.data(), 0.25f, 2.5f, "%.3f", ImGuiSliderFlags_Logarithmic);
					ImGui::SliderFloat2("u/v param shift (in % of u/v deltas)", mPxFillParamShift.data(), 0.0f, 1.0f);
					ImGui::SliderFloat2("u/v param stretch (in % of u/v deltas)", mPxFillParamStretch.data(), 0.0f, 1.0f);
					ImGui::PopItemWidth();
					ImGui::Unindent();
				ImGui::Checkbox("Frustum Culling ON", &mFrustumCullingOn);
				bool rasterPipesNeedRecreation = ImGui::Checkbox("Backface Culling ON", &mBackfaceCullingOn);
				rasterPipesNeedRecreation =      ImGui::Checkbox("Disable Color Attachment Output" , &mDisableColorAttachmentOut) || rasterPipesNeedRecreation;

				ImGui::Text("[F6] ... toggle wireframe mode (currently %s)", mWireframeModeOn ? "ON" : "OFF");

				// Just some Debug stuff:
				ImGui::Separator();
				ImGui::Text("DEBUG SLIDERS:");
                ImGui::PushItemWidth(imGuiWindowWidth * 0.6f);
				ImGui::SliderFloat("LERP SH <-> Sphere (aka float dbg slider #1)", &mDebugSliders[0], 0.0f, 1.0f);
				ImGui::SliderInt("SH Band           (aka int dbg slider #1)", &mDebugSlidersi[0],  0, 31);
				ImGui::SliderInt("SH Basis Function (aka int dbg slider #2)", &mDebugSlidersi[1], -mDebugSlidersi[0], mDebugSlidersi[0]);
				ImGui::Text("##lololo");
				ImGui::SliderFloat("Terrain height (aka float dbg slider #2)", &mDebugSliders[1], 0.0f, 10.0f);
				ImGui::PopItemWidth();

				// Automatic performance measurement, camera flight:
				ImGui::Separator();
				ImGui::Checkbox("Start performance measurement 4s", &mStartMeasurement);
				ImGui::SliderFloat("Camera distance from origin", &mDistanceFromOrigin, 5.0f, 100.0f, "%.0f");
				ImGui::Checkbox("Move camera in steps during measurements", &mMoveCameraDuringMeasurements);
				ImGui::SliderInt("#steps to move the camera closer/away", &mMeasurementMoveCameraSteps, 0, 10);
				ImGui::DragFloat("Delta by how much to move the camera with each step", &mMeasurementMoveCameraDelta, 0.1f);

				ImGui::Separator();
                if (ImGui::Checkbox("Animations paused (freeze time)", &mAnimationPaused)) {
                    if (mAnimationPaused) {
                        mAnimationPauseTime = time().absolute_time();
                    }
                    else {
						mTimeToSubtract += time().absolute_time() - mAnimationPauseTime;
                    }
                }

				ImGui::Checkbox("Use individual patch resolution during px fill", &mUseMaxPatchResolutionDuringPxFill);

				ImGui::End();

				if (mGatherPipelineStats) {
				    ImGui::Begin("Stats and Counters");
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
					ImGui::Text(std::format("{:12L} pixel fill patches spawned", mNumPxFillPatchesCreated).c_str());
					ImGui::End();
				}

				// Do we need to recreate the vertex pipe?
				if (rasterPipesNeedRecreation) {
					auto newVertexPipe = create_vertex_pipe();
					std::swap(*mVertexPipeline, *newVertexPipe); // new pipe is now old pipe
					context().main_window()->handle_lifetime(std::move(newVertexPipe));

					{
						auto newTessPipePxFill = create_tess_pipe(
							"shaders/px-fill-tess/patch_ready.vert", 
							"shaders/px-fill-tess/patch_set.tesc", 
							"shaders/px-fill-tess/patch_go.tese",
							push_constant_binding_data{shader_type::all, 0, sizeof(patch_into_tess_push_constants)}
						);
						std::swap(*mTessPipelinePxFill, *newTessPipePxFill);
						context().main_window()->handle_lifetime(std::move(newTessPipePxFill));

						auto newTessPipePxFillWire = context().create_graphics_pipeline_from_template(mTessPipelinePxFill.as_reference(), [](graphics_pipeline_t& p) {
							p.rasterization_state_create_info().setPolygonMode(vk::PolygonMode::eLine);
						});
						std::swap(*mTessPipelinePxFillWireframe, *newTessPipePxFillWire);
						context().main_window()->handle_lifetime(std::move(newTessPipePxFillWire));
					}

					{
						auto newTessPipePxFillSS = create_tess_pipe(
							"shaders/px-fill-tess/patch_ready.vert", 
							"shaders/px-fill-tess/patch_set.tesc", 
							"shaders/px-fill-tess/patch_go.tese",
							push_constant_binding_data{shader_type::all, 0, sizeof(patch_into_tess_push_constants)}
						);
						std::swap(*mTessPipelinePxFillSS, *newTessPipePxFillSS);
						context().main_window()->handle_lifetime(std::move(newTessPipePxFillSS));

						auto newTessPipePxFillWireSS = context().create_graphics_pipeline_from_template(mTessPipelinePxFillSS.as_reference(), [](graphics_pipeline_t& p) {
							p.rasterization_state_create_info().setPolygonMode(vk::PolygonMode::eLine);
						});
						std::swap(*mTessPipelinePxFillWireframeSS, *newTessPipePxFillWireSS);
						context().main_window()->handle_lifetime(std::move(newTessPipePxFillWireSS));
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
		if (input().key_pressed(key_code::f6)) {
			mWireframeModeOn = !mWireframeModeOn;
		}

		if (input().key_pressed(key_code::u)) {
			mWhatToCopyToBackbuffer = 0;
        }
		if (input().key_pressed(key_code::h)) {
			mWhatToCopyToBackbuffer = 1;
        }
		if (input().key_pressed(key_code::b)) {
			mWhatToCopyToBackbuffer = 2;
        }
		if (input().key_pressed(key_code::f)) {
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
		const float MeasureSecsPerStep = TEST_DURATION_PER_STEP;
		if (mStartMeasurement) {
#if TEST_MODE_ON
			for (int ii = 0; ii < mParametricObjects.size(); ++ii) {
				mParametricObjects[ii].set_enabled(ii == TEST_ENABLE_OBJ_IDX);
			}
			fill_object_data_buffer();
			mConstInnerTessLevel = static_cast<float>(TEST_INNER_TESS_LEVEL);
			mConstOuterTessLevel = static_cast<float>(TEST_OUTER_TESS_LEVEL);
			mUseMaxPatchResolutionDuringPxFill = 1 == TEST_USEINDIVIDUAL_PATCH_RES;
			mRenderExtra3DModel = 1 == TEST_ENABLE_3D_MODEL;
			mWhatToCopyToBackbuffer = TEST_TARGET_IMAGE;
			mGatherPipelineStats = 1 == TEST_GATHER_PIPELINE_STATS;
			mPxFillPatchTargetResolutionScaler[0] = POINT_RENDERIN_PATCH_RES_S_X;
			mPxFillPatchTargetResolutionScaler[1] = POINT_RENDERIN_PATCH_RES_S_Y;
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
				std::string whichMethod = "TODO: not implemented yet";
				LOG_INFO_EM(std::format("{} frames rendered during {} sec. measurement time frame with {} method.", std::get<int>(mMeasurementFrameCounters.back()), mMeasurementEndTime - mMeasurementStartTime, whichMethod));
				LOG_INFO("The following settings were active:");
#if STATS_ENABLED
				LOG_INFO(std::format(" - mGatherPipelineStats: {}", mGatherPipelineStats));
#else
				LOG_INFO(            " - mGatherPipelineStats not included in build (!STATS_ENABLED)");
#endif
//                switch (mRenderingMethod) {
//                case rendering_method::point_rendered: 
//					LOG_INFO(std::format(" - Use individual patch resolution during px fill: {}", mUseMaxPatchResolutionDuringPxFill));
//					LOG_INFO(std::format(" - Adaptive pixel fill enabled:                    {}", mAdaptivePxFill));
//					LOG_INFO(std::format(" - PX_FILL_LOCAL_FB enabled: {}", (0 != PX_FILL_LOCAL_FB)));
//#if PX_FILL_LOCAL_FB
//					LOG_INFO(std::format("   - Tile size:     {}x{}", PX_FILL_LOCAL_FB_TILE_SIZE_X, PX_FILL_LOCAL_FB_TILE_SIZE_Y));
//					LOG_INFO(std::format("   - Tile factor:   {}x{}", TILE_FACTOR_X, TILE_FACTOR_Y));
//					LOG_INFO(std::format("   - Local FB size: {}x{}", LOCAL_FB_X, LOCAL_FB_Y));
//
//#endif
//                    break;
//                case rendering_method::tessellated_rasterized:
//                case rendering_method::tessellated_rasterized_wireframe:
//					LOG_INFO(std::format(" - Constant Inner Tessellation Level: {}", mConstInnerTessLevel));
//					LOG_INFO(std::format(" - Constant Outer Tessellation Level: {}", mConstOuterTessLevel));
//					LOG_INFO(std::format(" - Use individual patch resolution during px fill: {}", mUseMaxPatchResolutionDuringPxFill));
//					LOG_INFO(std::format(" - MSAA ENABLED {}, namely: {}", (0 != MSAA_ENABLED), vk::to_string(SAMPLE_COUNT)));
//					break;
//                }
				LOG_INFO("Measurement results (elapsed time, camera distance, unique pixels (avg.), num patches out to render (avg.), FPS):");
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
				std::get<2>(mMeasurementFrameCounters[curIdx]) += mLastBeginToParametricDuration - mLastClearDuration;
				std::get<3>(mMeasurementFrameCounters[curIdx]) += mLastPatchLodDuration;
#else
				std::get<2>(mMeasurementFrameCounters[curIdx]) += mCounterValues[1];
				std::get<3>(mMeasurementFrameCounters[curIdx]) += mNumPxFillPatchesCreated;
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
	                descriptor_binding(3, 2, mPatchLodCountBuffer->as_storage_buffer())
#if DRAW_PATCH_EVAL_DEBUG_VIS
					, descriptor_binding(4, 0, mCombinedAttachmentView->as_storage_image(layout::general))
#if STATS_ENABLED
					, descriptor_binding(4, 1, mHeatMapImageView->as_storage_image(layout::general))
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
			})
		);
	}

	void render() override
	{
		using namespace avk;
		std::chrono::steady_clock::time_point renderBegin = std::chrono::steady_clock::now();

		auto mainWnd = context().main_window();
		//auto resolution = mainWnd->resolution();
		auto resolution = glm::uvec2(mFramebuffer->image_at(0).width(), mFramebuffer->image_at(0).height());
		auto inFlightIndex = mainWnd->current_in_flight_index();

		glm::mat4 projMat = mQuakeCam.is_enabled() ? mQuakeCam.projection_matrix() : mOrbitCam.projection_matrix();

		frame_data_ubo uboData;
		if (mQuakeCam.is_enabled()) {
            uboData.mViewMatrix        = mQuakeCam.view_matrix();
            uboData.mViewProjMatrix    = mQuakeCam.projection_and_view_matrix();
			uboData.mInverseProjMatrix = glm::inverse(mQuakeCam.projection_matrix());
		}
        else {
            uboData.mViewMatrix        = mOrbitCam.view_matrix();
            uboData.mViewProjMatrix    = mOrbitCam.projection_and_view_matrix();
			uboData.mInverseProjMatrix = glm::inverse(mOrbitCam.projection_matrix());
        }
        uboData.mResolutionAndCulling                      = glm::uvec4(resolution, mBackfaceCullingOn ? 1u : 0u, 0u);
        uboData.mDebugSliders                              = mDebugSliders;
        uboData.mDebugSlidersi                             = mDebugSlidersi;
		uboData.mHeatMapEnabled                            = mWhatToCopyToBackbuffer == 1 || mWhatToCopyToBackbuffer == 2 ? VK_TRUE : VK_FALSE;
        uboData.mUseMaxPatchResolutionDuringPxFill         = mUseMaxPatchResolutionDuringPxFill ? VK_TRUE : VK_FALSE;
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
			mLastClearDuration = timers[1] - timers[0];
			mLastPatchLodDuration = timers[2] - timers[1];
			mLastPatchToTileDuration = timers[3] - timers[2];
			mLastRenderDuration = timers[4] - timers[3];
			mLastBeginToParametricDuration = timers[4] - timers[0];
			mLastTimestamp = timers[3];

			if (mGatherPipelineStats) {
                mPipelineStats = mPipelineStatsPool->get_results<uint64_t, 3>(inFlightIndex, 1, vk::QueryResultFlagBits::e64);
            }
		}

		// Read the counter values:
		if (mGatherPipelineStats) {
			VkDrawIndirectCommand pxFillCountBufferContents;
			std::array<glm::uvec4, MAX_PATCH_SUBDIV_STEPS> patchLodCountBufferContents;
			context().record_and_submit_with_fence({
				mCountersSsbo->read_into(mCounterValues.data(), 0),
		        mIndirectPxFillCountBuffer->read_into(&pxFillCountBufferContents, 0),
			    mPatchLodCountBuffer->read_into(patchLodCountBufferContents.data(), 0)
			}, *mQueue)->wait_until_signalled();
			mNumPxFillPatchesCreated = pxFillCountBufferContents.instanceCount;
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
		graphics_pipeline& tessPipePxFillToBeUsed = mWireframeModeOn ? mTessPipelinePxFillWireframe : mTessPipelinePxFill;
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
#if PX_FILL_LOCAL_FB && SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
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
				
			// 2) LOD Stage:
            lodStageCommands,
		
            sync::global_memory_barrier(stage::compute_shader + access::memory_write >> stage::all_graphics + access::memory_read),
#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 2, stage::compute_shader),
			mTimestampPool->write_timestamp(firstQueryIndex + 3, stage::compute_shader),
#endif

			// 3) Render patches:
			command::render_pass(mRenderpass.as_reference(), mFramebuffer.as_reference(), command::gather(

				// 3.1) Render tessellated patches into framebuffer (whether or not that is super sampled).
                command::bind_pipeline(tessPipePxFillToBeUsed.as_reference()),
				command::bind_descriptors(tessPipePxFillToBeUsed->layout(), mDescriptorCache->get_or_create_descriptor_sets({
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
				    descriptor_binding(3, 2, mIndirectPxFillCountBuffer->as_storage_buffer())
				})),
					
				command::push_constants(tessPipePxFillToBeUsed->layout(), patch_into_tess_push_constants{ mConstOuterTessLevel, mConstInnerTessLevel }),

				command::draw_vertices_indirect(mIndirectPxFillCountBuffer.as_reference(), 0, sizeof(VkDrawIndirectCommand), 1u), // <-- Exactly ONE draw (but potentially a lot of instances)
				// 3.2) Render extra 3D models into the same framebuffer (super-sampled or not)

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
				command::many_n_times(static_cast<int>(mDrawCalls.size()-mExtra3DModelBeginIndex), [this](int i) {
					i += mExtra3DModelBeginIndex;
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

			)),
#if STATS_ENABLED
			mTimestampPool->write_timestamp(firstQueryIndex + 4, stage::fragment_shader),
#endif
			// 3.3) Color attachment has been resolved into non-super sampled image
			// 3.4) TODO: Copy resolved (color + depth) --into--> combined attachment 

			sync::global_memory_barrier(stage::all_commands + access::memory_write >> stage::compute_shader + access::memory_read), // TODO: Barrier too heavy

			command::bind_pipeline(mCopyToCombinedAttachmentPipe.as_reference()),
				command::bind_descriptors(mCopyToCombinedAttachmentPipe->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					descriptor_binding(0, 0, mCombinedAttachmentView->as_storage_image(layout::general)),
					descriptor_binding(1, 0, mColorAttachmentView->as_sampled_image(layout::shader_read_only_optimal)),
					descriptor_binding(1, 1, mDepthAttachmentView->as_sampled_image(layout::shader_read_only_optimal))
				})),
			command::dispatch((resolution.x + 15u) / 16u, (resolution.y + 15u) / 16u, 1u),

			sync::global_memory_barrier(stage::compute_shader + access::memory_write >> stage::compute_shader + access::memory_read),

			// 3.5) Perform point rendering into combined attachment 

















#if PX_FILL_LOCAL_FB && SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
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
				mTimestampPool->write_timestamp(firstQueryIndex + 3, stage::compute_shader),
#endif

				// 3rd pass compute shader: px fill PER DRAW PACKAGE
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
#if PX_FILL_LOCAL_FB && SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
					, descriptor_binding(4, 0, mTilePatchesBuffer->as_storage_buffer())
#endif
				})),

				command::push_constants(mPxFillComputePipe->layout(), pass3_push_constants{
					mGatherPipelineStats ? VK_TRUE : VK_FALSE,  /* padding: */ 0,
					glm::vec2{ mPxFillPatchTargetResolutionScaler[0], mPxFillPatchTargetResolutionScaler[1] },
					glm::vec2{ mPxFillParamShift[0], mPxFillParamShift[1] },
					glm::vec2{ mPxFillParamStretch[0], mPxFillParamStretch[1] },
				}),

#if PX_FILL_LOCAL_FB
				command::dispatch((resolution.x + PX_FILL_LOCAL_FB_TILE_SIZE_X - 1) / PX_FILL_LOCAL_FB_TILE_SIZE_X, (resolution.y + PX_FILL_LOCAL_FB_TILE_SIZE_Y - 1) / PX_FILL_LOCAL_FB_TILE_SIZE_Y, 1),
				//command::dispatch(25, 25, 1)
#else
				command::dispatch_indirect(mIndirectPxFillCountBuffer, sizeof(VkDrawIndirectCommand::vertexCount)), // => in order to use the instanceCount!
#endif

#if STATS_ENABLED
				mTimestampPool->write_timestamp(firstQueryIndex + 4, stage::compute_shader),
#endif















#if STATS_ENABLED
			    commandsEndStats,
#else
			    // Fascinating: Without that following barrier, we get rendering artifacts in the ABSENCE of the timestamp write!
			    // TODO:            ^ investigate further!
			    sync::global_memory_barrier(stage::fragment_shader | stage::compute_shader >> stage::compute_shader, access::shader_write >> access::shader_read),
#endif
		
			// 3.6) Copy combined attachment -> back buffer
			



			// That worked:
			//sync::image_memory_barrier(context().main_window()->current_backbuffer()->image_at(0),  // Window's back buffer's color attachment
			//		                    stage::none          >>   stage::blit,
			//		                    access::none         >>   access::transfer_write)
			//	.with_layout_transition(layout::undefined   >>   layout::transfer_dst),             // Don't care about the previous layout

			//blit_image(mFramebuffer->image_at(0), layout::transfer_src, context().main_window()->current_backbuffer()->image_at(0), layout::transfer_dst, 
			//			vk::ImageAspectFlagBits::eColor, vk::Filter::eLinear),

			//sync::image_memory_barrier(context().main_window()->current_backbuffer()->image_at(0),  // Window's back buffer's color attachment
			//		                    stage::blit                    >>   stage::color_attachment_output, // <-- for ImGui, which draws afterwards
			//		                    access::transfer_write         >>   access::color_attachment_read | access::color_attachment_write)
			//	.with_layout_transition(       layout::transfer_dst   >>   layout::color_attachment_optimal)


			// Copy from Uint64 image into back buffer:
			sync::image_memory_barrier(context().main_window()->current_backbuffer()->image_at(0),  // Window's back buffer's color attachment
					                    stage::none          >>   stage::compute_shader,
					                    access::none         >>   access::shader_storage_write)
				.with_layout_transition(layout::undefined   >>   layout::general),                  // Don't care about the previous layout
						
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


		);

		// Now, with all the commands recorded => submit all the work to the GPU:
		context().record(gatheredCommands)
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mQueue)
			// Do not start to render before the image has become available:
			.waiting_for(imageAvailableSemaphore >> stage::color_attachment_output)
			.submit();
		
		mainWnd->handle_lifetime(std::move(cmdBfr));

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
	avk::compute_pipeline mPatchLodComputePipe;
	avk::compute_pipeline mPxFillComputePipe;
#if PX_FILL_LOCAL_FB && SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
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
	std::vector<uint32_t> mKnitYarnObjectDataIndices;

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
	glm::ivec4                 mDebugSlidersi   = {8, 0, 0, 0};

#if STATS_ENABLED
    avk::query_pool mTimestampPool;
	uint64_t mLastTimestamp = 0;
	uint64_t mLastClearDuration = 0;
	uint64_t mLastPatchLodDuration = 0;
	uint64_t mLastPatchToTileDuration = 0;
	uint64_t mLastRenderDuration = 0;
	uint64_t mLastBeginToParametricDuration = 0;
	uint64_t mLastFrameDuration = 0;

	avk::query_pool mPipelineStatsPool;
	std::array<uint64_t, 3> mPipelineStats;
#endif
	
	avk::renderpass  mRenderpass;
	avk::framebuffer mFramebuffer;
	avk::image_view  mColorAttachmentView;
	avk::image_view  mDepthAttachmentView;
	avk::image_view  mCombinedAttachmentView;
    avk::compute_pipeline mCopyToBackufferPipe;
    avk::compute_pipeline mClearCombinedAttachmentPipe;
    avk::compute_pipeline mCopyToCombinedAttachmentPipe;
	avk::buffer mObjectDataBuffer;
	avk::buffer mPatchLodBufferPing;
	avk::buffer mPatchLodBufferPong;
	avk::buffer mPatchLodCountBuffer;
#if PX_FILL_LOCAL_FB && SEPARATE_PATCH_TILE_ASSIGNMENT_PASS
	avk::buffer mTilePatchesBuffer;
#endif
	avk::buffer mIndirectPxFillParamsBuffer;
	avk::buffer mIndirectPxFillCountBuffer;

    avk::graphics_pipeline mVertexPipeline;
    avk::graphics_pipeline mTessPipelinePxFill;
    avk::graphics_pipeline mTessPipelinePxFillSS;
    avk::graphics_pipeline mTessPipelinePxFillWireframe;
    avk::graphics_pipeline mTessPipelinePxFillWireframeSS;
	float mConstOuterTessLevel = 16.0;
	float mConstInnerTessLevel = 16.0;

#if STATS_ENABLED
    bool mGatherPipelineStats      = true;
	avk::image_view  mHeatMapImageView;
#else
    bool mGatherPipelineStats      = false;
#endif
	int mWhatToCopyToBackbuffer = 0; // 0 ... rendering output, 1 ... heat map, 2 ... generate both, but copy rendering output

	std::vector<glm::vec3> mSpherePositions;

	bool mStartMeasurement = false;
	float mDistanceFromOrigin = 20.f;
	bool mMeasurementInProgress = false;
	double mMeasurementStartTime = 0.0;
	double mMeasurementEndTime = 0.0;
	std::vector<std::tuple<double, float, uint64_t, uint64_t, int>> mMeasurementFrameCounters;
	bool mMoveCameraDuringMeasurements = true;
	int mMeasurementMoveCameraSteps = 6;
	float mMeasurementMoveCameraDelta = -5.0f;
	int mMeasurementIndexLastFrame = -1;

	avk::buffer mCountersSsbo;
	std::array<uint32_t, 4> mCounterValues;
	std::array<uint32_t, MAX_PATCH_SUBDIV_STEPS> mPatchesCreatedPerLevel;
	uint32_t mNumPxFillPatchesCreated;

	// Other, global settings, stored in common UBO:
    bool m2ndPassEnabled = false;
    bool mAdaptivePxFill = false;
    bool mUseMaxPatchResolutionDuringPxFill = false;

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
	std::array<float, 2> mPxFillPatchTargetResolutionScaler {{ 1.2f, 1.2f }};
	std::array<float, 2> mPxFillParamShift {{ 0.0f, 0.0f }};
	std::array<float, 2> mPxFillParamStretch {{ 0.0f, 0.0f }};

	int mNumMaterials = 1;
	int mExtra3DModelBeginIndex = -1;
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
		ui.set_custom_font("assets/JetBrainsMono-Regular.ttf");

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
