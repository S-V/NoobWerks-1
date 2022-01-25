#include <Lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <Base/Base.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Template/Containers/BitSet/BitArray.h>
#include <Base/Text/String.h>
#include <Base/Math/ViewFrustum.h>

#include <bx/bx.h>	// uint8_t

//#include <libnoise/noise.h>
//#pragma comment( lib, "libnoise.lib" )

#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Graphics/private/d3d_common.h>
#include <Driver/Driver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/TxTSupport/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Renderer/Core/Material.h>
#include <Renderer/private/RenderSystem.h>
#include <Renderer/private/shader_uniforms.h>
#include <Renderer/Shaders/__shader_globals_common.h>	// G_VoxelTerrain

#include <Utility/MeshLib/TriMesh.h>

#include <Utility/VoxelEngine/VoxelTerrainChunk.h>
#include <Utility/VoxelEngine/VoxelTerrainRenderer.h>

#include "TowerDefenseGame.h"
#include <Voxels/Utility/vx5_utilities.h>

const VX5::ChunkID g_GI_MIN_CHUNK_ID( 0, 0, 0, 0 );
const VX5::ChunkID g_GI_MAX_CHUNK_ID( 3, 3, 3, 0 );	// inclusive

static AABBf getVoxelVolumeAabbInWorldSpace( const VX5::OctreeWorld& world )
{
	const AABBf min_chunk_aabb = AABBf::fromOther(world._octree.getNodeAABB( g_GI_MIN_CHUNK_ID ));
	const AABBf max_chunk_aabb = AABBf::fromOther(world._octree.getNodeAABB( g_GI_MAX_CHUNK_ID ));
	return AABBf::make( min_chunk_aabb.min_corner, max_chunk_aabb.max_corner );
}

void TowerDefenseGame::_gi_debugDraw()
{
//	const AABBf voxel_volume_aabb_in_world_space = getVoxelVolumeAabbInWorldSpace( _voxel_world );
//	_render_system->m_debugRenderer.DrawAABB( voxel_volume_aabb_in_world_space.min_corner, voxel_volume_aabb_in_world_space.max_corner, RGBAf::WHITE );

}

namespace {

	static mxFORCEINLINE
	V4f UByte4_to_V4f( const UByte4& src )
	{
		return CV4f(
			_UInt8ToNormalizedFloat( src.x ),
			_UInt8ToNormalizedFloat( src.y ),
			_UInt8ToNormalizedFloat( src.z ),
			_UInt8ToNormalizedFloat( src.w )
			);
	}

	static mxFORCEINLINE
	UByte4 V4f_to_UByte4( const V4f& src )
	{
		UByte4	result;
		result.x = _NormalizedFloatToUInt8( src.x );
		result.y = _NormalizedFloatToUInt8( src.y );
		result.z = _NormalizedFloatToUInt8( src.z );
		result.w = _NormalizedFloatToUInt8( src.w );
		return result;
	}


