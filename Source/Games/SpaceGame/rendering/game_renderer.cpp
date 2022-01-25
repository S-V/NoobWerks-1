#include "stdafx.h"
#pragma hdrstop

#include <bx/radixsort.h>

#include <Base/Util/Cubemap_Util.h>

#include <Core/Util/Tweakable.h>
#include <Core/Util/ScopedTimer.h>

#include <Graphics/graphics_shader_system.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Private/RenderSystemData.h>

#include <Engine/WindowsDriver.h>	// InputState
#include <Engine/Engine.h>	// NwEngineSettings
#include <Engine/Model.h>


#include "game_app.h"
#include "game_settings.h"
#include "world/world.h"
#include "human_player.h"
#include "game_renderer.h"
#include "physics/aabb_tree.h"


namespace
{
	static ProxyAllocator& rendererHeap() { return MemoryHeaps::renderer(); }
}


tbDEFINE_VERTEX_FORMAT(BillboardStarVertex);
mxBEGIN_REFLECTION( BillboardStarVertex )
	mxMEMBER_FIELD( sphere_in_view_space ),
	mxMEMBER_FIELD( velocity_and_life ),
mxEND_REFLECTION
void BillboardStarVertex::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float4, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Float4, NwAttributeUsage::TexCoord, 0)
		.end();
}


/*
-----------------------------------------------------------------------------
	SgRenderer
-----------------------------------------------------------------------------
*/
SgRenderer::SgRenderer()
{
	h_skybox_cubemap.setNil();
	_is_drawing_debug_lines = false;
}

ERet SgRenderer::initialize(
							const NwEngineSettings& engine_settings
							, const RrGlobalSettings& renderer_settings
							, NwClump* scene_clump
							)
{
	_scene_clump = scene_clump;

	//
	Rendering::initializeResourceLoaders( rendererHeap() );

	//
	_render_system = NwRenderSystemI::create(rendererHeap());
	mxDO(_render_system->initialize(
		engine_settings.window.width, engine_settings.window.height
		, renderer_settings
		, scene_clump
		));

	//
	mxDO(_GenerateSkyboxCubemap());

	//
	mxREFACTOR("move to engine!");
	NwModel_::RegisterRenderCallback(_render_system->_render_callbacks);

	//
	mxDO(_render_system->_debug_renderer.reserveSpace( mxMiB(1), mxMiB(1), mxKiB(64) ));

	//
	_gizmo_renderer.BuildGeometry();

	mxDO(particle_renderer.Initialize(
		engine_settings
		, renderer_settings
		, scene_clump
		));

	mxZERO_OUT(stats);

	should_highlight_visible_ships = false;

	return ALL_OK;
}

void SgRenderer::shutdown()
{
	GL::DeleteTexture(h_skybox_cubemap);
	h_skybox_cubemap.setNil();

	particle_renderer.Shutdown();

	//
	_render_system->shutdown();
	NwRenderSystemI::destroy( _render_system );
	_render_system = nil;

	Rendering::shutdownResourceLoaders( rendererHeap() );

	//
	_scene_clump = nil;
}


void SgRenderer::BeginFrame(
	const NwTime& game_time
	, const SgWorld& world
	, const SgAppSettings& game_settings
	, const SgUserSettings& user_settings
	, const SgGameplayMgr& gameplay
	)
{
	//
	const NwFloatingOrigin floating_origin = world.spatial_database->getFloatingOrigin();

	//
	const SgHumanPlayer& player = gameplay.player;

	//
	NwCameraView & camera_view = scene_view;
	{
		if(gameplay.spaceship_controller.IsControllingSpaceship())
		{
			const SgFollowingCamera& orbit_cam = gameplay.spaceship_controller._following_camera;
			camera_view.eye_pos_in_world_space = V3d::fromXYZ(orbit_cam.cam_eye_pos);
			camera_view.eye_pos = orbit_cam.cam_eye_pos;

			camera_view.right_dir = orbit_cam.cam_right_dir;
			camera_view.look_dir = orbit_cam.cam_look_dir;
		}
		else
		{

#if 0
			camera_view.eye_pos = floating_origin.relative_eye_position;
			camera_view.eye_pos_in_world_space = player.camera._eye_position;
#else
			//camera_view.eye_pos_in_world_space = player.camera._eye_position;
			camera_view.eye_pos_in_world_space = V3d::fromXYZ(player.GetEyePosition());

			camera_view.eye_pos = player.GetEyePosition();
#endif

			const NwFlyingCamera& camera = player.camera;
			camera_view.right_dir = camera.m_rightDirection;
			camera_view.look_dir = camera.m_look_direction;
		}

		// repair near plane
		camera_view.near_clip = maxf( game_settings.renderer.camera_near_clip, MIN_NEAR_CLIP_PLANE_DIST );

		camera_view.far_clip = game_settings.renderer.camera_far_clip;

		//
		const UInt2 window_size = WindowsDriver::getWindowSize();
		camera_view.screen_width = window_size.x;
		camera_view.screen_height = window_size.y;
	}
	camera_view.recomputeDerivedMatrices();

	//
	_render_system->BeginFrame(
		camera_view
		, game_settings.renderer
		, game_time
		);

	//
	_PrepareDebugLineRenderer();
}

namespace
{
	// Return random noise in the range [0.0, 1.0], as a function of x.
	mxFORCEINLINE float Noise3D( const V3f& p, NwRandom & rng )
	{
		union {
			struct {
				int	x, y, z;
			};
			V3f		as_vec3;
		} u;

		u.as_vec3 = p;

		//
		NwRandom	rng2(
			(u.x ^ u.y ^ u.z)
			^ rng.RandomInt()
			);

		return rng2.RandomFloat();
	}

