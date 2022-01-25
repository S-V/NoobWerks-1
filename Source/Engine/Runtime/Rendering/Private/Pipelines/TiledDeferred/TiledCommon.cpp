#include <Base/Base.h>
#pragma hdrstop
#include <Rendering/Private/Pipelines/TiledDeferred/TiledCommon.h>

ERet ComputeTileFrusta(
					   const float _screen_width,
					   const float _screen_height,
					   const M44f& _inverseXForm,	// pass inverse_projection_matrix to transform into view space
					   V4f *_verticalPlanes,
					   const U32 _numVerticalPlanes,
					   V4f *_horizontalPlanes,
					   const U32 _numHorizontalPlanes
					   )
{
	const U32 tilesX = CalculateNumTilesX(_screen_width);	// numHorizontalTiles
	const U32 tilesY = CalculateNumTilesY(_screen_height);	// numVerticalTiles

	// window width evenly divisible by tile width
	const float WW = (float) (TILE_SIZE_X * tilesX);//<= NOTE: cast is important!
	// window height evenly divisible by tile height
	const float HH = (float) (TILE_SIZE_Y * tilesY);//<= NOTE: must be float!

	const U32 verticalPlanes = smallest(tilesX+1, _numVerticalPlanes);
	for( U32 iTileX = 0; iTileX < verticalPlanes; iTileX++ )
	{
		const U32 x_viewport = TILE_SIZE_X * iTileX;
		const float x_NDC = (1.0f/_screen_width) * x_viewport * 2.0f - 1.0f;

		// left side of the frustum in clip space
		const V4f topCS		= V4f::set( x_NDC,  1.0f, 1.0f, 1.0f );
		const V4f bottomCS	= V4f::set( x_NDC, -1.0f, 1.0f, 1.0f );
		// left side of the frustum in view space
		const V3f topVS		= V4_As_V3(M44_Project( _inverseXForm, topCS ));
		const V3f bottomVS	= V4_As_V3(M44_Project( _inverseXForm, bottomCS ));

		// plane normal points left
		_verticalPlanes[ iTileX ] = Plane_FromThreePoints( V3_Zero(), topVS, bottomVS );
	}
	const U32 horizontalPlanes = smallest(tilesY+1, _numHorizontalPlanes);
	for( U32 iTileY = 0; iTileY < tilesY+1; iTileY++ )
	{
		const U32 y_viewport = TILE_SIZE_Y * iTileY;
		const float y_NDC = 1.0f - (1.0f/_screen_height) * y_viewport * 2.0f;

		const V4f leftCS	= V4f::set( -1.0f, y_NDC, 1.0f, 1.0f );
		const V4f rightCS	= V4f::set(  1.0f, y_NDC, 1.0f, 1.0f );
		const V3f leftVS	= V4_As_V3(M44_Project( _inverseXForm, leftCS ));
		const V3f rightVS	= V4_As_V3(M44_Project( _inverseXForm, rightCS ));

		// plane normal points up
		_horizontalPlanes[ iTileY ] = Plane_FromThreePoints( V3_Zero(), rightVS, leftVS );
	}

	return ALL_OK;
}

ReduceDepth_CS_Util::ReduceDepth_CS_Util( AllocatorI & allocator )
	: _depth_reduction_targets( allocator )
{
	Arrays::initializeWithExternalStorage( _depth_reduction_targets, _depth_reduction_targets_storage );
}

void ReduceDepth_CS_Util::createRenderTargets( unsigned screen_width, unsigned screen_height )
{
	NwColorTargetDescription	color_target_description;
	color_target_description.format = NwDataFormat::R16G16_UNORM;
	color_target_description.numMips = 1;
	color_target_description.flags = NwTextureCreationFlags::RANDOM_WRITES;

	//
	U32 w = screen_width;
	U32 h = screen_height;

	while( w > 1 || h > 1 )
	{
		w = DispatchSize( DEPTH_REDUCTION_TG_SIZE, w );
		h = DispatchSize( DEPTH_REDUCTION_TG_SIZE, h );

		color_target_description.width = w;
		color_target_description.height = h;

		DepthReductionTarget	depth_reduction_target;
		depth_reduction_target.handle = NGpu::CreateColorTarget(
			color_target_description
			IF_DEVELOPER , "DepthReductionRT"
			);
		depth_reduction_target.width = w;
		depth_reduction_target.height = h;
		
		_depth_reduction_targets.add( depth_reduction_target );
	}
}

