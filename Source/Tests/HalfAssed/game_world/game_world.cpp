//
#include "stdafx.h"
#pragma hdrstop

#include <stdint.h>

#include <Core/Tasking/TaskSchedulerInterface.h>	// threadLocalHeap()
#include <Renderer/Core/MeshInstance.h>
#include <Renderer/Modules/Animation/AnimatedModel.h>

#include <Engine/Model.h>
#include <Engine/Engine.h>	// NwTime

#include <BlobTree/BlobTree.h>

#include "FPSGame.h"
#include "game_player.h"
#include "game_settings.h"
#include "experimental/game_experimental.h"
#include "game_world/game_world.h"
#include "game_animation/game_animation.h"




namespace VX5
{
	ChunkDatabase_InMemoryCache::ChunkDatabase_InMemoryCache()
	{
		//
	}

	ChunkDatabase_InMemoryCache::~ChunkDatabase_InMemoryCache()
	{
		//
	}

	ERet ChunkDatabase_InMemoryCache::Read(
		const ChunkID& chunk_id
		, void *buffer, U32 buffer_size
		, U32 *size_read_
		) const 
	{
		return ERR_OBJECT_NOT_FOUND;
	}

	ERet ChunkDatabase_InMemoryCache::Write(
		const ChunkID& chunk_id
		, const void* data, U32 size
		) 
	{
		return ALL_OK;
	}

	ERet ChunkDatabase_InMemoryCache::Erase(
		const ChunkID& chunk_id
		) 
	{
		return ALL_OK;
	}

	ERet ChunkDatabase_InMemoryCache::Drop() 
	{
		return ALL_OK;
	}


	MyDebugDraw::MyDebugDraw( AllocatorI& allocator )
		: _points( allocator )
		, _lines( allocator )
		, _aabbs( allocator )
	{
		//
	}

	void MyDebugDraw::initialize(
		TbPrimitiveBatcher* prim_renderer
		, ARenderWorld* render_world
		)
	{
		_prim_renderer = prim_renderer;
		_render_world = render_world;

		_crit_section.Initialize();
	}

	void MyDebugDraw::shutdown()
	{
		_crit_section.Shutdown();

		_prim_renderer = nil;
	}

	ERet MyDebugDraw::begin(
		const NwFloatingOrigin& floating_origin
		, const NwCameraView& camera_view
		)
	{
		_floating_origin = floating_origin;
		_camera_matrices = camera_view;

		// enable depth testing
		NwRenderState * renderState = nil;
		mxDO(Resources::Load(renderState, MakeAssetID("default")));

		_prim_renderer->PushState();
		_prim_renderer->SetStates( *renderState );


		//
		const V3f up = camera_view.getUpDir();

		//
		//_dbgview.draw_internal(
		//	camera_view,
		//	*_prim_renderer
		//	);

		//DEVOUT("Drawing %u colored points...", _colored_points.num());

		//
		{
			SpinWait::Lock	scoped_lock(_crit_section);

			nwFOR_EACH(const DebugPoint& point, _points)
			{
				_prim_renderer->DrawSolidCircle(
					_floating_origin.getRelativePosition( point.position_in_world_space )
					, camera_view.right_dir, up
					, RGBAf::FromRGBAi(point.color)
					, point.scale
					, 4	// num sides
					);
			}

			//
			nwFOR_EACH(const DebugLine& line, _lines)
			{
				_prim_renderer->DrawLine(
					_floating_origin.getRelativePosition( line.start_position_in_world_space )
					, _floating_origin.getRelativePosition( line.end_position_in_world_space )
					, RGBAf::FromRGBAi( line.start_color )
					, RGBAf::FromRGBAi( line.end_color )
					);
			}

			//
			nwFOR_EACH(const DebugAABB& aabb, _aabbs)
			{
				_prim_renderer->DrawAABB(
					_floating_origin.getRelativeAABB( aabb.aabb_in_world_space )
					, RGBAf::FromRGBAi( aabb.color )
					);
			}
		}

		return ALL_OK;
	}

	void MyDebugDraw::end()
	{
		_prim_renderer->PopState();
	}