	// https://www.shadertoy.com/view/Md2SR3
	// Convert Noise3() into a "star field" by stomping everthing below fThreshhold to zero.
	mxFORCEINLINE float NoisyStarField( const V3f& vSamplePos, float fThreshhold, NwRandom & rng )
	{
		float StarVal = Noise3D( vSamplePos, rng );
		if ( StarVal >= fThreshhold )
			StarVal = pow( (StarVal - fThreshhold)/(1.0 - fThreshhold), 6.0 );
		else
			StarVal = 0.0;
		return StarVal;
	}
}

ERet SgRenderer::_GenerateSkyboxCubemap(int seed)
{
	ScopedTimer	timer;
	//
	enum {
		//TODO: high-res and mipmaps
		CUBEMAP_FACE_RES = 512//1024
	};

	const NwDataFormat::Enum cubemap_tex_format = NwDataFormat::R8_UNORM;
	const UINT bpp = NwDataFormat::BitsPerPixel(cubemap_tex_format);
	const UINT bytes_per_pixel = BITS_TO_BYTES(bpp);

	const size_t cubemap_face_data_size = (CUBEMAP_FACE_RES * CUBEMAP_FACE_RES) * bytes_per_pixel;
	const size_t raw_texture_data_size = cubemap_face_data_size * 6;

	//
	const GL::Memory* tex_data_mem = GL:: allocate(
		sizeof(TextureHeader_d) + raw_texture_data_size
		, EFFICIENT_ALIGNMENT
		);
	mxENSURE(tex_data_mem, ERR_OUT_OF_MEMORY, "");

	//
	TextureHeader_d *	tex_hdr = (TextureHeader_d*) tex_data_mem->data;
	tex_hdr->magic = TEXTURE_FOURCC;
	tex_hdr->size = raw_texture_data_size;
	tex_hdr->width = CUBEMAP_FACE_RES;
	tex_hdr->height = CUBEMAP_FACE_RES;
	tex_hdr->type = TEXTURE_CUBEMAP;
	tex_hdr->format = cubemap_tex_format;
	tex_hdr->num_mips = 1;
	tex_hdr->array_size = 6;

	//
	NwRandom	rng(seed);

	{
		char *dst_cubemap_face_data = (char *) ( tex_hdr + 1 );

		for( UINT cubemap_face = 0; cubemap_face < 6; cubemap_face++ )
		{
			U8* dst_face_texels = (U8*) dst_cubemap_face_data;

			//
			const float face_res = float(CUBEMAP_FACE_RES);
			const float inv_face_res = 1.0f/face_res;

			float yyf = 1.0f;
			for( int iY = 0; iY < CUBEMAP_FACE_RES; iY++, yyf+=2.0f )
			{
				float xxf = 1.0f;
				for( int iX = 0; iX < CUBEMAP_FACE_RES; iX++, xxf+=2.0f )
				{
					// From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
					// Ref: uu = 2.0*(xxf+0.5)/faceSize - 1.0;
					//      vv = 2.0*(yyf+0.5)/faceSize - 1.0;
					const float uu = xxf*inv_face_res - 1.0f;
					const float vv = yyf*inv_face_res - 1.0f;

					const V3f direction = Cubemap_Util::texelCoordToVec( uu, vv, cubemap_face );

					//
					float StarFieldThreshhold = 0.97;
					float star_intensity01 = NoisyStarField(
						direction
						, StarFieldThreshhold
						, rng
						);
					//
					dst_face_texels[ iY * CUBEMAP_FACE_RES + iX ] = (U8) (star_intensity01 * 255.0f);
				}//x
			}//y

			dst_cubemap_face_data = mxAddByteOffset(dst_cubemap_face_data, cubemap_face_data_size);
		}//for each cube face
	}

	//
	h_skybox_cubemap = GL::CreateTexture(
		tex_data_mem
		, NwTextureCreationFlags::defaults
		, "proc_skybox"
		);

	DEVOUT("Generated skybox cubemap (face res=%d, total size = %.3f KiB) in %d msec!"
		, CUBEMAP_FACE_RES
		, (float)raw_texture_data_size/mxKIBIBYTE
		, timer.ElapsedMilliseconds()
		);

	return ALL_OK;
}

ERet SgRenderer::_PrepareDebugLineRenderer()
{
	const NwCameraView& camera_view = scene_view;

	GL::NwRenderContext & render_context = GL::getMainRenderContext();

	//
	const U32 passIndex = _render_system->GetRenderPath()
		.getPassDrawingOrder(mxHASH_STR("DebugLines"));

	NwShaderEffect* shaderTech;
	mxDO(Resources::Load(shaderTech, MakeAssetID("auxiliary_rendering"), _scene_clump));
	_render_system->_debug_renderer.setTechnique(shaderTech);

	{
		GL::NwPushCommandsOnExitingScope	applyShaderCommands(
			render_context,
			GL::buildSortKey( passIndex, 0 )	// first
			);

		const M44f view_projection_matrixT = M44_Transpose(camera_view.view_projection_matrix);
		mxDO(shaderTech->BindCBufferData(
			render_context._command_buffer,
			view_projection_matrixT
			));

		mxDO(shaderTech->pushShaderParameters( render_context._command_buffer ));
	}

	_render_system->_debug_renderer.SetViewID( passIndex );

	return ALL_OK;
}