	static const UByte4 sampleAlbedoAndDensity( const VX5::MaterialID* voxels
		, UINT iTextureVoxelX
		, UINT iTextureVoxelY
		, UINT iTextureVoxelZ
		, UINT chunk_voxels_per_texture_voxel	// the length of a texture voxel in terrain voxels
		)
	{
		const UINT iStartCellX = ( iTextureVoxelX * chunk_voxels_per_texture_voxel ) + VX5::CHUNK_MARGIN_MIN;
		const UINT iStartCellY = ( iTextureVoxelY * chunk_voxels_per_texture_voxel ) + VX5::CHUNK_MARGIN_MIN;
		const UINT iStartCellZ = ( iTextureVoxelZ * chunk_voxels_per_texture_voxel ) + VX5::CHUNK_MARGIN_MIN;

		const UINT iEndCellX = iStartCellX + chunk_voxels_per_texture_voxel;
		const UINT iEndCellY = iStartCellY + chunk_voxels_per_texture_voxel;
		const UINT iEndCellZ = iStartCellZ + chunk_voxels_per_texture_voxel;

		const int voxel_block_size = ToCube( chunk_voxels_per_texture_voxel );

		int	num_solid_voxels_in_block = 0;
		V4f average_color_in_block = CV4f(0);

		for( int iCellZ = iStartCellZ; iCellZ < iEndCellZ; iCellZ++ )	// for each horizontal layer
		{
			for( int iCellY = iStartCellY; iCellY < iEndCellY; iCellY++ )
			{
				for( int iCellX = iStartCellX; iCellX < iEndCellX; iCellX++ )
				{
					const UINT cell_index = VX5::getChunkCellIndex( iCellX, iCellY, iCellZ );

					const UINT iVoxelIndex0 = VX5::getChunkVoxelIndex( iCellX,   iCellY,   iCellZ );
					const UINT iVoxelIndex1 = VX5::getChunkVoxelIndex( iCellX+1, iCellY,   iCellZ );
					const UINT iVoxelIndex2 = VX5::getChunkVoxelIndex( iCellX,   iCellY+1, iCellZ );
					const UINT iVoxelIndex3 = VX5::getChunkVoxelIndex( iCellX+1, iCellY+1, iCellZ );

					const UINT iVoxelIndex4 = VX5::getChunkVoxelIndex( iCellX,   iCellY,   iCellZ+1 );
					const UINT iVoxelIndex5 = VX5::getChunkVoxelIndex( iCellX+1, iCellY,   iCellZ+1 );
					const UINT iVoxelIndex6 = VX5::getChunkVoxelIndex( iCellX,   iCellY+1, iCellZ+1 );
					const UINT iVoxelIndex7 = VX5::getChunkVoxelIndex( iCellX+1, iCellY+1, iCellZ+1 );

					const UINT voxel0 = voxels[ iVoxelIndex0 ];
					const UINT voxel1 = voxels[ iVoxelIndex1 ];
					const UINT voxel2 = voxels[ iVoxelIndex2 ];
					const UINT voxel3 = voxels[ iVoxelIndex3 ];

					const UINT voxel4 = voxels[ iVoxelIndex4 ];
					const UINT voxel5 = voxels[ iVoxelIndex5 ];
					const UINT voxel6 = voxels[ iVoxelIndex6 ];
					const UINT voxel7 = voxels[ iVoxelIndex7 ];

					num_solid_voxels_in_block +=
						MapTo01(voxel0) + MapTo01(voxel1) + MapTo01(voxel2) + MapTo01(voxel3) +
						MapTo01(voxel4) + MapTo01(voxel5) + MapTo01(voxel6) + MapTo01(voxel7) ;

					//
					const UByte4 voxel_color0 = Demos::g_average_terrain_material_colors[ voxel0 ];
					const UByte4 voxel_color1 = Demos::g_average_terrain_material_colors[ voxel1 ];
					const UByte4 voxel_color2 = Demos::g_average_terrain_material_colors[ voxel2 ];
					const UByte4 voxel_color3 = Demos::g_average_terrain_material_colors[ voxel3 ];
																							   
					const UByte4 voxel_color4 = Demos::g_average_terrain_material_colors[ voxel4 ];
					const UByte4 voxel_color5 = Demos::g_average_terrain_material_colors[ voxel5 ];
					const UByte4 voxel_color6 = Demos::g_average_terrain_material_colors[ voxel6 ];
					const UByte4 voxel_color7 = Demos::g_average_terrain_material_colors[ voxel7 ];

					//
					const V4f tmp0 = UByte4_to_V4f( voxel_color0 );
					const V4f tmp1 = UByte4_to_V4f( voxel_color1 );
					const V4f tmp2 = UByte4_to_V4f( voxel_color2 );
					const V4f tmp3 = UByte4_to_V4f( voxel_color3 );

					const V4f tmp4 = UByte4_to_V4f( voxel_color4 );
					const V4f tmp5 = UByte4_to_V4f( voxel_color5 );
					const V4f tmp6 = UByte4_to_V4f( voxel_color6 );
					const V4f tmp7 = UByte4_to_V4f( voxel_color7 );

					//
					V4f	sum = (tmp0 + tmp1 + tmp2 + tmp3) + (tmp4 + tmp5 + tmp6 + tmp7);
					V4f	avg = V4f::scale( sum, 0.125f );

					average_color_in_block += avg;
				}//X
			}//Y
		}//Z

		average_color_in_block = V4f::scale( average_color_in_block, (1.0f / voxel_block_size) );
		const float density = float( num_solid_voxels_in_block ) / voxel_block_size;

		UByte4	texture_voxel;
		texture_voxel = V4f_to_UByte4( average_color_in_block );
		texture_voxel.a = _NormalizedFloatToUInt8( density );

		return texture_voxel;
	}

