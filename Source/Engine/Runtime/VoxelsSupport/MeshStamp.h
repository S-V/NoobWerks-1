#pragma once

#include <Rendering/Public/Core/Mesh.h>

#include <Voxels/public/vx5.h>

#include <VoxelsSupport/VoxelTerrainMaterials.h>


//
class VXTriangleMeshWrapper: public VX5::TriangeMeshI, NonCopyable
{
	const Rendering::NwMesh* src_mesh;

public:
	VXTriangleMeshWrapper(const Rendering::NwMesh* src_mesh)
		: src_mesh(src_mesh)
	{
	}
	virtual AABBf VX_GetLocalAABB() const override
	{
		return src_mesh->local_aabb;
	}

	virtual int VX_GetVertexCount() const
	{
		return src_mesh->vertex_count;
	}

	virtual void VX_GetVertexData(
		const int vertex_index,
		float pos[3],
		float uv[2]
	) const
	{
		const MeshStampVertex* draw_verts = (MeshStampVertex*) src_mesh->raw_vertex_data.raw();
		const MeshStampVertex& src_vert = draw_verts[vertex_index];
		pos[0] = src_vert.xyz.a[0];
		pos[1] = src_vert.xyz.a[1];
		pos[2] = src_vert.xyz.a[2];

#if MESH_STAMP_SUPPORT_UV
		uv[0] = Half_To_Float(src_vert.uv.x);
		uv[1] = Half_To_Float(src_vert.uv.y);
#else
		uv[0] = 0;
		uv[1] = 0;
#endif
	}

	virtual int VX_GetTriangleCount() const
	{
		return src_mesh->index_count / 3;
	}

	virtual void VX_GetTriangleData(
		const int triangle_index,
		int vertex_indices[3],
		VX5::MaterialID &triangle_material_id,
		VX5::TextureID &triangle_texture_id
		) const
	{
		if(src_mesh->flags & Rendering::MESH_USES_32BIT_IB)
		{
			const Tuple3<U32>& src_tri32 = ((Tuple3<U32>*) src_mesh->raw_index_data.raw())[triangle_index];
			vertex_indices[0] = src_tri32.a[0];
			vertex_indices[1] = src_tri32.a[1];
			vertex_indices[2] = src_tri32.a[2];
		}
		else
		{
			const Tuple3<U16>& src_tri16 = ((Tuple3<U16>*) src_mesh->raw_index_data.raw())[triangle_index];
			vertex_indices[0] = src_tri16.a[0];
			vertex_indices[1] = src_tri16.a[1];
			vertex_indices[2] = src_tri16.a[2];
		}

		// use the material of the solid
		triangle_material_id = VX5::EMPTY_SPACE;

		// no texture
		triangle_texture_id.SetNil();
	}
};

namespace VXExt
{
	ERet LoadMeshForStamping(
		TResPtr<Rendering::NwMesh> & brush_mesh
		, NwClump& storage_clump
		);

}//namespace VXExt
