#include <Base/Base.h>
#pragma hdrstop

//// for std::sort()
//#include <algorithm>
//
//// ozz::memory::SetDefaulAllocator()
//#include <ozz/base/memory/allocator.h>
//
//#include <Base/Template/Containers/Array/TInplaceArray.h>
//
//#include <Core/Client.h>
//#include <Core/ConsoleVariable.h>
//#include <Core/Util/Tweakable.h>
//
//#include <Engine/WindowsDriver.h>
//
#include <GPU/Public/graphics_system.h>
//
//#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Scene/MeshInstance.h>
//#include <Rendering/Public/Core/Material.h>
//#include <Rendering/Public/Core/Material.h>
//#include <Rendering/Public/Globals.h>
//#include <Rendering/Public/Settings.h>
//#include <Rendering/Public/Core/VertexFormats.h>
//#include <Rendering/Private/ShaderInterop.h>
//#include <Rendering/Public/Core/RenderPipeline.h>
//#include <Rendering/Private/RenderSystemData.h>
//#include <Rendering/Private/SceneRendering.h>
////#include <Renderer/private/shadows/shadow_map_renderer.h>
#include <Rendering/Private/Modules/Particles/Particle_Rendering.h>
//#include <Rendering/Private/Modules/idTech4/idRenderModel.h>
//
//mxREFACTOR("shouldn't be here:")//for DeferredLight
//#include <Rendering/Private/Pipelines/TiledDeferred/TiledCommon.h>
#include <Rendering/Private/Shaders/Shared/nw_shared_globals.h>
#include <Rendering/Private/RenderSystemData.h>


