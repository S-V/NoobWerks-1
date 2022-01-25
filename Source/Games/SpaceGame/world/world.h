#pragma once

//
#include <ProcGen/FastNoiseSupport.h>
#pragma comment( lib, "ProcGen.lib" )


#include <Rendering/Public/Globals.h>	// LargePosition, NwCameraView
#include <ProcGen/Noise/NwNoiseFunctions.h>
#include <Planets/PlanetsCommon.h>
#include <Planets/Noise.h>	// HybridMultiFractal

#include <VoxelsSupport/VoxelTerrainRenderer.h>

#include <Voxels/public/vx5.h>
#include <Voxels/extensions/lmdb_storage.h>	// ChunkDatabase_LMDB

//
#include "compile_config.h"

//
#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
//#include <Physics/Bullet_Wrapper.h>
#endif // GAME_CFG_WITH_PHYSICS

//
#include "common.h"
#include "experimental/game_experimental.h"
#include "world/procedural_terrain_generation.h"
#include "gameplay/game_entities.h"




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
		: public DebugDrawI
		, public DebugViewI
	{
		TPtr< TbPrimitiveBatcher >	_prim_renderer;
		TPtr< SpatialDatabaseI >		_render_world;
		NwFloatingOrigin			_floating_origin;
		NwCameraView				scene_view;

		//
		SpinWait		_crit_section;
		AtomicInt		_is_locked;
		
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
			, SpatialDatabaseI* spatial_database
			);
		void shutdown();

		ERet begin(
			const NwFloatingOrigin& floating_origin
			, const NwCameraView& camera_view
			);
		void end();

	public:	// DebugDrawI

		virtual void VX_DrawLine_MainThread(
			const V3d& start_pos_in_world_space
			, const RGBAi& start_color
			, const V3d& end_pos_in_world_space
			, const RGBAi& end_color
		) override;

		virtual void VX_DrawAABB_MainThread(
			const AABBd& aabb_in_world_space,
			const RGBAf& color
		) override;

	public:	// DebugViewI

		virtual void VX_Lock() override;
		virtual void VX_Unlock() override;

		virtual void VX_AddPoint(
			const V3d& position_in_world_space
			, const RGBAi& color
			, double point_scale = 1
		) override;

		virtual void VX_AddLine(
			const V3d& start_pos_in_world_space
			, const RGBAi& start_color
			, const V3d& end_pos_in_world_space
			, const RGBAi& end_color
		) override;

		virtual void VX_AddAABB(
			const AABBd& aabb_in_world_space
			, const RGBAi& color
			) override;

		virtual void VX_Clear() override;
	};

}//namespace VX5













namespace VX5
{

	struct WorldDatabases_LMDB2
	{
		VX5::ChunkDatabase_LMDB	index_database_LMDB;
		VX5::ChunkDatabase_LMDB	chunk_database_LMDB;
		VX5::ChunkDatabase_LMDB	lods_database_LMDB;

	public:
		ERet Open(const char* folder_with_database_files);
		void Close();
	};

}//namespace VX5


struct MyVoxels
	: public VX5::ProcGen::ChunkGeneratorI
	, public VX5::NoiseBrushI
{
	//
	TPtr< VX5::EngineI >	voxel_engine;
	VX5::MyDebugDraw		voxel_engine_dbg_drawer;

	//
	PlaneD						_test_plane;
	BSphereD					_test_sphere;
	TPtr< VX5::WorldI >			voxel_world;
	CubeMLf						world_bounds;

	//
	VX5::WorldDatabases_LMDB2	voxel_databases;
	//VX5::WorldStorage			dummy_voxel_databases;

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

	virtual void VX_GetBoxContentsInfo(
		VX5::ProcGen::BoxContentsInfo &box_contents_
		, const CubeMLd& box_in_world_space
		, const VX5::ProcGen::ChunkContext& context
		) override;

	virtual VX5::ProcGen::ProvidedChunkData VX_StartGeneratingChunk(
		VX5::ProcGen::ChunkConstants &chunk_constants_
		, const VX5::ProcGen::ChunkContext& context
		) override;

	virtual double VX_GetSignedDistanceAtPoint(
		const V3d& position_in_world_space
		, const VX5::ProcGen::ChunkContext& context
		, const VX5::ProcGen::ChunkConstants& scratchpad
		) override;

public:	// VX5::NoiseBrushI
	virtual float01_t	GetStrength(const V3f& pos) override;
};