	void MyDebugDraw::VX_DrawLine_MainThread(
		const V3d& start_pos_in_world_space
		, const RGBAi& start_color
		, const V3d& end_pos_in_world_space
		, const RGBAi& end_color
		)
	{
		_prim_renderer->DrawLine(
			V3f::fromXYZ(start_pos_in_world_space)
			, V3f::fromXYZ(end_pos_in_world_space)
			, RGBAf::fromRgba32( start_color.u )
			, RGBAf::fromRgba32( end_color.u )
			);
	}

	void MyDebugDraw::DrawAABB_MainThread(
		const AABBd& aabb_in_world_space,
		const RGBAf& color
		)
	{
		const AABBf aabb_relative_to_camera = _floating_origin.getRelativeAABB( aabb_in_world_space );

		// Cull bounding boxes that take very little space on the screen (contribution culling).
		// This improves increases performance when looking at lots of nested LoDs from the distance.

		V2f min_corner_in_screen_space;
		V2f max_corner_in_screen_space;

		const bool min_corner_in_view = _camera_matrices.projectViewSpaceToScreenSpace(
			aabb_relative_to_camera.min_corner, &min_corner_in_screen_space
			);
		const bool max_corner_in_view = _camera_matrices.projectViewSpaceToScreenSpace(
			aabb_relative_to_camera.max_corner, &max_corner_in_screen_space
			);

		// If the box is not behind the near plane:
		if( min_corner_in_view || max_corner_in_view )
		{
			const float box_area = 
				fabs( max_corner_in_screen_space.x - min_corner_in_screen_space.x )
				*
				fabs( max_corner_in_screen_space.y - min_corner_in_screen_space.y )
				;

			if( box_area > 2.0f )
			{
				_prim_renderer->DrawAABB(
					aabb_relative_to_camera
					, color
					);
			}
		}
	}

	void MyDebugDraw::VX_AddPoint_AnyThread(
		const V3d& position_in_world_space
		, const RGBAi& color
		, double point_scale
	)
	{
		SpinWait::Lock	scoped_lock(_crit_section);

		DebugPoint	new_point;
		new_point.position_in_world_space = position_in_world_space;
		new_point.scale = point_scale;
		new_point.color = color;

		_points.add( new_point );
	}

	void MyDebugDraw::VX_AddLine_AnyThread(
		const V3d& start_pos_in_world_space
		, const RGBAi& start_color
		, const V3d& end_pos_in_world_space
		, const RGBAi& end_color
		)
	{
		SpinWait::Lock	scoped_lock(_crit_section);

		DebugLine	new_line;
		{
			new_line.start_position_in_world_space = start_pos_in_world_space;
			new_line.start_color = start_color;
			new_line.end_position_in_world_space = end_pos_in_world_space;
			new_line.end_color = end_color;
		}
		_lines.add(new_line);
	}

	void MyDebugDraw::VX_AddAABB_AnyThread(
		const AABBd& aabb_in_world_space
		, const RGBAi& color
		)
	{
		SpinWait::Lock	scoped_lock(_crit_section);

		DebugAABB	new_aabb;
		new_aabb.aabb_in_world_space = aabb_in_world_space;
		new_aabb.color = color;

		_aabbs.add( new_aabb );
	}

	void MyDebugDraw::VX_Clear_AnyThread()
	{
		SpinWait::Lock	scoped_lock(_crit_section);

		_points.RemoveAll();
		_lines.RemoveAll();
		_aabbs.RemoveAll();
	}

}//namespace VX5

/*
-----------------------------------------------------------------------------
	MyGameWorld
-----------------------------------------------------------------------------
*/
MyGameWorld::MyGameWorld( AllocatorI& allocator )
	: _allocator( allocator )
	, voxels( allocator )
	, models_to_delete(allocator)

#if !GAME_CFG_WITH_PHYSICS
	, physics_world( *(NwPhysicsWorld*)nil )
#endif

#if TEST_LOAD_RED_FACTION_MODEL
	, _RF_test_model( allocator )
	, _RF_test_anim( allocator )
#endif // TEST_LOAD_RED_FACTION_MODEL

{
	//_fast_noise_scale = 1;
	Arrays::initializeWithExternalStorage(
		models_to_delete,
		models_to_delete_embedded_storage
		);
}

