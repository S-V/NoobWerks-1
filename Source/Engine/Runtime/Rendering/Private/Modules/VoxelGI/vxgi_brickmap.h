//
#pragma once

#include <Base/Template/NwHandleTable.h>

#include <Rendering/Private/Modules/VoxelGI/Private/BoxAllocator.h>


#define VXGI_DEBUG_BRICKS	(MX_DEBUG)





/// no artifacts, moderate memory consumptions - a good compromise, recommended
#define VXGI_DISTANCE_FIELD_FORMAT_UNORM16	(1)

/// stair-stepping artifacts, minimal memory consumption
#define VXGI_DISTANCE_FIELD_FORMAT_UNORM8	(2)

/// no artifacts, excessive memory consumptions - should only be used for testing & debugging
#define VXGI_DISTANCE_FIELD_FORMAT_FLOAT	(0)



//#define VXGI_DISTANCE_FIELD_FORMAT_USED	(VXGI_DISTANCE_FIELD_FORMAT_UNORM16)
#define VXGI_DISTANCE_FIELD_FORMAT_USED		(VXGI_DISTANCE_FIELD_FORMAT_UNORM8)




namespace Rendering
{
namespace VXGI
{

mxDECLARE_32BIT_HANDLE(BrickHandle);


#if VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_FLOAT
	typedef float SDFValue;
#elif VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_UNORM16
	/// 16-bit unsigned-normalized-integer format
	typedef U16 SDFValue;
#elif VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_UNORM8
	/// 8-bit unsigned-normalized-integer format
	typedef U8 SDFValue;
#else
#	error Unknown SDF format!
#endif


class BrickMap: NonCopyable
{
public:
	///
	HTexture	h_sdf_brick_atlas_tex3d;
	float		inverse_sdf_brick_atlas_resolution;

	Graphics::BoxAllocator	_box_allocator;

	struct VoxelBrick
	{
		UShort3	offset_in_atlas_tex;
		U16		dim_1D;

		BrickHandle	my_handle;
		U32			pad16;
	};
	ASSERT_SIZEOF(VoxelBrick, 16);

	VoxelBrick  *	_bricks;
	NwHandleTable	_brick_handle_tbl;


public:
	BrickMap(AllocatorI& allocator);

	struct Config {
		U16	max_brick_count;
	public:
		Config() {
			max_brick_count = 1024;
		}
	};

	ERet Initialize(const Config& cfg);
	void Shutdown();

	const VoxelBrick& GetBrickByHandle(
		const BrickHandle brick_handle
	) const;

	ERet AllocBrick(
		BrickHandle &new_brick_handle_
		, const SDFValue* sdf_values
		, const U32 sdf_grid_dim_1D
		);
	ERet FreeBrick(
		const BrickHandle old_brick_handle
		);

	ERet UpdateBrick(
		const VoxelBrick& brick
		, const SDFValue* sdf_values
		, const U32 sdf_grid_dim_1D
		);
};

}//namespace VXGI
}//namespace Rendering