ERet SgRenderer::RenderFrame(
	const SgWorld& world
	, const NwTime& game_time
	, const SgGameplayMgr& gameplay
	, const SgPhysicsMgr& physics_mgr
	, const SgAppSettings& game_settings
	, const SgUserSettings& user_settings
	, const SgHumanPlayer& player
	)
{
	const NwCameraView& camera_view = scene_view;
	//
	GL::NwRenderContext & render_context = GL::getMainRenderContext();

	//
	_DrawWorld(
		world
		, camera_view
		, game_time
		, game_settings
		, render_context
		);

	//
	_DrawMisc(
		camera_view
		, game_settings.renderer
		, game_time
		);

	player.RenderFirstPersonView(
		scene_view
		, render_context
		, game_time
		, world.spatial_database
		);

	//
	const SgGameplayRenderData	gameplay_data = gameplay.GetRenderData();
	stats.num_objects_total = gameplay_data.spaceships._count;

	//
	{
		AllocatorI& scratchpad = MemoryHeaps::temporary();

		//
		ViewFrustum	view_frustum;
		view_frustum.extractFrustumPlanes_Generic(
			camera_view.eye_pos
			, camera_view.right_dir
			, camera_view.look_dir
			, camera_view.getUpDir()
			, camera_view.half_FoV_Y
			, camera_view.aspect_ratio
			, camera_view.near_clip
			, camera_view.far_clip
			);

		//
		CulledObjectsIDs	culled_objects_ids(scratchpad);

#if 1
		physics_mgr._aabb_tree->GatherObjectsInFrustum(
			view_frustum
			, Vector4_Load( camera_view.eye_pos.a )
			, culled_objects_ids
			);
#else
		physics_mgr.GatherObjectsInFrustum(
			view_frustum
			, culled_objects_ids
			, gameplay.GetShipHandleMgr()
			);
#endif

		// sort by object type, then by approx distance from the camera

		const U32 num_culled_objs = culled_objects_ids._count;
		stats.num_objects_visible = num_culled_objs;

		if(num_culled_objs)
		{
			// std::sort() is very slow in debug builds
			//std::sort(culled_objects_ids.begin(), culled_objects_ids.end());

			CulledObjectID*	temp_sort_items;
			mxTRY_ALLOC_SCOPED(temp_sort_items, num_culled_objs, scratchpad);

			bx::radixSort(
				culled_objects_ids._data
				, temp_sort_items
				, num_culled_objs
				);

			//
			_DrawVisibleShips(
				culled_objects_ids
				, camera_view
				, game_time
				, game_settings
				, gameplay_data
				, physics_mgr
				, render_context
				);

			//
			if(user_settings.ingame.gameplay.highlight_enemies)
			{
				_HighlightVisibleShips(
					culled_objects_ids
					, camera_view
					, game_time
					, game_settings
					, gameplay_data
					, physics_mgr
					, render_context
					);
			}
		}//if any obj is visible
	}

	//
	_HighlightSpaceshipThatCanBeControlledByPlayer();

	//
	_DrawProjectiles(
		gameplay_data.npc_projectiles
		, camera_view
		, game_time
		, game_settings
		, gameplay_data
		, world
		, render_context
		);

	_DrawProjectiles(
		gameplay_data.player_projectiles
		, camera_view
		, game_time
		, game_settings
		, gameplay_data
		, world
		, render_context
		);

	////
	//this->submit_Star(
	//	world._main_star
	//	, player
	//	, camera_view
	//	, render_context
	//	);

	if( game_settings.player_torchlight_params.enabled )
	{
		TbDynamicPointLight	test_light;
		test_light.position = V3f::fromXYZ(player.camera._eye_position); //region_of_interest_position;
		test_light.radius = game_settings.player_torchlight_params.radius;
		test_light.color = game_settings.player_torchlight_params.color;
		world.spatial_database->AddDynamicPointLight(test_light);
	}

	//

	_BeginDebugLineDrawing(camera_view, render_context);


#if !GAME_CFG_RELEASE_BUILD

	DebugDrawWorld(
		world
		, world.spatial_database
		, camera_view
		, player
		, game_settings
		, game_settings.renderer
		, game_time
		, render_context
		);

	//
#if GAME_CFG_WITH_VOXELS
	//if( game_settings.voxels.engine.debug.flags & VX5::DbgFlag_Draw_LoD_Octree )
	{
		const NwFloatingOrigin floating_origin = world.spatial_database->getFloatingOrigin();

		world.debugDrawVoxels(
			floating_origin
			, camera_view
			);
	}
#endif

#endif // !GAME_CFG_RELEASE_BUILD

	//
	if(user_settings.developer.visualize_BVH)
	{
		physics_mgr._aabb_tree->DebugDraw(
			_render_system->_debug_renderer
			);
	}

	//
	_FinishDebugLineDrawing();

	return ALL_OK;
}

void SgRenderer::_DrawMisc(
	const NwCameraView& camera_view
	, const RrGlobalSettings& renderer_settings
	, const NwTime& game_time
	)
{
	//
	particle_renderer.AdvanceParticlesOncePerFrame(
		game_time
		);

	//
	burning_fires.UpdateOncePerFrame(
		game_time
		);

	explosions.UpdateOncePerFrame(game_time);

	//
	decals.AdvanceDecalsOncePerFrame(
		game_time
		);
}

ERet SgRenderer::_HighlightSpaceshipThatCanBeControlledByPlayer()
{
	SgGameApp& game_app = SgGameApp::Get();

	SgGameplayMgr& gameplay = game_app.gameplay;

	//
	const bool	the_player_is_controlling_a_spaceship = gameplay.spaceship_controller.IsControllingSpaceship();

	//
	if(!the_player_is_controlling_a_spaceship)
	{
		bool	enemy_ship_under_crosshair = false;

		if(gameplay.h_spaceship_under_crosshair.isValid())
		{
			const SgShip* spaceship_under_crosshair = gameplay.FindSpaceship(gameplay.h_spaceship_under_crosshair);
			if(!spaceship_under_crosshair) {
				// the ship must have been killed
				return ERR_OBJECT_NOT_FOUND;
			}

			const bool is_enemy_ship = spaceship_under_crosshair->flags & fl_ship_is_my_enemy;

			//
			SimdAabb	spaceship_aabb;
			game_app.physics_mgr.GetRigidBodyAABB(
				spaceship_aabb
				, spaceship_under_crosshair->rigid_body_handle
				);

			//
			TbPrimitiveBatcher& prim_batcher = game_app.renderer._render_system->_debug_renderer;
			prim_batcher.DrawAABB(
				SimdAabb_Get_AABBf(spaceship_aabb)
				, is_enemy_ship ? RGBAf::ORANGE : RGBAf::GREEN
				);

			//
			enemy_ship_under_crosshair = is_enemy_ship;
		}

		game_app.renderer.DrawCrosshair(enemy_ship_under_crosshair);
	}

	return ALL_OK;
}