ERet MyGameWorld::initialize(
							  const MyGameSettings& game_settings
							  , NwClump* scene_clump
							  , TbRenderSystemI* render_system
							  , AllocatorI& scratchpad
							  )
{
	_scene_clump = scene_clump;

	//
	render_world = ARenderWorld::staticCreate( _allocator );



	//
	voxels.voxel_engine_dbg_drawer.initialize(
		&render_system->_debug_renderer
		, render_world
		);

	mxDO(_InitVoxelEngine(
		game_settings
		, scene_clump
		, scratchpad
		));

	//
#if 1
	render_system->_render_callbacks.code[RE_VoxelTerrainChunk] = &VoxelTerrainRenderer::renderVoxelTerrainChunks_DLoD_Planet;
	render_system->_render_callbacks.data[RE_VoxelTerrainChunk] = &voxels._voxel_terrain_renderer;
#else
	render_system->_render_callbacks.code[RE_VoxelTerrainChunk] = CustomTerrainRenderer::renderVoxelTerrainChunks_DLoD_with_Stencil_CSG;
	render_system->_render_callbacks.data[RE_VoxelTerrainChunk] = &voxels._voxel_terrain_renderer;
#endif

	//
	render_system->_shadow_render_callbacks.code[ RE_VoxelTerrainChunk ] = &VoxelTerrainRenderer::renderShadowDepthMap_DLoD;
	render_system->_shadow_render_callbacks.data[ RE_VoxelTerrainChunk ] = &voxels._voxel_terrain_renderer;

	//
	render_system->_voxelization_callbacks.code[ RE_VoxelTerrainChunk ] = &VoxelTerrainRenderer::voxelizeChunksOnGPU_DLoD;
	render_system->_voxelization_callbacks.data[ RE_VoxelTerrainChunk ] = &voxels._voxel_terrain_renderer;

	//
	mxDO(render_system->_debug_renderer.reserveSpace( mxMiB(1), mxMiB(1), mxKiB(64) ));

	//
#if GAME_CFG_WITH_PHYSICS

	mxDO(_InitPhysics(
		scene_clump
		, render_system->_debug_renderer
		));

#endif // GAME_CFG_WITH_PHYSICS


#if TEST_LOAD_RED_FACTION_MODEL
	mxDO(loadTestV3C(
		_RF_test_model, _scene_clump, scratchpad
		));

	if( _RF_test_model.isSkinned() )
	{
		mxDO(loadTestRFA(
			_RF_test_anim, _RF_test_model
			));
	}

	_RF_test_anim_playback_ctrl.set_loop(true);
	_RF_test_anim_playback_ctrl.set_playback_speed(1.0f);

#endif // TEST_LOAD_RED_FACTION_MODEL

	//mxDO(_player_spaceship.load(
	//	scene_clump
	//	, render_world
	//	));

	//gibs.Initialize()

	mxDO(_InitNPCs(
		game_settings
		, scene_clump
		));

	return ALL_OK;
}

void MyGameWorld::shutdown()
{
	_DeinitNPCs();

	//_player_spaceship.unload();

	_DeinitVoxelEngine();

	// collisions/physics must be closed after voxels!!!
#if GAME_CFG_WITH_PHYSICS

	_DeinitPhysics();

#endif // GAME_CFG_WITH_PHYSICS

	//
	ARenderWorld::staticDestroy(render_world);
	render_world = nil;

	//
	_scene_clump = nil;
}


