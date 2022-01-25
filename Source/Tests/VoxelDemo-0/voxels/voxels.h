#pragma once


#include "common.h"

#include <Rendering/Public/Scene/CameraView.h>

#include <Voxels/public/vx5.h>
#include <Voxels/extensions/lmdb_storage.h>	// ChunkDatabase_LMDB

#include <VoxelsSupport/VoxelTerrainRenderer.h>


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
		TPtr< Rendering::SpatialDatabaseI >		_render_world;
		Rendering::NwFloatingOrigin			_floating_origin;
		Rendering::NwCameraView				scene_view;

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

		void Initialize(
			TbPrimitiveBatcher* prim_renderer
			, Rendering::SpatialDatabaseI* spatial_database
			);
		void Shutdown();

		ERet begin(
			const Rendering::NwFloatingOrigin& floating_origin
			, const Rendering::NwCameraView& scene_view
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





///
struct MyVoxels
	: public VX5::EngineListenerI
	, public VX5::MeshBakerI
	, public VX5::MeshLoaderI
	, public VX5::WorldControllerI
	, public VX5::WorldListenerI
	, public VX5::ProcGen::ChunkGeneratorI
	, public VX5::ProcGen::UserDataSourceFloatT
	//, public VX5::NoiseBrushI
{
	//
	TPtr< VX5::EngineI >	voxel_engine;
	VX5::MyDebugDraw		debug_drawer;

	//
	AabbCEd						_test_box;
	PlaneD						_test_plane;
	BSphereD					_test_sphere;
	TPtr< VX5::WorldI >			voxel_world;
	//
	VX5::WorldDatabases_LMDB2	voxel_databases;
	//VX5::WorldStorage			dummy_voxel_databases;

	//
	VoxelTerrainRenderer	_voxel_terrain_renderer;

	//// for craters
	//Noise::NwPerlinNoise	_perlin_noise;
	//Noise::NwTerrainNoise2D	_test_terrain_noise;

	TPtr< Rendering::SpatialDatabaseI >	spatial_database;
	TPtr< NwClump >		_scene_clump;

public:
	MyVoxels( AllocatorI& allocator )
		: debug_drawer( allocator )
		, _voxel_terrain_renderer( allocator )
	{
	}


	ERet Initialize(
		const NEngine::LaunchConfig & engine_settings
		, const MyAppSettings& app_settings
		, NwClump* scene_clump
		, Rendering::SpatialDatabaseI* spatial_database
		, const Rendering::VXGI::VoxelGrids* vxgi
		);
	void Shutdown();


private:
	ERet CreateVoxelBrickOnGPU(
		const VX5::ChunkID& chunk_id
		, VX5::WorldI& world
		);
	void DestroyVoxelBrickOnGPU(
		const VX5::ChunkID& chunk_id
		, VX5::WorldI& world
		);

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
	virtual VX_ChunkUserData* VX_LoadChunkUserData_MainThread(
		const VX5::ChunkLoadingInputs& args
		, VX5::WorldI & world
		) override;
	
	virtual void VX_UnloadChunkData_MainThread(
		VX_ChunkUserData* mesh
		, const VX5::ChunkID& chunk_id
		, VX5::WorldI& world
		) override;

public:	// WorldListenerI
	virtual void VX_OnChunkWasCreated_MainThread(
		const VX5::ChunkID& chunk_id
		, const VX5::ChunkLoadingInputs& loaded_chunk_data
		, VX5::WorldI& world
		) override;

	/// called before the chunk is destroyed
	virtual void VX_OnChunkWillBeDestroyed_MainThread(
		const VX5::ChunkID& chunk_id
		, VX5::WorldI& world
		) override;

	/// called after the chunk was modified (e.g. after editing)
	virtual void VX_OnChunkContentsDidChange(
		const VX5::ChunkID& chunk_id
		, const VX5::ChunkLoadingInputs& loaded_chunk_data
		, VX5::WorldI& world
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
		VX_ChunkUserData *&mesh_
		, const VX5::ChunkLoadingInputs& args
		);
	
	void VX_UnloadChunkData_MainThread_Internal(
		VX_ChunkUserData* mesh
		);

public:	// VX5::WorldControllerI
	virtual const VX5::RequestedChunkData GetRequestedChunkData_AnyThread(
		const VX5::ChunkID& chunk_id
		, VX_WorldUserData* user_world_data
		) const override;

public:	// VX5::WorldListenerI

	///// for showing loading screen
	//virtual void VX_OnLevelLoadProgress(
	//	const float01_t progress01
	//	) override;

	///// Used for spawning NPCs only after the level is fully loaded.
	///// LoDs are refined from top to bottom, 0 = index of the most detailed LoD.
	//virtual void VX_OnLevelLoaded() override;

public:	// VX5::EngineListenerI

	virtual void VX_OnError(
		const char* error_message
	) override
	{
		DBGOUT("%s", error_message);
	}

public:	// VX5::ChunkGeneratorI

	virtual const VX5::ProcGen::UserDataSourceFloatT& VX_GetDataSource_Float() const override;

	virtual void VX_GetBoxContentsInfo(
		VX5::ProcGen::BoxContentsInfo &box_contents_
		, const CubeMLd& box_in_world_space
		, const VX5::ProcGen::ChunkContext& context
		) const override;

	virtual VX5::ProcGen::ProvidedChunkData VX_StartGeneratingChunk(
		VX5::ProcGen::ChunkConstants &chunk_constants_
		, const VX5::ProcGen::ChunkContext& context
		) override;

	//virtual double VX_GetSignedDistanceAtPoint(
	//	const V3d& position_in_world_space
	//	, const VX5::ProcGen::ChunkContext& context
	//	, const VX5::ProcGen::ChunkConstants& scratchpad
	//	) override;

public:	// VX5::ProcGen::UserDataSourceFloatT

	virtual float VX_GetSignedDistanceAtPoint(
		const Tuple3<float>& position_in_world_space
		, const VX5::ProcGen::ChunkContext& chunk_context
		, const VX5::ProcGen::ChunkConstants& chunk_constants
		) const override;

public:	// VX5::NoiseBrushI
	//virtual float01_t	GetStrength(const V3f& pos) override;
};