	static ERet _fillTextureRegionWithChunkData( void *texture_data_
		, const RrVolumeTextureCommonSettings& vct_settings
		, const UInt3& chunk_offset
		, const VX5::ChunkID& chunk_id
		, const VX5::ChunkData& chunk_data
		, const VX5::OctreeWorld& world
		)
	{
		const int volume_texture_dimension_in_LoD0_chunks = 2;// ( 2 << vct_settings.vol_radius_log2 );
		const int volume_texture_resolution = ( 1 << vct_settings.vol_tex_res_log2 );
		const UInt3	volume_texture_size_3d( volume_texture_resolution );

		// assume that the terrain chunk's resolution is higher than or equal to the volume texture's resolution
		const U32 chunk_dimension_in_texture_voxels = volume_texture_resolution / volume_texture_dimension_in_LoD0_chunks;
		mxASSERT(chunk_dimension_in_texture_voxels >= 1);

		// the length of a texture voxel in terrain voxels:
		const U32 chunk_voxels_per_texture_voxel = VX5::CHUNK_CELLS_DIM / chunk_dimension_in_texture_voxels;

		const U32 start_voxel_x_offset_in_texture = chunk_offset.x * chunk_dimension_in_texture_voxels;
		const U32 start_voxel_y_offset_in_texture = chunk_offset.y * chunk_dimension_in_texture_voxels;
		const U32 start_voxel_z_offset_in_texture = chunk_offset.z * chunk_dimension_in_texture_voxels;

		const U32 end_voxel_x_offset_in_texture = start_voxel_x_offset_in_texture + chunk_dimension_in_texture_voxels;
		const U32 end_voxel_y_offset_in_texture = start_voxel_y_offset_in_texture + chunk_dimension_in_texture_voxels;
		const U32 end_voxel_z_offset_in_texture = start_voxel_z_offset_in_texture + chunk_dimension_in_texture_voxels;

		//
		UByte4	*dst_texture_data = (UByte4*) texture_data_;	// RGBA8

		const VX5::MaterialID* voxels = chunk_data._voxels.raw();

		for( UINT iZ = start_voxel_z_offset_in_texture; iZ < end_voxel_z_offset_in_texture; iZ++ )
		{
			for( UINT iY = start_voxel_y_offset_in_texture; iY < end_voxel_y_offset_in_texture; iY++ )
			{
				for( UINT iX = start_voxel_x_offset_in_texture; iX < end_voxel_x_offset_in_texture; iX++ )
				{
					const U32 voxel_offset_in_texture = GetFlattenedIndexV( UInt3( iX, iY, iZ ), volume_texture_size_3d );

					const UByte4 texture_voxel = sampleAlbedoAndDensity( voxels,
						iX-start_voxel_x_offset_in_texture,
						iY-start_voxel_y_offset_in_texture,
						iZ-start_voxel_z_offset_in_texture,
						chunk_voxels_per_texture_voxel );
//					UByte4 texture_voxel;	texture_voxel.v = ~0;
//					if( chunk_offset == UInt3(0) )
						dst_texture_data[ voxel_offset_in_texture ] = texture_voxel;
					//else
					//	dst_texture_data[ voxel_offset_in_texture ].v = 0;
				}//X
			}//Y
		}//Z


#if 0
		// For each internal cell of the chunk (i.e. excluding margins):

		for( int iCellZ = VX5::CHUNK_MARGIN; iCellZ < VX5::CHUNK_SIZE_CELLS - VX5::CHUNK_MARGIN; iCellZ++ )	// for each horizontal layer
		{
			for( int iCellY = VX5::CHUNK_MARGIN; iCellY < VX5::CHUNK_SIZE_CELLS - VX5::CHUNK_MARGIN; iCellY++ )
			{
				for( int iCellX = VX5::CHUNK_MARGIN; iCellX < VX5::CHUNK_SIZE_CELLS - VX5::CHUNK_MARGIN; iCellX++ )
				{
					const UINT cell_index = VX5::getChunkCellIndex( iCellX, iCellY, iCellZ );

					const UINT iVoxelIndex0 = VX5::getChunkVoxelIndex( iCellX,   iCellY,   iCellZ );
					const UINT iVoxelIndex1 = VX5::getChunkVoxelIndex( iCellX+1, iCellY,   iCellZ );
					const UINT iVoxelIndex2 = VX5::getChunkVoxelIndex( iCellX,   iCellY+1, iCellZ );
					const UINT iVoxelIndex3 = VX5::getChunkVoxelIndex( iCellX+1, iCellY+1, iCellZ );

					const UINT iVoxelIndex4 = VX5::getChunkVoxelIndex( iCellX,   iCellY,   iCellZ+1 );
					const UINT iVoxelIndex5 = VX5::getChunkVoxelIndex( iCellX+1, iCellY,   iCellZ+1 );
					const UINT iVoxelIndex6 = VX5::getChunkVoxelIndex( iCellX,   iCellY+1, iCellZ+1 );
					const UINT iVoxelIndex7 = VX5::getChunkVoxelIndex( iCellX+1, iCellY+1, iCellZ+1 );

					const UINT voxel0 = voxels[ iVoxelIndex0 ];
					const UINT voxel1 = voxels[ iVoxelIndex1 ];
					const UINT voxel2 = voxels[ iVoxelIndex2 ];
					const UINT voxel3 = voxels[ iVoxelIndex3 ];

					const UINT voxel4 = voxels[ iVoxelIndex4 ];
					const UINT voxel5 = voxels[ iVoxelIndex5 ];
					const UINT voxel6 = voxels[ iVoxelIndex6 ];
					const UINT voxel7 = voxels[ iVoxelIndex7 ];

					const float density =
						( MapTo01(voxel0) + MapTo01(voxel1) + MapTo01(voxel2) + MapTo01(voxel3)
						+ MapTo01(voxel4) + MapTo01(voxel5) + MapTo01(voxel6) + MapTo01(voxel7) ) * 0.125f
						;

					const U32 voxel_offset_in_texture = GetFlattenedIndexV(
						UInt3(
						start_voxel_x_offset_in_texture + iCellX - VX5::CHUNK_MARGIN,
						start_voxel_y_offset_in_texture + iCellY - VX5::CHUNK_MARGIN,
						start_voxel_z_offset_in_texture + iCellZ - VX5::CHUNK_MARGIN
						),
						volume_texture_size_3d
						);


					UByte4	texture_voxel;
					texture_voxel.v = 0;
					texture_voxel.a = _NormalizedFloatToUInt8( density );

					dst_texture_data[ voxel_offset_in_texture ] = texture_voxel;

					//
				}//X
			}//Y
		}//Z
#endif
		return ALL_OK;
	}