void SgRenderer::_DrawWorld(
	const SgWorld& world
	, const NwCameraView& camera_view
	, const NwTime& game_time
	, const SgAppSettings& game_settings
	, GL::NwRenderContext & render_context
	)
{
	//
	this->_render_system->drawWorld(
		world.spatial_database
		, camera_view
		, game_settings.renderer
		, game_time
		);

	//
	this->particle_renderer.RenderParticlesIfAny(
		*this
		, world.spatial_database
		);

	//
	this->explosions.Render(
		*this->_render_system
		, world.spatial_database
		);

	//
	this->burning_fires.Render(
		*this->_render_system
		, world.spatial_database
		);

	//
	this->decals.RenderDecalsIfAny(
		*this->_render_system
		, world.spatial_database
		);

	//
	this->Submit_Skybox();
}


namespace
{
	struct SgShipInstanceData
	{
		Vector4	pos;
		Vector4	quat;
	};

	enum
	{
		MAX_INSTANCES_IN_DRAWCALL = LLGL_MAX_UNIFORM_BUFFER_SIZE / sizeof(SgShipInstanceData)
	};

	static ERet DrawSpaceshipsOfSameType(
		const TSpan<CulledObjectID> culled_objects_ids
		, const EGameObjType ship_type
		, const UINT mesh_lod
		, const SgGameplayRenderData& gameplay_data
		, const SgPhysicsMgr& physics_mgr
		, const NwCameraView& camera_view
		, GL::NwRenderContext & render_context
		)
	{
		const SgShipDef& ship_def = gameplay_data.ship_defs_by_ship_type[ ship_type ];
		
#if SG_USE_SDF_VOLUMES
		const HTexture ship_sdf_volume_texture = gameplay_data.ship_sdf_volume_textures_by_ship_type[ ship_type ];
#endif
		//
		const Rendering::NwRenderSystem& render_system = Rendering::NwRenderSystem::shared();

		const RenderPath& render_path = render_system.GetRenderPath();

		//
		const GL::LayerID	fill_gbuffer_pass_id = render_path
			.getPassDrawingOrder(Rendering::ScenePasses::FillGBuffer)
			;

		//
		NwShaderEffect* shader_effect = nil;
		mxDO(Resources::Load( shader_effect, MakeAssetID("draw_spaceships_near") ));

		//
#if SG_USE_SDF_VOLUMES
		mxDO(shader_effect->setResource(
			mxHASH_STR("t_sdf_volume"),
			GL::AsInput(ship_sdf_volume_texture)
			));
#endif

		//
		const NwShaderEffect::Variant shader_effect_variant = shader_effect->getDefaultVariant();

		//
		mxSTATIC_ASSERT_ISPOW2(MAX_MESH_LODS);
		//
		const NwMesh& mesh = *ship_def.lod_meshes[ mesh_lod & (MAX_MESH_LODS-1) ];


		//
		GL::Cmd_Draw	dip;

		dip.setup(
			shader_effect_variant.program_handle
			);

		dip.input_layout = mesh.vertex_format->input_layout_handle;
		dip.primitive_topology = mesh.topology;
		dip.use_32_bit_indices = mesh.flags & MESH_USES_32BIT_IB;

		dip.VB = mesh.geometry_buffers.VB->buffer_handle;
		dip.IB = mesh.geometry_buffers.IB->buffer_handle;

		//
		dip.instance_count = culled_objects_ids._count;

		dip.base_vertex = 0;
		dip.vertex_count = mesh.vertex_count;
		dip.start_index = 0;
		dip.index_count = mesh.index_count;

		IF_DEBUG dip.src_loc = GFX_SRCFILE_STRING;


		//
		GL::RenderCommandWriter	cmd_writer( render_context );


		//mxENSURE(culled_objects_ids._count <= MAX_INSTANCES_IN_DRAWCALL
		//	, ERR_TOO_MANY_OBJECTS
		//	, ""
		//	);
		mxASSERT(culled_objects_ids._count <= MAX_INSTANCES_IN_DRAWCALL);

		const U32 safe_instance_count = smallest(culled_objects_ids._count, MAX_INSTANCES_IN_DRAWCALL);

		//
		AllocatorI& instance_data_temp_allocator = MemoryHeaps::temporary();

		//
		const size_t instance_data_size = safe_instance_count * sizeof(SgShipInstanceData);
		SgShipInstanceData *const	instance_xforms = (SgShipInstanceData*) instance_data_temp_allocator.allocate(
			instance_data_size, LLGL_CBUFFER_ALIGNMENT
			);
		mxENSURE(instance_xforms, ERR_OUT_OF_MEMORY, "");

		//
		const StridedRBInstanceData rb_instances_data = physics_mgr.GetRBInstancesData();

		//
		for(UINT ii = 0; ii < safe_instance_count; ii++)
		{
			const CulledObjectID culled_obj_id = culled_objects_ids._data[ii];
			const UINT obj_index = culled_obj_id & CULLED_OBJ_INDEX_MASK;

			//
			const SgRBInstanceData& rb_inst = rb_instances_data.GetAt(obj_index);

			SgShipInstanceData& dst_inst_dat = instance_xforms[ii];

			dst_inst_dat.pos = rb_inst.pos;
			dst_inst_dat.pos.m128_i32[3] = ship_type;
			dst_inst_dat.quat = rb_inst.quat;
		}//for each instance

		//
		mxDO(cmd_writer.UpdateBuffer(
			render_system._cb_instance_matrices
			, instance_xforms
			, instance_data_size
			, &instance_data_temp_allocator
			, (MX_DEVELOPER ? "cb_instances" : nil)
			));

		//
		mxDO(cmd_writer.SetCBuffer(INSTANCE_CBUFFER_SLOT, render_system._cb_instance_matrices));

#if SG_USE_SDF_VOLUMES
		shader_effect->pushShaderParameters( render_context._command_buffer );
#endif

		//
		NwRenderState *	render_state;
		mxDO(Resources::Load(render_state, MakeAssetID("default")));
		cmd_writer.SetRenderState(*render_state);

		//
		cmd_writer.Draw(dip);

		//
		cmd_writer.submitCommandsWithSortKey(
			GL::buildSortKey( fill_gbuffer_pass_id, shader_effect_variant.program_handle, 0/*sort_key_from_depth*/ )
			);

		return ALL_OK;
	}
}//namespace


