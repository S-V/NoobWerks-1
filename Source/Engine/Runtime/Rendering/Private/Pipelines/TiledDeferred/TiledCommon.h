/*
=============================================================================
	File:	
	Desc:	
=============================================================================
*/
#pragma once

#include <Rendering/Public/Globals.h>

#include <Rendering/Private/Shaders/Shared/nw_shared_definitions.h>

/// Calculate the number of tiles in the horizontal direction
inline U32 CalculateNumTilesX( U32 screenWidth )
{
	// This is actually ceil( screenWidth / (float)TILE_SIZE_X )
	//return TAlignUp< TILE_SIZE_X >( screenWidth/TILE_SIZE_X );
	return (screenWidth + TILE_SIZE_X-1) / TILE_SIZE_X;
}
/// Calculate the number of tiles in the vertical direction
inline U32 CalculateNumTilesY( U32 screenHeight )
{
	//return TAlignUp< TILE_SIZE_Y >( screenHeight/TILE_SIZE_Y );
	return (screenHeight + TILE_SIZE_Y-1) / TILE_SIZE_Y;
}

/// Computes a compute shader dispatch size given a thread group size, and number of elements to process.
inline U32 DispatchSize( U32 thread_group_size, U32 element_count )
{
    U32 dispatchSize = element_count / thread_group_size;
    dispatchSize += ( element_count % thread_group_size > 0 ) ? 1 : 0;
    return dispatchSize;
}

ERet ComputeTileFrusta(
					   const float _screen_width,
					   const float _screen_height,
					   const M44f& _inverseXForm,	// pass inverse_projection_matrix to transform into view space
					   V4f *_verticalPlanes,
					   const U32 _numVerticalPlanes,
					   V4f *_horizontalPlanes,
					   const U32 _numHorizontalPlanes
					   );

///
class ReduceDepth_CS_Util: NonCopyable
{
	enum { MAX_READBACK_LATENCY_FRAMES = 4 };

	struct DepthReductionTarget
	{
		HColorTarget	handle;
		U16				width;
		U16				height;
	};

	DynamicArray< DepthReductionTarget >	_depth_reduction_targets;
	HTexture								_depth_reduction_staging_textures[ MAX_READBACK_LATENCY_FRAMES ];

	DepthReductionTarget					_depth_reduction_targets_storage[ 16 ];

public:
	ReduceDepth_CS_Util( AllocatorI & allocator );

	/// Creates the chain of render targets used for computing min/max depth.
	void createRenderTargets( unsigned screen_width, unsigned screen_height );

	/// Creates the staging textures for reading back the reduced depth buffer.
	void createReductionStagingTextures();

	void Shutdown();

public:
	/// Computes the min and max depth from the depth buffer using a parallel reduction.
	ERet reduceDepth_CS( const NGpu::LayerID view_id
		, const HDepthTarget depth_buffer
		, NGpu::NwRenderContext & render_context
		, NwClump * object_storage
		);

private:
	void releaseRenderTargets();
	void releaseReductionStagingTextures();
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