namespace Rendering
{
namespace Globals
{



namespace
{

static UINT32 CalculateRenderTargetDimension( INT32 screenDimension, const TbTextureSize& textureSize )
{
	float sizeValue = textureSize.size;
	if( textureSize.relative ) {
		sizeValue *= screenDimension;
	}
	return (UINT32) sizeValue;
}

static UINT32 CalculateRenderTargetArea( UINT16 screenWidth, UINT16 screenHeight, const TbTextureBase& renderTarget )
{
	UINT32 sizeX = CalculateRenderTargetDimension( screenWidth, renderTarget.sizeX );
	UINT32 sizeY = CalculateRenderTargetDimension( screenHeight, renderTarget.sizeY );
	return sizeX * sizeY;
}

enum ResType
{
	RES_DEPTH_TARGET,	// TbDepthTarget
	RES_COLOR_TARGET,	// TbColorTarget
	RES_TEXTURE_2D,		// TbTexture2D
};

struct SortItem
{
	TbTextureBase *	o;	// TbColorTarget, TbDepthTarget or TbTexture2D
	UINT32			size;
	ResType			type;
};

ERet CreateRenderTargets(
	const ClumpList & clump,
	UINT16 screenWidth, UINT16 screenHeight,		
	bool createOnlyThoseDependentOnBackBuffer )
{
	// create largest render targets first
	TInplaceArray< SortItem, 64 >	sortedItems;

	{
		ClumpList::Iterator< TbColorTarget >	colorTargetIt( clump );
		while( colorTargetIt.IsValid() )
		{
			TbColorTarget& colorTarget = colorTargetIt.Value();
			if( createOnlyThoseDependentOnBackBuffer && !colorTarget.DependsOnBackBufferSize() ) {
				colorTargetIt.MoveToNext();
				continue;
			}
			SortItem& sortItem = sortedItems.AddNew();
			sortItem.o = &colorTarget;
			sortItem.size = CalculateRenderTargetArea( screenWidth, screenHeight, colorTarget ) * NwDataFormat::BitsPerPixel(colorTarget.format);
			sortItem.type = RES_COLOR_TARGET;
			colorTargetIt.MoveToNext();
		}
	}
	{
		ClumpList::Iterator< TbDepthTarget >	depthTargetIt( clump );
		while( depthTargetIt.IsValid() )
		{
			TbDepthTarget& depth_target = depthTargetIt.Value();
			if( createOnlyThoseDependentOnBackBuffer && !depth_target.DependsOnBackBufferSize() ) {
				depthTargetIt.MoveToNext();
				continue;
			}
			SortItem& sortItem = sortedItems.AddNew();
			sortItem.o = &depth_target;
			sortItem.size = CalculateRenderTargetArea( screenWidth, screenHeight, depth_target ) * NwDataFormat::BitsPerPixel(depth_target.format);
			sortItem.type = RES_DEPTH_TARGET;
			depthTargetIt.MoveToNext();
		}
	}
	{
		ClumpList::Iterator< TbTexture2D >	textureIt( clump );
		while( textureIt.IsValid() )
		{
			TbTexture2D& texture = textureIt.Value();
			if( createOnlyThoseDependentOnBackBuffer && !texture.DependsOnBackBufferSize() ) {
				textureIt.MoveToNext();
				continue;
			}
			SortItem& sortItem = sortedItems.AddNew();
			sortItem.o = &texture;
			sortItem.size = CalculateRenderTargetArea( screenWidth, screenHeight, texture ) * NwDataFormat::BitsPerPixel(texture.format);
			sortItem.type = RES_TEXTURE_2D;
			textureIt.MoveToNext();
		}
	}

	struct CompareRenderTargetsByArea {
		inline bool operator () ( const SortItem& a, const SortItem& b ) const { return a.size > b.size; }
	};
	std::stable_sort( sortedItems.raw(), sortedItems.raw() + sortedItems.num(), CompareRenderTargetsByArea() );

	NwColorTargetDescription	colorTargetDescription;
	NwDepthTargetDescription	depthTargetDescription;
	NwTexture2DDescription	texture2DDescription;
	for( UINT32 iRenderTarget = 0; iRenderTarget < sortedItems.num(); iRenderTarget++ )
	{
		SortItem& item = sortedItems[ iRenderTarget ];
		if( item.type == RES_COLOR_TARGET )
		{
			TbColorTarget& colorTarget = *static_cast< TbColorTarget* >( item.o );
			mxASSERT(colorTarget.handle.IsNil());

			colorTargetDescription.format = colorTarget.format;
			colorTargetDescription.width = CalculateRenderTargetDimension( screenWidth, colorTarget.sizeX );
			colorTargetDescription.height = CalculateRenderTargetDimension( screenHeight, colorTarget.sizeY );
			colorTargetDescription.numMips = colorTarget.numMips;
			colorTargetDescription.flags = colorTarget.flags;

			colorTarget.handle = NGpu::CreateColorTarget(
				colorTargetDescription
				, colorTarget.name.c_str()
				);
		}
		else if( item.type == RES_DEPTH_TARGET )
		{
			TbDepthTarget& depth_target = *static_cast< TbDepthTarget* >( item.o );
			mxASSERT(depth_target.handle.IsNil());

			depthTargetDescription.format = depth_target.format;
			depthTargetDescription.width = CalculateRenderTargetDimension( screenWidth, depth_target.sizeX );
			depthTargetDescription.height = CalculateRenderTargetDimension( screenHeight, depth_target.sizeY );
			depthTargetDescription.sample = depth_target.sample;

			depth_target.handle = NGpu::CreateDepthTarget(
				depthTargetDescription
				, depth_target.name.c_str()
				);
		}
		else
		{
			mxASSERT(item.type == RES_TEXTURE_2D);
			TbTexture2D& texture2D = *static_cast< TbTexture2D* >( item.o );
			mxASSERT(texture2D.handle.IsNil());

			texture2DDescription.format = texture2D.format;
			texture2DDescription.width = CalculateRenderTargetDimension( screenWidth, texture2D.sizeX );
			texture2DDescription.height = CalculateRenderTargetDimension( screenHeight, texture2D.sizeY );
			texture2DDescription.numMips = texture2D.numMips;
			texture2DDescription.flags = texture2D.flags;

			texture2D.handle = NGpu::createTexture2D(
				texture2DDescription
				, nil
				, texture2D.name.c_str()
				);
		}
	}
	return ALL_OK;
}


/// this gets called after the main viewport has been reallocated
ERet RecreateResourcesDependentOnBackBuffer( const ClumpList& _clump, UINT16 screenWidth, UINT16 screenHeight )
{
	mxDO(CreateRenderTargets(_clump, screenWidth, screenHeight, true));
	return ALL_OK;
}

ERet CreateRenderResources( const ClumpList& _clump )
{
	// Ideally, render states should be sorted by state deltas (so that changes between adjacent states are minimized).

	UINT	numSamplerStates = 0;
	UINT	numDepthStencilStates = 0;
	UINT	numRasterizerStates = 0;
	UINT	numBlendStates = 0;

	// for resolving stateblocks
	AllocatorI &	scratch = MemoryHeaps::temporary();
	DynamicArray< FxSamplerState* >			samplerStates( scratch );
	DynamicArray< FxDepthStencilState* >	depthStencilStates( scratch );
	DynamicArray< FxRasterizerState* >		rasterizerStates( scratch );
	DynamicArray< FxBlendState* >			blendStates( scratch );

	{
		ClumpList::Iterator< FxSamplerState >	samplerStateIt( _clump );
		while( samplerStateIt.IsValid() )
		{
			FxSamplerState& samplerState = samplerStateIt.Value();
			mxASSERT(samplerState.handle.IsNil());
			samplerState.handle = NGpu::CreateSamplerState( samplerState );
			samplerStates.add( &samplerState );
			samplerStateIt.MoveToNext();
			++numSamplerStates;
		}
	}
	{
		ClumpList::Iterator< FxDepthStencilState >	depthStencilStateIt( _clump );
		while( depthStencilStateIt.IsValid() )
		{
			FxDepthStencilState& depthStencilState = depthStencilStateIt.Value();
			mxASSERT(depthStencilState.handle.IsNil());
			depthStencilState.handle = NGpu::CreateDepthStencilState( depthStencilState );
			depthStencilStateIt.MoveToNext();
			numDepthStencilStates++;
		}
	}
	{
		ClumpList::Iterator< FxRasterizerState >	rasterizerStateIt( _clump );
		while( rasterizerStateIt.IsValid() )
		{
			FxRasterizerState& rasterizerState = rasterizerStateIt.Value();
			mxASSERT(rasterizerState.handle.IsNil());
			rasterizerState.handle = NGpu::CreateRasterizerState( rasterizerState );
			rasterizerStateIt.MoveToNext();
			numRasterizerStates++;
		}
	}
	{
		ClumpList::Iterator< FxBlendState >	blendStateIt( _clump );
		while( blendStateIt.IsValid() )
		{
			FxBlendState& blendState = blendStateIt.Value();
			mxASSERT(blendState.handle.IsNil());
			blendState.handle = NGpu::CreateBlendState( blendState );
			blendStateIt.MoveToNext();
			numBlendStates++;
		}
	}

	return ALL_OK;
}


#if 0
ERet CreateRenderResources( const ClumpList& _clump )
{
	// Ideally, render states should be sorted by state deltas (so that changes between adjacent states are minimized).

	UINT	numSamplerStates = 0;
	UINT	numDepthStencilStates = 0;
	UINT	numRasterizerStates = 0;
	UINT	numBlendStates = 0;

	// for resolving stateblocks
	AllocatorI &	scratch = Memory::Scratch();
	DynamicArray< FxSamplerState* >			samplerStates( scratch );
	DynamicArray< FxDepthStencilState* >	depthStencilStates( scratch );
	DynamicArray< FxRasterizerState* >		rasterizerStates( scratch );
	DynamicArray< FxBlendState* >			blendStates( scratch );

	{
		ClumpList::Iterator< FxSamplerState >	samplerStateIt( _clump );
		while( samplerStateIt.IsValid() )
		{
			FxSamplerState& samplerState = samplerStateIt.Value();
			mxASSERT(samplerState.handle.IsNil());
			samplerState.handle = GL::CreateSamplerState( samplerState );
			samplerStates.add( &samplerState );
			samplerStateIt.MoveToNext();
			++numSamplerStates;
		}
	}
	{
		ClumpList::Iterator< FxDepthStencilState >	depthStencilStateIt( _clump );
		while( depthStencilStateIt.IsValid() )
		{
			FxDepthStencilState& depthStencilState = depthStencilStateIt.Value();
			mxASSERT(depthStencilState.handle.IsNil());
			depthStencilState.handle = GL::CreateDepthStencilState( depthStencilState );
			depthStencilStateIt.MoveToNext();
			numDepthStencilStates++;
		}
	}
	{
		ClumpList::Iterator< FxRasterizerState >	rasterizerStateIt( _clump );
		while( rasterizerStateIt.IsValid() )
		{
			FxRasterizerState& rasterizerState = rasterizerStateIt.Value();
			mxASSERT(rasterizerState.handle.IsNil());
			rasterizerState.handle = GL::CreateRasterizerState( rasterizerState );
			rasterizerStateIt.MoveToNext();
			numRasterizerStates++;
		}
	}
	{
		ClumpList::Iterator< FxBlendState >	blendStateIt( _clump );
		while( blendStateIt.IsValid() )
		{
			FxBlendState& blendState = blendStateIt.Value();
			mxASSERT(blendState.handle.IsNil());
			blendState.handle = GL::CreateBlendState( blendState );
			blendStateIt.MoveToNext();
			numBlendStates++;
		}
	}

	{
		ClumpList::Iterator< TbRenderResourcesLUT >	renderResourcesIt( _clump );
		while( renderResourcesIt.IsValid() )
		{
			TbRenderResourcesLUT& renderResources = renderResourcesIt.Value();
			for( UINT32 iStateBlock = 0; iStateBlock < renderResources.state_blocks.num(); iStateBlock++ )
			{
				NwRenderState* stateBlock = renderResources.state_blocks[ iStateBlock ];
				//stateBlock->blendState		= renderResources.blend_states[ stateBlock->blendState.id ]->handle;
				//stateBlock->rasterizerState	= renderResources.rasterizer_states[ stateBlock->rasterizerState.id ]->handle;
				//stateBlock->depthStencilState= renderResources.depth_stencil_states[ stateBlock->depthStencilState.id ]->handle;

				//stateBlock->blendStateHandle		= renderResources.blend_states[ stateBlock->blendStateHandle.id ]->handle;
				//stateBlock->rasterizerStateHandle	= renderResources.rasterizer_states[ stateBlock->rasterizerStateHandle.id ]->handle;
				//stateBlock->depthStencilStateHandle= renderResources.depth_stencil_states[ stateBlock->depthStencilStateHandle.id ]->handle;
			}
			renderResourcesIt.MoveToNext();
		}
	}

	return ALL_OK;
}
#endif

static ERet renderer_ResizeBuffers( UINT16 width, UINT16 height, bool fullscreen )
{
//	mxDO(ReleaseResourcesDependentOnBackBuffer( ClumpList::g_loadedClumps ));
UNDONE;
//	//mxDO(NGpu::SetVideoMode( width, height ));
//	mxDO(NGpu::NextFrame());
//
//	mxDO(RecreateResourcesDependentOnBackBuffer( ClumpList::g_loadedClumps, width, height ));

	return ALL_OK;
}





static void _createVertexFormat(
								TbVertexFormat* vertex_format
								, NwVertexDescription * vertex_description
								)
{
	DBGOUT("Creating '%s'...", vertex_format->GetTypeName());
	mxASSERT(vertex_format->input_layout_handle.IsNil());

	//
	(*vertex_format->build_vertex_description_fun)( vertex_description );

	//
	vertex_format->input_layout_handle = NGpu::createInputLayout(
		*vertex_description
		, vertex_format->m_name
		);
}

void _createVertexFormats()
{
	NwVertexDescription	vertex_description;

	//
	TbVertexFormat* current_vertex_format = TbVertexFormat::s_all;
	while( current_vertex_format )
	{
		TbVertexFormat* next_vertex_format = current_vertex_format->_next;

		//
		_createVertexFormat(
			current_vertex_format
			, &vertex_description
			);

		//
		current_vertex_format = next_vertex_format;
	}
}


void SetDefaultShaderParameters(RenderSystemData& data, NGpu::ShaderInputs &_viewInputs )
{
	_viewInputs.clear();
	_viewInputs.CBs[G_PerFrame_Index] = data._global_uniforms.per_frame.handle;
	_viewInputs.CBs[G_PerCamera_Index] = data._global_uniforms.per_camera.handle;
	_viewInputs.CBs[G_PerObject_Index] = data._global_uniforms.per_model.handle;
	_viewInputs.CBs[G_Lighting_Index] = data._global_uniforms.lighting.handle;
	_viewInputs.CBs[G_SkinningData_Index] = data._global_uniforms.skinning_data.handle;
	_viewInputs.CBs[G_VoxelTerrain_Index] = data._global_uniforms.voxel_terrain.handle;
}

ERet _SetDefaultSceneLayerParameters(
	RenderSystemData& data
	)
{
	NwViewport	viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = data._screen_width;
	viewport.height = data._screen_height;

	NGpu::ShaderInputs	view_inputs;
	SetDefaultShaderParameters(data, view_inputs);

	//
	const RenderPath& render_path = GetRenderPath();

	for( UINT iPass = 0; iPass < render_path.sortedPassNameHashes.num(); iPass++ )
	{
		const ScenePassInfo& scenePassInfo = render_path.passInfo[ iPass ];
		const ScenePassData& passData = render_path.passData[ scenePassInfo.passDataIndex ];

		NGpu::ViewState	viewState;
		viewState.clear();

		mxDO(ViewState_From_ScenePass( viewState, passData, viewport ));

		for( UINT iSubPass = 0; iSubPass < scenePassInfo.num_sublayers; iSubPass++ )
		{
			const NGpu::LayerID globalDrawingOrder = scenePassInfo.draw_order + iSubPass;
			//
			//if( passId == ScenePasses::BlendedOIT && settings.whitepaper_quality )
			//{
			//	// clear BlendedOIT_Accumulation to white
			//	viewState.clear_colors[0][0] = 1.0f;
			//	viewState.clear_colors[0][1] = 1.0f;
			//	viewState.clear_colors[0][2] = 1.0f;
			//	viewState.clear_colors[0][3] = 1.0f;
			//}

			NGpu::SetViewParameters( globalDrawingOrder, viewState );
			NGpu::SetViewShaderInputs( globalDrawingOrder, view_inputs );

			IF_DEVELOPER NGpu::SetViewDebugName( globalDrawingOrder, passData.profiling_scope.c_str() );
		}//For each sub-pass.

	}//For each scene pass.



	////
	//{
	//	const ScenePassInfo debugWireframePassInfo = GetRenderPath().getPassInfo( ScenePasses::FillGBuffer_Pre );
	//	ScenePassData & scenePassData = getRenderPath_nonConst().passData[ debugWireframePassInfo.passDataIndex ];

	//	_SetupLayerForRenderingFirstPersonWeapons(
	//		ScenePasses::FillGBuffer_Pre
	//		, render_path
	//		);
	//}


		//
#if nwRENDERER_ENABLE_SHADOWS
		_shadow_map_renderer.setupViewParameters( render_path );
#endif

	return ALL_OK;
}

//NOTE: this must be called after all clumps with render resources have been loaded
ERet CreateGraphicsResources(
							 RenderSystemData& data
							 , const ClumpList& _clumps
							 , U32 _screenWidth
							 , U32 _screenHeight
							 )
{
	// Initialization order:
	// 0) Depth-stencil surfaces;	// 1) Render targets;
	// 2) State objects;
	// 3) Shaders;
	// 4) Input layouts;
	// 5) Read-only random access resources: frequently used textures;
	// 6) Read-only streams and less used resources: vertex buffers, index buffers, small textures;
	// 7) Everything else.

	// Larger, higher AA and FP format resources first:
	// render targets should be sorted by size,
	// shader resources should be sorted by size,
	// constant buffers should be sorted by size.

	// deferred initialization for render targets:
	//mxDO(CreateRenderTargets(screen, _clumps, false));


	mxDO(CreateRenderTargets(_clumps, _screenWidth, _screenHeight, false));

	mxDO(data._renderTargetPool.Initialize());

	mxDO(CreateRenderResources(_clumps));

	// "Don’t Throw it all Away: Efficient Buffer Management" (GDC 2012):
	// -------------------------------------------------------------
	// Type       | Usage (e.g)  | Create Flag | Update Method
	// -------------------------------------------------------------
	// “Forever”    Level BSPs      IMMUTABLE    Cannot Update 
	// Long-Lived   Characters      DEFAULT      UpdateSubResource 
	// Transient    UI/Text         DYNAMIC      CTransientBuffer 
	// Temporary    Particles       DYNAMIC      Map/NO_OVERWRITE 
	// Constant     Material Props  DYNAMIC      Map/DISCARD 

	// Create global constant buffers.
	{
		data._global_uniforms.per_frame.handle = nwCREATE_GLOBAL_CONSTANT_BUFFER(G_PerFrame);
		data._global_uniforms.per_frame.slot = G_PerFrame_Index;

		//
		data._global_uniforms.per_camera.handle = nwCREATE_GLOBAL_CONSTANT_BUFFER(G_PerCamera);
		data._global_uniforms.per_camera.slot = G_PerCamera_Index;

		//
		data._global_uniforms.per_model.handle = nwCREATE_GLOBAL_CONSTANT_BUFFER(G_PerObject);
		data._global_uniforms.per_model.slot = G_PerObject_Index;

		//
		data._global_uniforms.lighting.handle = nwCREATE_GLOBAL_CONSTANT_BUFFER(G_Lighting);
		data._global_uniforms.lighting.slot = G_Lighting_Index;

		//
		data._global_uniforms.skinning_data.handle = nwCREATE_GLOBAL_CONSTANT_BUFFER(G_SkinningData);
		data._global_uniforms.skinning_data.slot = G_SkinningData_Index;

		//
		data._global_uniforms.voxel_terrain.handle = nwCREATE_GLOBAL_CONSTANT_BUFFER(G_VoxelTerrain);
		data._global_uniforms.voxel_terrain.slot = G_VoxelTerrain_Index;
	}

	////
	//{
	//	buffer_description.size = LLGL_MAX_UNIFORM_BUFFER_SIZE;

	//	data._cb_instance_matrices = NGpu::CreateBuffer(
	//		buffer_description
	//		, nil
	//		IF_DEVELOPER , "InstanceTransforms"
	//		);
	//}


	// Initialize look-up tables
	_createVertexFormats();

	//
	_SetDefaultSceneLayerParameters(
		data
		);

	return ALL_OK;
}




ERet initializeResourceLoaders( AllocatorI & allocator )
{
	ProxyAllocator & resource_allocator = MemoryHeaps::resources();

	NwMesh::metaClass().loader = mxNEW( allocator, TbMeshLoader, resource_allocator );
	Material::metaClass().loader = mxNEW( allocator, MaterialLoader, resource_allocator );

	//
	NwAnimClip::metaClass().loader = mxNEW(
		allocator
		, NwAnimClipLoader
		, MemoryHeaps::animation()
		);

	NwSkinnedMesh::metaClass().loader = mxNEW(
		allocator
		, NwSkinnedMeshLoader
		, MemoryHeaps::animation()
		);


	return ALL_OK;
}

}//namespace






ERet Initialize(
				U32 screen_width, U32 screen_height
				, const RrGlobalSettings& settings
				, NwClump* object_storage_clump
				)
{
	AllocatorI& allocator = GetAllocator();

	mxDO(nwCREATE_OBJECT(g_data._ptr
		, allocator
		, RenderSystemData
		//, allocator
		));

	//
	RenderSystemData& data = *g_data;


	data._object_storage_clump = object_storage_clump;

	data._screen_width	= screen_width;
	data._screen_height	= screen_height;

	mxDO(data._deferred_renderer.Initialize( screen_width, screen_height ));

	mxDO(CreateGraphicsResources(
		data
		, ClumpList::g_loadedClumps
		, screen_width
		, screen_height
		));

	// NOTE: render target handles may not be valid yet, as they are initialized lazily


//	_atmosphere_renderer = Atmosphere_Rendering::NwAtmosphereModuleI::create(_storage);

#if nwRENDERER_ENABLE_SHADOWS
	mxDO(_shadow_map_renderer.Initialize( settings.shadows ));
#endif

#if nwRENDERER_CFG_WITH_VOXEL_GI
	_gpu_voxels.Initialize( settings.vxgi );
#endif

#if nwRENDERER_ENABLE_LIGHT_PROBES
	_lightProbeGrid.Initialize( settings.diffuse_GI );
	_test_need_to_rebuild_sky_irradiance_map = false;
	_test_need_to_relight_light_probes = false;
#endif

	mxDO(data._geometry_buffers_pool.Initialize( object_storage_clump ));

	mxDO(data._debug_renderer.Initialize( AuxVertex_vertex_format.input_layout_handle ));
	//
	mxDO(data._debug_renderer.reserveSpace(
		mxMiB(1)	// num_vertices
		, mxMiB(1)	// num_indices
		, mxKiB(64)	// num_batches
		));

	data._debug_line_renderer.ReserveSpace(512,512,512);

#if nwRENDERER_CFG_ENABLE_idTech4
	mxTRYnWARN(_id_material_system.Initialize());
	idRenderModel_::RegisterRenderCallback(data._render_callbacks);
#endif

#if nwRENDERER_CFG_WITH_SKELETAL_ANIMATION
	mxDO(_skeletal_animation_system.SetUp());
#endif

	//
	initializeResourceLoaders(allocator);

	//
	data._render_callbacks.code[ RE_MeshInstance ] = &renderMeshInstances;
	data._render_callbacks.data[ RE_MeshInstance ] = nil;

	//
	data._render_callbacks.code[ RE_Particles ] = &TbParticleSystem::renderParticles;
	data._render_callbacks.data[ RE_Particles ] = nil;

	return ALL_OK;
}




}//namespace Globals
}//namespace Rendering