ERet SgRenderer::_DrawVisibleShips(
	const CulledObjectsIDs& culled_objects_ids
	, const NwCameraView& camera_view
	, const NwTime& game_time
	, const SgAppSettings& game_settings
	, const SgGameplayRenderData& gameplay_data
	, const SgPhysicsMgr& physics_mgr
	, GL::NwRenderContext & render_context
	)
{
	mxOPTIMIZE("multithreaded cmd buf gen");

	//
	U32	start_instance = 0;
	U32	num_instances = 1;
	U32	prev_obj_type_and_lod = (culled_objects_ids._data[0] >> CULLED_OBJ_LOD_SHIFT);

	//
	for(UINT ii = 1; ii < culled_objects_ids._count; ii++)
	{
		const U32 culled_obj_id = culled_objects_ids._data[ii];

		//
		const U32 curr_obj_type_and_lod = (culled_obj_id >> CULLED_OBJ_LOD_SHIFT);

		if(prev_obj_type_and_lod == curr_obj_type_and_lod
			&& num_instances < MAX_INSTANCES_IN_DRAWCALL)
		{
			++num_instances;
		}
		else
		{
			// flush
			const EGameObjType ship_type = EGameObjType(prev_obj_type_and_lod >> CULLED_OBJ_LOD_BITS);
			const UINT mesh_lod = prev_obj_type_and_lod & (MAX_MESH_LODS-1);

			DrawSpaceshipsOfSameType(
				TSpan<CulledObjectID>(culled_objects_ids._data + start_instance, num_instances)
				, ship_type
				, mesh_lod
				, gameplay_data
				, physics_mgr
				, camera_view
				, render_context
				);

			start_instance = ii;
			num_instances = 1;
			prev_obj_type_and_lod = curr_obj_type_and_lod;
		}
	}

	if(num_instances)
	{
		const EGameObjType ship_type = EGameObjType(prev_obj_type_and_lod >> CULLED_OBJ_LOD_BITS);
		const UINT mesh_lod = prev_obj_type_and_lod & (MAX_MESH_LODS-1);

		DrawSpaceshipsOfSameType(
			TSpan<CulledObjectID>(culled_objects_ids._data + start_instance, num_instances)
			, ship_type
			, mesh_lod
			, gameplay_data
			, physics_mgr
			, camera_view
			, render_context
			);
	}

	return ALL_OK;
}

ERet SgRenderer::_HighlightVisibleShips(
	const CulledObjectsIDs& culled_objects_ids
	, const NwCameraView& camera_view
	, const NwTime& game_time
	, const SgAppSettings& game_settings
	, const SgGameplayRenderData& gameplay_data
	, const SgPhysicsMgr& physics_mgr
	, GL::NwRenderContext & render_context
	)
{
	if(!should_highlight_visible_ships) {
		return ALL_OK;
	}

	//
	enum {
		MAX_SHIPS_TO_HIGHLIGHT = 2048
	};

	//
	nwNON_TWEAKABLE_CONST(float, SCALE_FACTOR_MUL, 0.05f);

	nwTWEAKABLE_CONST(float, MIN_DISTANCE_TO_DRAW_SHAPES, 100.0f);
	nwTWEAKABLE_CONST(float, MAX_DISTANCE_TO_DRAW_SHAPES, 2000.0f);

	//
	float min_distance_to_draw_shapes = MIN_DISTANCE_TO_DRAW_SHAPES;
	float max_distance_to_draw_shapes = MAX_DISTANCE_TO_DRAW_SHAPES;

	//if(should_highlight_visible_ships) {
	//	min_distance_to_draw_shapes = 5.0f;
	//	max_distance_to_draw_shapes = BIG_NUMBER;
	//}

	//
	const StridedRBInstanceData rb_instances_data = physics_mgr.GetRBInstancesData();

	//
	const V3f	cam_up_dir = camera_view.getUpDir();
	const RGBAf	enemy_color = RGBAf::RED;

	//
	TbPrimitiveBatcher & debug_renderer = _render_system->_debug_renderer;

	//
	const UINT safe_num_ships_to_highlight = smallest(culled_objects_ids._count, MAX_SHIPS_TO_HIGHLIGHT);

	for(UINT ii = 0; ii < safe_num_ships_to_highlight; ii++)
	{
		const U32 culled_obj_id = culled_objects_ids._data[ii];

		//
		const UINT obj_index = culled_obj_id & CULLED_OBJ_INDEX_MASK;
		const EGameObjType obj_type = EGameObjType(culled_obj_id >> CULLED_OBJ_TYPE_SHIFT);

		const SgShip& spaceship = gameplay_data.spaceships[obj_index];
		const bool ship_is_enemy = (spaceship.flags & fl_ship_is_my_enemy);
		const bool ship_is_mission_goal = (spaceship.flags & fl_ship_is_mission_goal);

		//
		const Vector4	ship_pos = rb_instances_data.GetAt(obj_index).pos;

		//
		const float distance = (camera_view.eye_pos - Vector4_As_V3(ship_pos)).length();

		if(
			ship_is_mission_goal
			||
			distance >= min_distance_to_draw_shapes && distance <= max_distance_to_draw_shapes
			)
		{
			const float scale_factor = ship_is_mission_goal ? SCALE_FACTOR_MUL*4.0f : SCALE_FACTOR_MUL;

			if(ship_is_enemy)
			{
				const int num_circle_sides
					= ship_is_mission_goal
					? 4	// quad
					: 3	// triangle
					;

				//
				debug_renderer.DrawCircle(
					Vector4_As_V3(ship_pos)
					, camera_view.right_dir
					, cam_up_dir
					, enemy_color
					, distance * scale_factor
					, num_circle_sides
					);
			}
			else
			{
				debug_renderer.DrawCircle(
					Vector4_As_V3(ship_pos)
					, camera_view.right_dir
					, cam_up_dir
					, RGBAf::GREEN
					, distance * SCALE_FACTOR_MUL
					, 4//num_circle_sides
					);
			}
		}

	}//for each visible obj

	return ALL_OK;
}

