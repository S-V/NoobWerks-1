#include <Base/Base.h>
#pragma hdrstop

#include <VoxelsSupport/VoxelTerrainMaterials.h>
#include <VoxelsSupport/MeshStamp.h>


namespace VXExt
{
	ERet LoadMeshForStamping(
		TResPtr<Rendering::NwMesh> & brush_mesh
		, NwClump& storage_clump
		)
	{
		return brush_mesh.Load(
			&storage_clump
			, Rendering::NwMesh::LoadFlags::KeepMeshDataInRAM
			);
	}

}//namespace VXExt
