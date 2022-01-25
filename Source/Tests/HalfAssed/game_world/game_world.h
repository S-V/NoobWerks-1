#pragma once

//
#include <ProcGen/FastNoiseSupport.h>
#pragma comment( lib, "ProcGen.lib" )


#include <Renderer/Renderer.h>	// LargePosition, NwCameraView
#include <ProcGen/Noise/NwNoiseFunctions.h>
#include <Planets/PlanetsCommon.h>
#include <Planets/Noise.h>	// HybridMultiFractal

#include <Utility/VoxelEngine/VoxelTerrainRenderer.h>

#include <Voxels/public/vx5.h>
#include <Voxels/extensions/lmdb_storage.h>	// ChunkDatabase_LMDB

//
#include "game_compile_config.h"

//
#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
//#include <Physics/Bullet_Wrapper.h>
#endif // GAME_CFG_WITH_PHYSICS

//
#include "game_forward_declarations.h"
#include "experimental/game_experimental.h"
#include "game_world/procedural_terrain_generation.h"
#include "game_entities/game_entities.h"
#include "game_world/game_projectiles.h"
#include "game_world/gibs.h"
#include "npc/game_nps.h"



#define USE_PERSISTENT_VOXEL_DATABASE	(0)


/*
===============================================================================
	VOXELS
===============================================================================
*/
namespace VX5
{

	/// A software cache.
	///
	class ChunkDatabase_InMemoryCache: public ChunkDatabaseI
	{
	public:
		ChunkDatabase_InMemoryCache();
		~ChunkDatabase_InMemoryCache();

		virtual ERet Read(
			const ChunkID& chunk_id
			, void *buffer, U32 buffer_size
			, U32 *size_read_ = NULL
			) const override;

		virtual ERet Write(
			const ChunkID& chunk_id
			, const void* data, U32 size
			) override;

		virtual ERet Erase(
			const ChunkID& chunk_id
			) override;

		virtual ERet Drop() override;
	};


	///
	class MyDebugDraw
		: public ImmediateDebugDrawerI
		, public RetainedDebugDrawerI
	{
		TPtr< TbPrimitiveBatcher >	_prim_renderer;
		TPtr< ARenderWorld >		_render_world;
		NwFloatingOrigin			_floating_origin;
		NwCameraView				_camera_matrices;

		//
		SpinWait		_crit_section;
		
		//VX::DebugView				_dbgview;
		// 

		struct DebugPoint
		{
			V3d		position_in_world_space;
			float	scale;
			RGBAi	color;
		};
		DynamicArray< DebugPoint >	_points;

		struct DebugLine {
			V3d		start_position_in_world_space;
			RGBAi	start_color;
			V3d		end_position_in_world_space;
			RGBAi	end_color;
		};
		DynamicArray< DebugLine >	_lines;

		//
		struct DebugAABB {
			AABBd	aabb_in_world_space;
			RGBAi	color;
		};
		DynamicArray< DebugAABB >	_aabbs;

	public:
		MyDebugDraw( AllocatorI& allocator );

		void initialize(
			TbPrimitiveBatcher* prim_renderer
			, ARenderWorld* render_world
			);
		void shutdown();

		ERet begin(
			const NwFloatingOrigin& floating_origin
			, const NwCameraView& camera_view
			);
		void end();

	public:	// ImmediateDebugDrawerI

		virtual void VX_DrawLine_MainThread(
			const V3d& start_pos_in_world_space
			, const RGBAi& start_color
			, const V3d& end_pos_in_world_space
			, const RGBAi& end_color
		) override;

		virtual void DrawAABB_MainThread(
			const AABBd& aabb_in_world_space,
			const RGBAf& color
		) override;

	public:	// RetainedDebugDrawerI

		virtual void VX_AddPoint_AnyThread(
			const V3d& position_in_world_space
			, const RGBAi& color
			, double point_scale = 1
		) override;

		virtual void VX_AddLine_AnyThread(
			const V3d& start_pos_in_world_space
			, const RGBAi& start_color
			, const V3d& end_pos_in_world_space
			, const RGBAi& end_color
		) override;

		virtual void VX_AddAABB_AnyThread(
			const AABBd& aabb_in_world_space
			, const RGBAi& color
			) override;

		virtual void VX_Clear_AnyThread() override;
	};

}//namespace VX5