struct PlasmaBoltVertex
{
	//
	V4f		position_and_type;

	// life: [0..1] is used for alpha
	V4f		direction_and_life;
};

ERet SgRenderer::_DrawProjectiles(
	const TSpan< const SgProjectile >& projectiles
	, const NwCameraView& camera_view
	, const NwTime& game_time
	, const SgAppSettings& game_settings
	, const SgGameplayRenderData& gameplay_data
	, const SgWorld& world
	, GL::NwRenderContext & render_context
	)
{
	if(projectiles.isEmpty())
	{
		return ALL_OK;
	}

	//
	nwNON_TWEAKABLE_CONST(float, plasma_light_radius, 2.0f);

	//
	const V3f projectile_colors[ obj_type_count ] = {
		{ 0.7f, 0.7f, 0.9f } //CV3f(0.3f, 0.60f, 0.82f)
		, { 0.3f, 0.8f, 0.9f }

		, { 0.3f, 0.8f, 0.9f }
		, { 0.1f, 0.8f, 0.2f }

		// enemies

		, { 1.0f, 0.1f, 0.05f }
		, { 1.0f, 0.1f, 0.05f }
	};

	//
	const SgProjectile*const plasma_bolts_ptr = projectiles.raw();
	const UINT num_plasma_bolts = projectiles.num();

	//
	ParticleVertex *	allocated_particles;
	mxDO(Render_VolumetricParticlesWithShader(
		MakeAssetID("projectile_plasma_bolt")
		, num_plasma_bolts
		, allocated_particles
		));

	//
	nwFOR_LOOP(UINT, i, num_plasma_bolts)
	{
		const SgProjectile& projectile = plasma_bolts_ptr[ i ];

		//
		const V3f& projectile_color = projectile_colors[ projectile.ship_type ];
		
		//
		PlasmaBoltVertex &particle_vertex = (PlasmaBoltVertex&) *allocated_particles++;
		
		particle_vertex.position_and_type = V4f::set(
			projectile.pos
			, projectile.ship_type
			);

		particle_vertex.direction_and_life = V4f::set(
			projectile.velocity.normalized()
			, projectile.life
			);

#if 0
_render_system->_debug_renderer.DrawLine(
projectile.pos
, projectile.pos + projectile.velocity*0.2f
, projectile_color
, projectile_color
);
#endif

mxOPTIMIZE("custom pool for dynamic lights");
//
{
	TbDynamicPointLight	plasma_light;
	plasma_light.position = projectile.pos;
	plasma_light.radius = plasma_light_radius;
	plasma_light.color = projectile_color;

	world.spatial_database->AddDynamicPointLight(plasma_light);
}
	}

	return ALL_OK;
}

ERet SgRenderer::_BeginDebugLineDrawing(
	const NwCameraView& camera_view
	, GL::NwRenderContext & render_context
	)
{
	_is_drawing_debug_lines = true;

	return ALL_OK;
}

void SgRenderer::_FinishDebugLineDrawing()
{
	_render_system->_debug_line_renderer.flush(
		_render_system->_debug_renderer
		);
	_render_system->_debug_renderer.Flush();
	_is_drawing_debug_lines = false;
}

ERet SgRenderer::DebugDrawWorld(
									  const SgWorld& game_world
									  , const SpatialDatabaseI* spatial_database
									  , const NwCameraView& camera_view
									  , const SgHumanPlayer& game_player
									  , const SgAppSettings& game_settings
									  , const RrGlobalSettings& renderer_settings
									  , const NwTime& game_time
									  , GL::NwRenderContext & render_context
									  )
{
	mxASSERT(_is_drawing_debug_lines);


	//
#if 0
	game_player.Dbg_RenderFirstPersonView(
		camera_view
		, *this
		);
#endif

#if !GAME_CFG_RELEASE_BUILD
	// Crosshair
	if( game_settings.drawCrosshair )
	{
		_render_system->_debug_renderer.DrawSolidCircle(
			camera_view.eye_pos + camera_view.look_dir,
			camera_view.right_dir,
			camera_view.getUpDir(),
			RGBAf::RED,
			0.01f
			);
	}
#endif

	//
	if( game_settings.drawGizmo )
	{
		const NwFloatingOrigin floating_origin = spatial_database->getFloatingOrigin();


		// Draws a red point at the center of the world.
#if 0
		_render_system->_debug_renderer.DrawSolidCircle(
			-V3f::fromXYZ(floating_origin.position_in_world_space),
			camera_view.right_dir,
			camera_view.getUpDir(),
			RGBAf::RED,
			0.01f
			);
#endif


		// draw axes at the world origin

		DrawGizmo(
			_gizmo_renderer
			, M44_BuildTS(
				-V3f::fromXYZ(floating_origin.center_in_world_space)// - camera_view.eye_pos
				, 1
			)
			, camera_view.eye_pos
			, _render_system->_debug_renderer
			);
	}

	//
	if( game_settings.dbg_use_3rd_person_camera_for_terrain_RoI )
	{
		//
		const NwFloatingOrigin floating_origin = spatial_database->getFloatingOrigin();

		double radius = sqrt(( game_player.camera._eye_position - game_settings.dbg_3rd_person_camera_position ).lengthSquared()) * 0.03;

		_render_system->_debug_renderer.DrawSolidSphere(
			floating_origin.getRelativePosition( game_settings.dbg_3rd_person_camera_position )
			, radius
			, RGBAf::RED
			);
	}


#if GAME_CFG_WITH_PHYSICS
	if( game_settings.physics.debug_draw )
	{
		// VERY SLOW!!!
		btDynamicsWorld& bt_world = (btDynamicsWorld&) game_world.physics_world.m_dynamicsWorld;
		bt_world.debugDrawWorld();
		//dbg_drawRigidBodyFrames(_physics_world, _render_system->m_debugRenderer);
	}
#endif // GAME_CFG_WITH_PHYSICS

	return ALL_OK;
}

