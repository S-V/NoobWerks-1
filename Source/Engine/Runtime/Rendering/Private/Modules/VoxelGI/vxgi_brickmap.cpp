#include <Base/Base.h>
#pragma hdrstop

#include <Core/Tasking/TaskSchedulerInterface.h>

#include <GPU/Public/render_context.h>
#include <Graphics/Public/graphics_shader_system.h>

#include <Rendering/Private/Modules/VoxelGI/_vxgi_system.h>
#include <Rendering/Private/Modules/VoxelGI/vxgi_brickmap.h>


namespace Rendering
{
namespace VXGI
{

	enum {
		MIN_BRICK_DIM = 8,

		/// The resolution of the texture containing "dense" voxel bricks.
		// res = 128^3, size = 4 194 304
		// res = 256^3, size = 33 554 432
		nwBRICK_ATLAS_RES = 128
	};

BrickMap::BrickMap(AllocatorI& allocator)
	: _box_allocator(
		MIN_BRICK_DIM, MIN_BRICK_DIM, MIN_BRICK_DIM
		, nwBRICK_ATLAS_RES, nwBRICK_ATLAS_RES, nwBRICK_ATLAS_RES
		, allocator
	)
	, _bricks(nil)
	, _brick_handle_tbl(allocator)
{
}

ERet BrickMap::Initialize(const Config& cfg)
{
#if 0
	{
		NwTexture3DDescription	brickmap_texture_description;
		{
			brickmap_texture_description.format		= NwDataFormat::R32_UINT;
			brickmap_texture_description.width		= nwBRICKMAP_RES;
			brickmap_texture_description.height		= nwBRICKMAP_RES;
			brickmap_texture_description.depth		= nwBRICKMAP_RES;
			brickmap_texture_description.numMips	= 1;
			// Leave default flags, because the texture will be updated via UpdateSubresource().
		}

		DBGOUT("Creating voxel brickmap: res=%d^3, size=%d",
			nwBRICKMAP_RES, brickmap_texture_description.CalcRawSize()
			);

		//
		h_brickmap_tex = GL::createTexture3D(
			brickmap_texture_description
			, nil
			IF_DEVELOPER, "Brickmap"
			);
	}
#endif
#if 1
	//
	{
		NwTexture3DDescription	brick_atlas_texture_description;
		{
#if VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_FLOAT
			brick_atlas_texture_description.format		= NwDataFormat::R32F;
#elif VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_UNORM16
			brick_atlas_texture_description.format		= NwDataFormat::R16_UNORM;
#elif VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_UNORM8
			brick_atlas_texture_description.format		= NwDataFormat::R8_UNORM;
#else
#	error Unknown SDF format!
#endif
			brick_atlas_texture_description.width		= nwBRICK_ATLAS_RES;
			brick_atlas_texture_description.height		= nwBRICK_ATLAS_RES;
			brick_atlas_texture_description.depth		= nwBRICK_ATLAS_RES;
			brick_atlas_texture_description.numMips		= 1;
			// Leave default flags, because the texture will be updated via UpdateSubresource().
		}

		DBGOUT("Creating voxel brick atlas: res=%d^3, size=%d",
			nwBRICK_ATLAS_RES, brick_atlas_texture_description.CalcRawSize()
			);

		//
		h_sdf_brick_atlas_tex3d = NGpu::createTexture3D(
			brick_atlas_texture_description
			, nil
			IF_DEVELOPER, "BrickAtlas"
			);
		inverse_sdf_brick_atlas_resolution = 1.0f / nwBRICK_ATLAS_RES;
	}
#endif

	mxDO(nwAllocArray(
		_bricks
		, cfg.max_brick_count
		, _brick_handle_tbl._allocator
		));

	mxDO(_brick_handle_tbl.InitWithMaxCount(
		cfg.max_brick_count
		));

	return ALL_OK;
}

void BrickMap::Shutdown()
{
	NGpu::DeleteTexture( h_sdf_brick_atlas_tex3d );
	h_sdf_brick_atlas_tex3d.SetNil();

	//
	_brick_handle_tbl.Shutdown();

	mxDELETE_AND_NIL(
		_bricks
		, _brick_handle_tbl._allocator
		);
}

const BrickMap::VoxelBrick& BrickMap::GetBrickByHandle(
	const BrickHandle brick_handle
	) const
{
	const U32 brick_index = _brick_handle_tbl.GetItemIndexByHandle(brick_handle.id);
	return _bricks[ brick_index ];
}

ERet BrickMap::AllocBrick(
	BrickHandle &new_brick_handle_
	, const SDFValue* sdf_values
	, const U32 sdf_grid_dim_1D
	)
{
	//
	U32	brick_base_offset_x, brick_base_offset_y, brick_base_offset_z;
	mxDO(_box_allocator.AddElement(
		brick_base_offset_x, brick_base_offset_y, brick_base_offset_z
		, sdf_grid_dim_1D, sdf_grid_dim_1D, sdf_grid_dim_1D
		));


	//
	const UINT new_brick_index = _brick_handle_tbl._num_alive_items;

	BrickHandle	new_brick_handle;
	mxDO(_brick_handle_tbl.AllocHandle(
		new_brick_handle.id
		));
	new_brick_handle_ = new_brick_handle;

	//
	VoxelBrick& new_brick = _bricks[ new_brick_index ];
	new_brick.offset_in_atlas_tex = UShort3::set(
		brick_base_offset_x, brick_base_offset_y, brick_base_offset_z
		);
	new_brick.dim_1D = sdf_grid_dim_1D;
	new_brick.my_handle = new_brick_handle;


	//
	UpdateBrick(
		new_brick
		, sdf_values
		, sdf_grid_dim_1D
		);

	return ALL_OK;
}

ERet BrickMap::FreeBrick(
	const BrickHandle old_brick_handle
	)
{
	VoxelBrick* old_brick = _brick_handle_tbl.SafeGetItemByHandle(
		old_brick_handle.id,
		_bricks
		);
	mxENSURE(old_brick, ERR_INVALID_PARAMETER, "");

	//
	_box_allocator.RemoveElement(
		old_brick->offset_in_atlas_tex.x,
		old_brick->offset_in_atlas_tex.y,
		old_brick->offset_in_atlas_tex.z
		,
		old_brick->dim_1D,
		old_brick->dim_1D,
		old_brick->dim_1D
		);

	// Doesn't work - the allocated brick is overwritten with debug values.
#if 0//VXGI_DEBUG_BRICKS
	{
		const UINT num_voxels_in_brick = ToCube(old_brick->dim_1D);
		SDFValue *	debug_values;
		if(mxSUCCEDED(__TryAllocate(debug_values, num_voxels_in_brick, threadLocalHeap())))
		{
			memset(debug_values, 0, sizeof(debug_values[0])*num_voxels_in_brick);

			UpdateBrick(
				*old_brick
				, debug_values
				, old_brick->dim_1D
				);
		}
	}
#endif

	//
	U32	indices_of_swapped_items[2];
	mxDO(_brick_handle_tbl.DestroyHandle(
		old_brick_handle.id
		, _bricks
		, indices_of_swapped_items
		));

	return ALL_OK;
}

ERet BrickMap::UpdateBrick(
	const VoxelBrick& brick
	, const SDFValue* sdf_values
	, const U32 sdf_grid_dim_1D
	)
{
	//
	NwTextureRegion	texture_update_region;
	{
		texture_update_region.x	= brick.offset_in_atlas_tex.x;
		texture_update_region.y	= brick.offset_in_atlas_tex.y;
		texture_update_region.z	= brick.offset_in_atlas_tex.z;
		texture_update_region.width		= brick.dim_1D;
		texture_update_region.height	= brick.dim_1D;
		texture_update_region.depth		= brick.dim_1D;
		texture_update_region.arraySlice = 0;
		texture_update_region.mipMapLevel	= 0;
	}

	//
	const size_t updated_texture_region_size = ToCube(sdf_grid_dim_1D) * sizeof(sdf_values[0]);

	//
	const NGpu::Memory* volume_texture_data = NGpu::Allocate( updated_texture_region_size );
	mxENSURE( volume_texture_data, ERR_OUT_OF_MEMORY, "" );

	//
	mxOPTIMIZE("avoid copies! write to mem directly, inside the voxel engine");
	memcpy(volume_texture_data->data, sdf_values, updated_texture_region_size);

	//
	NGpu::UpdateTexture(
		h_sdf_brick_atlas_tex3d
		, texture_update_region
		, volume_texture_data
		);

	return ALL_OK;
}

}//namespace VXGI
}//namespace Rendering