	static void downsampleRGBA8( UByte4 *dst_, const UByte4* src, const UInt3& src_dims )
	{
		mxASSERT( src_dims.x >= 2 && src_dims.y >= 2 && src_dims.z >= 2 );
		const UInt3 dst_dims = UInt3::div( src_dims, 2 );

		for( int iDstZ = 0; iDstZ < dst_dims.z; iDstZ++ )	// for each horizontal layer
		{
			for( int iDstY = 0; iDstY < dst_dims.y; iDstY++ )
			{
				for( int iDstX = 0; iDstX < dst_dims.x; iDstX++ )
				{
					const U32 dst_voxel_index = GetFlattenedIndexV( UInt3( iDstX, iDstY, iDstZ ), dst_dims );

					//
					const UByte4 voxel0 = src[ GetFlattenedIndexV_NoCheck( UInt3( iDstX*2 + 0, iDstY*2 + 0, iDstZ*2 + 0 ), src_dims ) ];
					const UByte4 voxel1 = src[ GetFlattenedIndexV_NoCheck( UInt3( iDstX*2 + 1, iDstY*2 + 0, iDstZ*2 + 0 ), src_dims ) ];
					const UByte4 voxel2 = src[ GetFlattenedIndexV_NoCheck( UInt3( iDstX*2 + 0, iDstY*2 + 1, iDstZ*2 + 0 ), src_dims ) ];
					const UByte4 voxel3 = src[ GetFlattenedIndexV_NoCheck( UInt3( iDstX*2 + 1, iDstY*2 + 1, iDstZ*2 + 0 ), src_dims ) ];
																										
					const UByte4 voxel4 = src[ GetFlattenedIndexV_NoCheck( UInt3( iDstX*2 + 0, iDstY*2 + 0, iDstZ*2 + 1 ), src_dims ) ];
					const UByte4 voxel5 = src[ GetFlattenedIndexV_NoCheck( UInt3( iDstX*2 + 1, iDstY*2 + 0, iDstZ*2 + 1 ), src_dims ) ];
					const UByte4 voxel6 = src[ GetFlattenedIndexV_NoCheck( UInt3( iDstX*2 + 0, iDstY*2 + 1, iDstZ*2 + 1 ), src_dims ) ];
					const UByte4 voxel7 = src[ GetFlattenedIndexV_NoCheck( UInt3( iDstX*2 + 1, iDstY*2 + 1, iDstZ*2 + 1 ), src_dims ) ];

					//
					V4f	tmp0 = UByte4_to_V4f( voxel0 );
					V4f	tmp1 = UByte4_to_V4f( voxel1 );
					V4f	tmp2 = UByte4_to_V4f( voxel2 );
					V4f	tmp3 = UByte4_to_V4f( voxel3 );

					V4f	tmp4 = UByte4_to_V4f( voxel4 );
					V4f	tmp5 = UByte4_to_V4f( voxel5 );
					V4f	tmp6 = UByte4_to_V4f( voxel6 );
					V4f	tmp7 = UByte4_to_V4f( voxel7 );

					//
					V4f	sum = (tmp0 + tmp1 + tmp2 + tmp3) + (tmp4 + tmp5 + tmp6 + tmp7);
					V4f	avg = V4f::scale( sum, 0.125f );

					//
					dst_[ dst_voxel_index ] = V4f_to_UByte4( avg );
				}//X
			}//Y
		}//Z
	}