ERet SgRenderer::Submit_Skybox()
{
	GL::NwRenderContext& render_context = GL::getMainRenderContext();

	const GL::LayerID viewId = _render_system->GetRenderPath().getPassDrawingOrder(Rendering::ScenePasses::Sky);

	NwShaderEffect* shader_effect = nil;
	mxDO(Resources::Load( shader_effect, MakeAssetID("procedural_starfield_skybox") ));

	//
	shader_effect->setResource(
		mxHASH_STR("t_cubeMap"),
		GL::AsInput( h_skybox_cubemap )
		);

	const NwShaderEffect::Pass& pass0 = shader_effect->getPasses()[0];

	//
	GL::NwPushCommandsOnExitingScope	submitBatch(
		render_context,
		GL::buildSortKey( viewId, pass0.default_program_handle, 0 )
		);

	mxDO(shader_effect->pushShaderParameters( render_context._command_buffer ));

	mxDO(Rendering::DrawSkybox_TriStrip_NoVBIB(
		render_context
		, pass0.render_state
		, pass0.default_program_handle
	));

	return ALL_OK;
}

void SgRenderer::DrawCrosshair(bool highlight_as_enemy)
{
	TbPrimitiveBatcher& dbg_draw = _render_system->_debug_renderer;
	const NwCameraView& camera_view = scene_view;
							   
	//
	const V3f point_in_front_of_camera = camera_view.eye_pos + camera_view.look_dir;

	nwTWEAKABLE_CONST(float, CROSSHAIR_LENGTH_SCALE_MIN, 0.01f);
	nwTWEAKABLE_CONST(float, CROSSHAIR_LENGTH_SCALE_MAX, 0.07f);

	const RGBAf crosshair_color
		= highlight_as_enemy
		? RGBAf::RED
		: RGBAf::LIGHT_YELLOW_GREEN
		;

	// horizontal lines
	dbg_draw.DrawLine(
		point_in_front_of_camera + camera_view.right_dir * CROSSHAIR_LENGTH_SCALE_MIN,
		point_in_front_of_camera + camera_view.right_dir * CROSSHAIR_LENGTH_SCALE_MAX,
		crosshair_color,
		crosshair_color
		);
	dbg_draw.DrawLine(
		point_in_front_of_camera - camera_view.right_dir * CROSSHAIR_LENGTH_SCALE_MIN,
		point_in_front_of_camera - camera_view.right_dir * CROSSHAIR_LENGTH_SCALE_MAX,
		crosshair_color,
		crosshair_color
		);


	// vertical lines
	const V3f up_dir = camera_view.getUpDir();
	dbg_draw.DrawLine(
		point_in_front_of_camera + up_dir * CROSSHAIR_LENGTH_SCALE_MIN,
		point_in_front_of_camera + up_dir * CROSSHAIR_LENGTH_SCALE_MAX,
		crosshair_color,
		crosshair_color
		);
	dbg_draw.DrawLine(
		point_in_front_of_camera - up_dir * CROSSHAIR_LENGTH_SCALE_MIN,
		point_in_front_of_camera - up_dir * CROSSHAIR_LENGTH_SCALE_MAX,
		crosshair_color,
		crosshair_color
		);
}

/*
*/
static void scaleIntoCameraDepthRange(
								 const V3d& pos_in_world_space
								 , const SgHumanPlayer& game_player
								 , const NwCameraView& camera_view
								 , double *object_scale_	// (0 < scale < 1]
								 , V3d *scaled_position_relative_to_camera_
								 )
{
	//
	const V3d position_relative_to_camera =
		pos_in_world_space - game_player.camera._eye_position
		;

	const double real_distance_from_camera = sqrt( position_relative_to_camera.lengthSquared() );

	//
	const double half_depth_range = camera_view.far_clip * 0.5;

	const double VERY_FAR_AWAY = 1e9;

	//

	double scaled_distance_from_camera
		= half_depth_range
		+ half_depth_range * (1.0 - exp((half_depth_range - real_distance_from_camera) / VERY_FAR_AWAY))
		;

	*object_scale_ = scaled_distance_from_camera / real_distance_from_camera;
	*scaled_position_relative_to_camera_ = position_relative_to_camera.normalized() * scaled_distance_from_camera;
}