void MyGameWorld::UpdateOncePerFrame(
							  const NwTime& game_time
							  , MyGamePlayer& game_player
							  , const MyGameSettings& game_settings
							  , FPSGame& game
							  )
{
	_DeletePendingModels();

	const V3d	player_camera_position = game_player.camera._eye_position;


	//
	voxels.engine->applySettings(
		game_settings.voxels.engine
		);

	//
	{
		//
		const V3d terrain_region_of_interest_position
			= game_settings.dbg_use_3rd_person_camera_for_terrain_RoI
			? game_settings.dbg_3rd_person_camera_position
			: player_camera_position
			;

		VX5::EngineI::UpdateArgs	params;
		params.camera_position_in_world_space = terrain_region_of_interest_position;

		voxels.engine->updateOncePerFrame(
			params
			);
	}




	// NOTE: advance animations before physics
	// to avoid unsightly "teleporting" when using root motion!
	if( !game.is_paused )
	{
		NwAnimatedModel_::UpdateInstances(
			*_scene_clump
			, game_time
			, TaskScheduler_Serial::s_instance
			);
	}


	//

#if GAME_CFG_WITH_PHYSICS

	if( !game.is_paused )
	{
		_UpdatePhysics(
			game_time
			, game_player
			, game_settings
			);

		NPCs.UpdateGraphicsTransformsFromAnimationsAndPhysics(
			game_time
			, render_world
			, physics_world
			);
	}

#endif // GAME_CFG_WITH_PHYSICS


	//
	if(
		!game.is_paused
		&&
		!game_settings.ai.debug_stop_AI
		)
	{
		NPCs.TickAI(
			game_time
			//, game.dbg_target_pos
			, V3f::fromXYZ(player_camera_position)
			, voxels.world_bounds
			);
	}

	//
	if( !game.is_paused )
	{
		//
		GameProjectiles::StateDeltaForCollisionRayCasts	projectiles_state_delta(threadLocalHeap());

		GameParticleRenderer& particle_renderer = const_cast<GameParticleRenderer&>(game.renderer.particle_renderer);
		//

		projectiles.AdvanceParticlesOncePerFrame(
			game_time
			, particle_renderer
			, render_world
			, projectiles_state_delta
			);

		//
#if GAME_CFG_WITH_PHYSICS
		projectiles.CollideWithWorld(
			projectiles_state_delta
			, physics_world
			, game.sound
			, particle_renderer
			, game.renderer.decals
			, voxels.voxel_world
			, &voxels
			);
#endif // GAME_CFG_WITH_PHYSICS

		//
		gibs.UpdateOncePerFrame(
			game_time
			, render_world
			);

		NwModel_::UpdateGraphicsTransformsFromRigidBodies(
			*_scene_clump
			, *render_world
			);
	}


	//
	render_world->updateOncePerFrame(
		player_camera_position
		, game_time.real.delta_seconds
		);



#if TEST_LOAD_RED_FACTION_MODEL

	if( !game_settings.rf1.pause_animation )
	{
		_RF_test_anim_playback_ctrl.update(
			_RF_test_anim.durationInSeconds()
			, game_time.real.delta_seconds
			);
	}

#endif // TEST_LOAD_RED_FACTION_MODEL

}





void MyGameWorld::Render(
	const NwCameraView& camera_view
	, const RrRuntimeSettings& renderer_settings
	, const NwTime& game_time
	, MyGameRenderer& game_renderer
	)
{
	//
	game_renderer._render_system->drawWorld(
		render_world
		, camera_view
		, renderer_settings
		, game_time
		);

	//
	projectiles.RenderProjectilesIfAny(
		game_renderer
		);

	//
	game_renderer.particle_renderer.RenderParticlesIfAny(
		game_renderer
		, render_world
		);

	//
	game_renderer.explosions.Render(
		*game_renderer._render_system
		, render_world
		);

	//
	game_renderer.burning_fires.Render(
		*game_renderer._render_system
		, render_world
		);

	//
	game_renderer.decals.RenderDecalsIfAny(
		*game_renderer._render_system
		, render_world
		);

	//
	game_renderer.Submit_Skybox();
}

void MyGameWorld::RequestDeleteModel(NwModel* model_to_delete)
{
	mxASSERT(models_to_delete.num() < models_to_delete.capacity());
	models_to_delete.add(model_to_delete);
}

void MyGameWorld::_DeletePendingModels()
{
	if(models_to_delete.isEmpty()) {
		return;
	}
	std::sort(models_to_delete.begin(), models_to_delete.end());
	Arrays::removeDupesFromSortedArray(models_to_delete);

	nwFOR_EACH(NwModel* model_to_delete, models_to_delete)
	{
		NwModel_::Delete(
			model_to_delete
			, *_scene_clump
			, *render_world
			, physics_world
			);
	}

	models_to_delete.RemoveAll();
}

ERet MyGameWorld::LoadFromTape(
	const char* level_filename
	)
{
	//
	SDF::Tape	tape(MemoryHeaps::temporary());
	mxDO(tape.LoadFromFile(level_filename));

	return this->ApplyEditsFromTape(tape);
}

ERet MyGameWorld::ApplyEditsFromTape(
									 const SDF::Tape& tape
									 )
{
	// Remove all data from the database.
	voxels.voxel_databases.RemoveAllData();

	//
	mxDO(tape.ApplyEditsToWorld(
		voxels.voxel_world
		, FPSGame::Get().runtime_clump
		));

	return ALL_OK;
}