	static UINT calcVolumeTextureSize( UINT volume_texture_resolution )
	{
		const UINT num_voxels_in_volume_texture = ToCube( volume_texture_resolution );
		const UINT num_bits_per_pixel_in_volume_texture = DataFormat::BitsPerPixel( RrVoxelConeTracing::VOXEL_RADIANCE_TEXTURE_FORMAT );
		const UINT size_of_volume_texture = BITS_TO_BYTES( num_voxels_in_volume_texture * num_bits_per_pixel_in_volume_texture );
		return size_of_volume_texture;
	}

}//namespace

ERet TowerDefenseGame::_gi_bakeAlbedoDensityVolumeTexture( const CameraMatrices& camera_matrices )
{
#if 0
	Rendering::VoxelConeTracing &	voxel_cone_tracing = Rendering::VoxelConeTracing::Get();

	RrVolumeTextureCommonSettings & vct_settings = Rendering::g_settings.gi.vct;
	const int volume_texture_dimension_in_LoD0_chunks = ( 2 << vct_settings.vol_radius_log2 );
	const int volume_texture_resolution = ( 1 << vct_settings.vol_tex_res_log2 );

	//
	TextureRegion	texture_update_region;
	{
		texture_update_region.x	= 0;
		texture_update_region.y	= 0;
		texture_update_region.z	= 0;
		texture_update_region.width		= volume_texture_resolution;
		texture_update_region.height	= volume_texture_resolution;
		texture_update_region.depth		= volume_texture_resolution;
		texture_update_region.arraySlice = 0;
		texture_update_region.mipMapLevel	= 0;
	}

	const UINT size_of_volume_texture = calcVolumeTextureSize( volume_texture_resolution );

	IF_DEVELOPER LogStream(LL_Info),"Baking radiance volume: ",(size_of_volume_texture/mxMEBIBYTE)," MiB";

	//
	const int volume_dim_in_terrain_cells = volume_texture_dimension_in_LoD0_chunks * VX5::CHUNK_DIM;
	const int volume_dim_in_texture_voxels = volume_texture_resolution;


	//
	const AABBf voxel_volume_aabb_in_world_space = getVoxelVolumeAabbInWorldSpace( _voxel_world );
	vct_settings.volume_min_corner_in_world_space = voxel_volume_aabb_in_world_space.mins;
	vct_settings.vol_tex_dim_in_world_space = voxel_volume_aabb_in_world_space.size().x;
	vct_settings.volume_origin_in_view_space = voxel_volume_aabb_in_world_space.mins - camera_matrices.origin;

	voxel_cone_tracing.modify( vct_settings );


	//
	const GL::Memory* volume_texture_data = GL::allocate( size_of_volume_texture );
	mxENSURE( volume_texture_data, ERR_OUT_OF_MEMORY, "" );

	//
	AllocatorI	& scratch = MemoryHeaps::temporary();

#if 1
	for( int iChunkZ = 0; iChunkZ < volume_texture_dimension_in_LoD0_chunks; iChunkZ++ )
	{
		for( int iChunkY = 0; iChunkY < volume_texture_dimension_in_LoD0_chunks; iChunkY++ )
		{
			for( int iChunkX = 0; iChunkX < volume_texture_dimension_in_LoD0_chunks; iChunkX++ )
			{
				const VX5::ChunkID	chunk_id( iChunkX, iChunkY, iChunkZ );

				//
				VX5::ChunkInfo	chunk_metadata;

				mxTEMP		
				const bool chunk_exists_in_DB = mxSUCCEDED(_voxel_world.m_indexDB.Get( chunk_id.M, &chunk_metadata ));
				if( !chunk_exists_in_DB ) {
					LogStream(LL_Warn) << "Chunk " << chunk_id << " doesn't exist in the database!";
					continue;
				}

				//
				VX5::ChunkData	chunk_data( scratch );
				if( chunk_metadata.IsHomogeneous() ) {
					chunk_data.fillWith( chunk_metadata.homogeneous_materialID );
				} else {
					mxDO(chunk_data.loadFromDatabase( chunk_id, _voxel_world.m_voxelDB ));
				}

				//
				_fillTextureRegionWithChunkData( volume_texture_data->data
					, vct_settings
					, UInt3( iChunkX, iChunkY, iChunkZ )
					, chunk_id
					, chunk_data
					, _voxel_world
					);
			}//X
		}//Y
	}//Z
#else

	//memset( volume_texture_data->data, ~0, size_of_volume_texture );
	{
		UByte4 *dst_texture_data = (UByte4*) volume_texture_data->data;

		for( UINT iZ = 0; iZ < 32; iZ++ )
		{
			for( UINT iY = 0; iY < 32; iY++ )
			{
				for( UINT iX = 0; iX < 32; iX++ )
				{
					const U32 voxel_offset_in_texture = GetFlattenedIndexV( UInt3( iX, iY, iZ ), UInt3(32) );

					if( iZ < 16 )
					{
						dst_texture_data[ voxel_offset_in_texture ].v = ~0;
					}
					else
					{
						dst_texture_data[ voxel_offset_in_texture ].v = 0;
					}
				}//X
			}//Y
		}//Z
	}


#endif

	//
	GL::updateTexture( voxel_cone_tracing._radiance_opacity_texture
		, texture_update_region
		, volume_texture_data
		);

	//
	// Update coarse mips.
	//

	const UByte4* prev_fine_mip_data = (UByte4*) volume_texture_data->data;

	UINT coarse_mip_level_index_to_update = 1;

	UINT coarse_mip_res = volume_texture_resolution / 2;
	while( coarse_mip_res > 0 )
	{
		const GL::Memory* this_coarse_mip_data = GL::allocate( calcVolumeTextureSize( coarse_mip_res ) );
		mxENSURE( this_coarse_mip_data, ERR_OUT_OF_MEMORY, "" );

		downsampleRGBA8( (UByte4*) this_coarse_mip_data->data, prev_fine_mip_data, UInt3(coarse_mip_res*2) );

		texture_update_region.width		= coarse_mip_res;
		texture_update_region.height	= coarse_mip_res;
		texture_update_region.depth		= coarse_mip_res;
		texture_update_region.arraySlice = 0;
		texture_update_region.mipMapLevel = coarse_mip_level_index_to_update;

		//
		GL::updateTexture( voxel_cone_tracing._radiance_opacity_texture
			, texture_update_region
			, this_coarse_mip_data
			);

		coarse_mip_res /= 2;
		coarse_mip_level_index_to_update += 1;
		prev_fine_mip_data = (UByte4*) this_coarse_mip_data->data;
	}
#endif
	return ALL_OK;
}