ERet SgRenderer::submit_Star(
								  const NwStar& star
								  , const SgHumanPlayer& game_player
								  , const NwCameraView& camera_view
								  , GL::NwRenderContext &render_context
								  )
{
#if MX_DEVELOPER
	{
		NwStar & mut_star = (NwStar&) star;
		HOT_VAR(mut_star.pos_in_world_space);
		HOT_VAR(mut_star.radius_in_meters);
	}
#endif

	//
	double	star_scale;
	V3d		scaled_position_relative_to_camera;
	scaleIntoCameraDepthRange(
		star.pos_in_world_space
		, game_player
		, camera_view
		, &star_scale
		, &scaled_position_relative_to_camera
		);

	//
	const V3f star_position_relative_to_camera = V3f::fromXYZ(scaled_position_relative_to_camera);

	//
	const RenderPath& render_path = _render_system->GetRenderPath();
	const U32 scene_pass_index = render_path.getPassDrawingOrder( Rendering::ScenePasses::VolumetricParticles );
UNDONE;
	//
	NwShaderEffect* shader_effect = nil;
	mxDO(Resources::Load( shader_effect, MakeAssetID("billboard_star") ));


	//
	const Rendering::NwRenderSystem& render_sys = (Rendering::NwRenderSystem&) *_render_system;
	mxDO(shader_effect->setResource(
		mxHASH_STR("DepthTexture"),
		GL::AsInput(render_sys._deferred_renderer.m_depthRT->handle)
		));


	//
	const NwShaderEffect::Variant shader_variant = shader_effect->getDefaultVariant();

	//
	GL::RenderCommandWriter	gfx_cmd_writer( render_context );
	//
	gfx_cmd_writer.SetRenderState( shader_variant.render_state );

	//
	mxDO(shader_effect->pushAllCommandsInto( render_context._command_buffer ));

	//
	NwTransientBuffer	transient_VB;

	const U32	max_vertices = 1;

	mxDO(GL::allocateTransientVertexBuffer(
		transient_VB
		, max_vertices
		, sizeof(BillboardStarVertex)
		));
	{
		BillboardStarVertex & star_vertex = *(BillboardStarVertex*) transient_VB.data;

		star_vertex.sphere_in_view_space = V4f::set(
			star_position_relative_to_camera
			, (float) ( star.radius_in_meters * star_scale )
			);
		star_vertex.velocity_and_life = V4f::set(CV3f(0), 1);
	}

	//
	GL::Cmd_Draw	draw_indexed_cmd;
	draw_indexed_cmd.setup( shader_variant.program_handle );
	draw_indexed_cmd.input_layout = BillboardStarVertex::metaClass().input_layout_handle;
	draw_indexed_cmd.primitive_topology = NwTopology::PointList;
	draw_indexed_cmd.use_32_bit_indices = false;

	draw_indexed_cmd.VB = transient_VB.buffer_handle;
	draw_indexed_cmd.IB.setNil();

	draw_indexed_cmd.base_vertex = transient_VB.base_index;
	draw_indexed_cmd.vertex_count = max_vertices;
	draw_indexed_cmd.start_index = 0;
	draw_indexed_cmd.index_count = 0;

	//
	gfx_cmd_writer.Draw( draw_indexed_cmd );

	//
	gfx_cmd_writer.submitCommandsWithSortKey(
		GL::buildSortKey( scene_pass_index, shader_variant.program_handle.id )
		);

	return ALL_OK;
}

//void SgRenderer::_updateAtmosphereRenderingParameters(
//	const NwCameraView& camera_view
//	, const RrGlobalSettings& renderer_settings
//	, const SgAppSettings& game_settings
//	)
//{
//	_atmosphere_rendering_parameters.atmosphere = game_settings.test_atmosphere;
//
//	_atmosphere_rendering_parameters.relative_eye_position = V3f::fromXYZ( camera_view.eye_pos_in_world_space );
//}

ERet SgRenderer::Render_VolumetricParticlesWithShader(
	const AssetID& particle_shader_id
	, const int num_particles
	, ParticleVertex *&allocated_buffer_
	)
{
	mxASSERT(num_particles > 0);
	mxASSERT2(num_particles < MAX_UINT16, "Only 16-bit indices are supported!");
	//
	NwShaderEffect* shader_effect = nil;
	mxDO(Resources::Load( shader_effect, particle_shader_id ));

	const NwShaderEffect::Variant shader_effect_variant = shader_effect->getDefaultVariant();

	//
	const Rendering::NwRenderSystem& render_sys = (Rendering::NwRenderSystem&) Rendering::NwRenderSystem::Get();

	//
	GL::NwRenderContext & render_context = GL::getMainRenderContext();
	//
	const U32 scene_pass_index = _render_system->GetRenderPath()
		.getPassDrawingOrder( Rendering::ScenePasses::VolumetricParticles );


	//
	ParticleVertex *	allocated_particles;

	//
	NwTransientBuffer	vertex_buffer;
	mxDO(GL::allocateTransientVertexBuffer(
		vertex_buffer
		, num_particles
		, sizeof(allocated_particles[0])
		));

	//
	allocated_particles = static_cast< ParticleVertex* >( vertex_buffer.data );
	allocated_buffer_ = allocated_particles;

	//
	GL::NwPushCommandsOnExitingScope	applyShaderCommands(
		render_context,
		GL::buildSortKey( scene_pass_index, 0 )
		);

	//
	GL::CommandBufferWriter	cmd_writer( render_context._command_buffer );

	//
	mxDO(shader_effect->setResource(
		mxHASH_STR("DepthTexture"),
		GL::AsInput(render_sys._deferred_renderer.m_depthRT->handle)
		));

	shader_effect->pushShaderParameters( render_context._command_buffer );

	//
	GL::Cmd_Draw_Obsolete	batch;
	batch.reset();
	{
		batch.states = shader_effect_variant.render_state;
		batch.program = shader_effect_variant.program_handle;

		batch.inputs = GL::Cmd_Draw_Obsolete::EncodeInputs(
			ParticleVertex::metaClass().input_layout_handle,
			NwTopology::PointList,
			false	// 32-bit indices?
			);

		batch.VB = vertex_buffer.buffer_handle;

		batch.base_vertex = vertex_buffer.base_index;
		batch.vertex_count = num_particles;
		batch.start_index = 0;
		batch.index_count = 0;
	}
	IF_DEBUG batch.src_loc = GFX_SRCFILE_STRING;
	cmd_writer.draw_obsolete( batch );

	return ALL_OK;
}