void ReduceDepth_CS_Util::createReductionStagingTextures()
{
	NwTexture2DDescription	staging_texture_description;
	staging_texture_description.format = NwDataFormat::R16G16_UNORM;
	staging_texture_description.width = 1;
	staging_texture_description.height = 1;
	staging_texture_description.numMips = 1;

	for( int i = 0; i < mxCOUNT_OF(_depth_reduction_staging_textures); ++i )
	{
		_depth_reduction_staging_textures[i] = NGpu::createTexture2D(
			staging_texture_description
			, nil
			IF_DEVELOPER , "DepthReductionStaging"
			);
	}
}

void ReduceDepth_CS_Util::Shutdown()
{
	this->releaseReductionStagingTextures();
	this->releaseRenderTargets();
}

void ReduceDepth_CS_Util::releaseRenderTargets()
{
	for( int i = 0; i < _depth_reduction_targets.num(); ++i )
	{
		NGpu::DeleteColorTarget( _depth_reduction_targets[i].handle );
	}

	_depth_reduction_targets.RemoveAll();
}

void ReduceDepth_CS_Util::releaseReductionStagingTextures()
{
	for( int i = 0; i < mxCOUNT_OF(_depth_reduction_staging_textures); ++i )
	{
		NGpu::DeleteTexture( _depth_reduction_staging_textures[i] );
		_depth_reduction_staging_textures[i].SetNil();
	}
}

ERet ReduceDepth_CS_Util::reduceDepth_CS( const NGpu::LayerID view_id
										 , const HDepthTarget depth_buffer
										 , NGpu::NwRenderContext & render_context
										 , NwClump * object_storage
										 )
{
	//const U32 viewId_TiledShadingCS = m_renderPath->FindPassIndex( ScenePasses::TiledDeferred_CS );
	//mxASSERT(viewId_TiledShadingCS != ~0);

	// Compute shader setup (always does all the lights at once)
UNDONE;
	//TbShader* compute_shader;
	//mxDO(Resources::Load( compute_shader, MakeAssetID("cs_depth_reduction"), object_storage ));

	////
	//const U32 first_command_offset = render_context._command_buffer.currentOffset();

	//mxDO(FxSlow_SetInput( compute_shader, "DepthTexture", NGpu::AsInput(depth_buffer) ));

	////
	//NGpu::Cmd_DispatchCS cmd_dispatch_compute_shader;
	//{
	//	cmd_dispatch_compute_shader.clear();
	//	cmd_dispatch_compute_shader.encode( NGpu::CMD_DISPATCH_CS, 0, 0 );

	//	SetShaderParameters( render_context, *compute_shader );
	//	SetShaderOutputs( cmd_dispatch_compute_shader, *compute_shader );

	//	cmd_dispatch_compute_shader.groupsX = _depth_reduction_targets[0].width;
	//	cmd_dispatch_compute_shader.groupsY = _depth_reduction_targets[0].height;
	//	cmd_dispatch_compute_shader.groupsZ = 1;
	//}
	//mxDO(render_context._command_buffer.put( cmd_dispatch_compute_shader ));

	//
	//for( UINT iPass = 1; iPass < _depth_reduction_targets.num(); iPass++ )
	//{
	//	{
	//		SetShaderParameters( render_context, *compute_shader );
	//		SetShaderOutputs( cmd_dispatch_compute_shader, *compute_shader );

	//		cmd_dispatch_compute_shader.program = compute_shader->programs[0];
	//		cmd_dispatch_compute_shader.groupsX = CalculateNumTilesX(sceneView.viewportWidth);
	//		cmd_dispatch_compute_shader.groupsY = CalculateNumTilesY(sceneView.viewportHeight);
	//		cmd_dispatch_compute_shader.groupsZ = 1;
	//	}
	//	mxDO(NGpu::Recording::put( render_context._commands, cmd_dispatch_compute_shader ));
	//}

	//render_context.submit(
	//	first_command_offset,
	//	NGpu::buildSortKey( view_id, NGpu::SortKey(0) )
	//	);

	return ALL_OK;
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