namespace VX5
{

	struct WorldDatabases_LMDB2: WorldStorage
	{
		VX5::ChunkDatabase_LMDB	index_database_LMDB;
		VX5::ChunkDatabase_LMDB	chunk_database_LMDB;

	public:
		WorldDatabases_LMDB2()
		{
			meta = &index_database_LMDB;
			data = &chunk_database_LMDB;
		}

		ERet Open(const char* folder_with_database_files);
		void Close();
	};

}//namespace VX5


struct MyVoxels
	: public VX5::ChunkGeneratorI
	, public VX5::NoiseBrushI
{
	//
	TPtr< VX5::EngineI >	engine;
	VX5::MyDebugDraw		voxel_engine_dbg_drawer;

	//
	PlaneD						_test_plane;
	TPtr< VX5::WorldI >			voxel_world;
	CubeMLf						world_bounds;

	//
#if USE_PERSISTENT_VOXEL_DATABASE
	VX5::WorldDatabases_LMDB2	voxel_databases;
#else
	VX5::WorldStorage		voxel_databases;	// dummy
#endif

	//
	VoxelTerrainRenderer	_voxel_terrain_renderer;

	// for craters
	Noise::NwPerlinNoise	_perlin_noise;
	Noise::NwTerrainNoise2D	_test_terrain_noise;

public:
	MyVoxels( AllocatorI& allocator )
		: voxel_engine_dbg_drawer( allocator )
		, _voxel_terrain_renderer( allocator )
	{
	}

public:	// VX5::ChunkGeneratorI

	virtual void GetBoxContentsInfo(
		VX5::BoxContentsInfo &box_contents_
		, const CubeMLd& box_in_world_space
		, const VX5::ChunkGenerationContext& context
		) override;

	virtual double GetSignedDistanceAtPoint(
		const V3d& position_in_world_space
		, const VX5::ChunkGenerationContext& context
		, const VX5::ChunkGenerationScratchpad& scratchpad
		) override;

public:	// VX5::NoiseBrushI
	virtual float01_t	GetStrength(const V3f& pos) override;
};


