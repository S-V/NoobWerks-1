#pragma once

#include <Base/Memory/FreeList/FreeList.h>	// FreeListAllocator
#include <Rendering/ForwardDecls.h>
#include <VoxelsSupport/VoxelTerrainMaterials.h>



namespace Rendering
{
	namespace VXGI
	{
		class VoxelGrids;
	}
}

struct VoxelTerrainChunk;


namespace VX5 {
class WorldI;
class DebugDrawI;
struct WorldDebugDrawSettings;
};


///
class VoxelTerrainRenderer: NonCopyable
{
public:
	VoxelTerrainMaterials		materials;

	TPtr< ObjectAllocatorI >	_assetStorage;		//!<
	FreeListAllocator	_chunkPool;	//!< memory storage for voxel terrain chunks

	TPtr<const Rendering::VXGI::VoxelGrids> _vxgi;

public:
	VoxelTerrainRenderer( AllocatorI& allocator )
		: materials( allocator )
	{
	}

	ERet Initialize(
		NGpu::NwRenderContext & context
		, AllocatorI & scratch
		, NwClump & storage

		// optional
		, const Rendering::VXGI::VoxelGrids* vxgi
		);

	void Shutdown();

	ERet DebugDrawWorldIfNeeded(
		const VX5::WorldI* world
		, VX5::DebugDrawI& debug_drawer
		, const VX5::WorldDebugDrawSettings& world_debug_draw_settings
		) const;
};