/*
-----------------------------------------------------------------------------
	SgWorld
-----------------------------------------------------------------------------
*/
class SgWorld
	: NwNonCopyable

#if GAME_CFG_WITH_VOXELS
	, public VX5::EngineListenerI
	, public VX5::MeshBakerI
	, public VX5::MeshLoaderI
	, public VX5::WorldListenerI
#endif

{
public:	// Rendering
	TPtr< SpatialDatabaseI >		spatial_database;

public:	// Voxels.

#if GAME_CFG_WITH_VOXELS
	MyVoxels	voxels;
#endif

public:	// Physics

#if GAME_CFG_WITH_PHYSICS

	NwPhysicsWorld				physics_world;
	TbBullePhysicsDebugDrawer	_physics_debug_drawer;

#else

	NwPhysicsWorld &			physics_world;

#endif // GAME_CFG_WITH_PHYSICS


public:	// Object storage
	//
	TPtr< NwClump >		_scene_clump;


	//
	DynamicArray<NwModel*>	models_to_delete;
	NwModel*				models_to_delete_embedded_storage[16];

	//
	AllocatorI &		_allocator;

public:
	SgWorld( AllocatorI& allocator );

	ERet initialize(
		const SgAppSettings& game_settings
		, NwClump* scene_clump
		, NwRenderSystemI* render_system
		, AllocatorI& scratchpad
		);

	void shutdown();

	void UpdateOncePerFrame(
		const NwTime& game_time
		, SgHumanPlayer& game_player
		, const SgAppSettings& game_settings
		, SgGameApp& game
		);

public:
	void RequestDeleteModel(NwModel* model_to_delete);

private:
	void _DeletePendingModels();




private:	// Voxels.

#if GAME_CFG_WITH_VOXELS

	ERet _InitVoxelEngine(
		const SgAppSettings& game_settings
		, NwClump* scene_clump
		, AllocatorI& scratchpad
		);
	void _DeinitVoxelEngine();

#endif





private:	// Physics.

#if GAME_CFG_WITH_PHYSICS

	ERet _InitPhysics(
		NwClump * storage_clump 
		, TbPrimitiveBatcher & debug_renderer
		);
	void _DeinitPhysics();

	void _UpdatePhysics(
		const NwTime& game_time
		, SgHumanPlayer& game_player
		, const SgAppSettings& game_settings
		);

	void Physics_AddGroundPlane_for_Collision();

#endif // GAME_CFG_WITH_PHYSICS

private:


#if GAME_CFG_WITH_VOXELS
	static VX5::WorldI* _CreateVoxelPlanet(
		const CubeMLd& world_bounds
		, const BSphereD& sphere
		, VX5::EngineI* voxel_engine
		, VX5::ProcGen::ChunkGeneratorI& chunk_generator
		, MeshBakerI& mesh_baker
		, MeshLoaderI& mesh_loader
		, VX5::WorldListenerI* world_delegate
		, const VX5::WorldStorageConfiguration& world_storage_config
		, const VX5::MaterialID default_solid_material
		, CubeMLf &world_bounds_
		, VX5::RunTimeWorldSettings* initial_world_settings = nil
		);
#endif

public:	// Debugging
	void debugDrawVoxels(
		const NwFloatingOrigin& floating_origin
		, const NwCameraView& camera_view
		) const;

	void regenerateVoxelTerrains();




#if GAME_CFG_WITH_VOXELS

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

public:	// VX5::EngineListenerI

	virtual void VX_OnError(
		const char* error_message
	) override
	{
		DBGOUT("%s", error_message);
	}

#endif // #if GAME_CFG_WITH_VOXELS

};