/*
-----------------------------------------------------------------------------
	MyGameWorld
-----------------------------------------------------------------------------
*/
class MyGameWorld
	: NonCopyable
	, public VX5::EngineListenerI
	, public VX5::MeshBakerI
	, public VX5::MeshLoaderI
	, public VX5::WorldListenerI
{
public:	// Rendering
	TPtr< ARenderWorld >		render_world;

public:	// Voxels.

	MyVoxels	voxels;


#if TEST_LOAD_RED_FACTION_MODEL

	TestSkinnedModel		_RF_test_model;

	RF1::Anim				_RF_test_anim;
	NwPlaybackController	_RF_test_anim_playback_ctrl;

#endif // TEST_LOAD_RED_FACTION_MODEL


public:	// Physics

#if GAME_CFG_WITH_PHYSICS

	NwPhysicsWorld				physics_world;
	TbBullePhysicsDebugDrawer	_physics_debug_drawer;

#else

	NwPhysicsWorld &			physics_world;

#endif // GAME_CFG_WITH_PHYSICS


	GameProjectiles		projectiles;

public:	// Gameplay

	MyGameNPCs		NPCs;

	MyGameGibs		gibs;

	//GameSpaceship	_player_spaceship;


	//BurningFire	test_burning_fire;


public:	// Object storage
	//
	TPtr< NwClump >		_scene_clump;


	//
	DynamicArray<NwModel*>	models_to_delete;
	NwModel*				models_to_delete_embedded_storage[16];

	//
	AllocatorI &		_allocator;

public:
	MyGameWorld( AllocatorI& allocator );

	ERet initialize(
		const MyGameSettings& game_settings
		, NwClump* scene_clump
		, TbRenderSystemI* render_system
		, AllocatorI& scratchpad
		);

	void shutdown();

	void UpdateOncePerFrame(
		const NwTime& game_time
		, MyGamePlayer& game_player
		, const MyGameSettings& game_settings
		, FPSGame& game
		);

	void Render(
		const NwCameraView& camera_view
		, const RrRuntimeSettings& renderer_settings
		, const NwTime& game_time
		, MyGameRenderer& game_renderer
		);

public:
	void RequestDeleteModel(NwModel* model_to_delete);

private:
	void _DeletePendingModels();

public:	// Game Level Loading.

	ERet LoadFromTape(
		const char* level_filename
		);

	ERet ApplyEditsFromTape(
		const SDF::Tape& tape
		);

private:	// Voxels.

	ERet _InitVoxelEngine(
		const MyGameSettings& game_settings
		, NwClump* scene_clump
		, AllocatorI& scratchpad
		);
	void _DeinitVoxelEngine();

private:	// NPCs/AI.

	ERet _InitNPCs(
		const MyGameSettings& game_settings
		, NwClump* scene_clump
		);
	void _DeinitNPCs();


private:	// Physics.

#if GAME_CFG_WITH_PHYSICS

	ERet _InitPhysics(
		NwClump * storage_clump 
		, TbPrimitiveBatcher & debug_renderer
		);
	void _DeinitPhysics();

	void _UpdatePhysics(
		const NwTime& game_time
		, MyGamePlayer& game_player
		, const MyGameSettings& game_settings
		);

	void Physics_AddGroundPlane_for_Collision();

#endif // GAME_CFG_WITH_PHYSICS

private:
	static VX5::WorldI* _CreateVoxelWorld(
		const PlaneD& plane
		, VX5::EngineI* voxel_engine
		, VX5::ChunkGeneratorI* chunk_generator
		, MeshBakerI* mesh_baker
		, MeshLoaderI* mesh_loader
		, VX5::WorldListenerI* world_delegate
		, const VX5::WorldStorage& voxel_databases
		, const VX5::MaterialID default_solid_material
		, CubeMLf &world_bounds_
		);

public:	// Debugging
	void debugDrawVoxels(
		const NwFloatingOrigin& floating_origin
		, const NwCameraView& camera_view
		);

	void regenerateVoxelTerrains();

public:	// VX5::MeshBakerI

	virtual void VX_BakeCollisionMesh_AnyThread(
		const BakeCollisionMeshInputs& inputs
		, BakeCollisionMeshOutputs &outputs_
		) override;

	virtual void VX_BakeChunkData_AnyThread(
		const BakeMeshInputs& inputs
		, BakeMeshOutputs &outputs_
		) override;

public:	// VX5::MeshLoaderI
	virtual void* VX_LoadChunkData_MainThread(
		const LoadMeshInputs& args
		) override;
	
	virtual void VX_UnloadChunkData_MainThread(
		void* mesh
		, const UnloadMeshInputs& args
		) override;

private:

	ERet VX_BakeCollisionMesh_AnyThread_Internal(
		const VX5::MeshBakerI::BakeCollisionMeshInputs& inputs
		, VX5::MeshBakerI::BakeCollisionMeshOutputs &outputs_
		);

	ERet VX_BakeChunkData_AnyThread_Internal(
		const VX5::MeshBakerI::BakeMeshInputs& inputs
		, VX5::MeshBakerI::BakeMeshOutputs &outputs_
		);

	ERet VX_LoadChunkData_MainThread_Internal(
		void *&mesh_
		, const VX5::MeshLoaderI::LoadMeshInputs& args

		);
	
	void VX_UnloadChunkData_MainThread_Internal(
		void* mesh
		, const VX5::MeshLoaderI::UnloadMeshInputs& args
		);

public:	// VX5::WorldListenerI

	/// for showing loading screen
	virtual void VX_OnLevelLoadProgress(
		const float01_t progress01
		) override;

	/// Used for spawning NPCs only after the level is fully loaded.
	/// LoDs are refined from top to bottom, 0 = index of the most detailed LoD.
	virtual void VX_OnLevelLoaded() override;
};
