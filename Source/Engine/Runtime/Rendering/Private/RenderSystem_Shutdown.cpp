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

#include <Rendering/Private/RenderSystemData.h>


namespace Rendering
{
namespace Globals
{


namespace
{



ERet ReleaseRenderTargets( const ClumpList& _clump )
{
	{
		ClumpList::Iterator< TbTexture2D >	textureIt( _clump );
		while( textureIt.IsValid() )
		{
			TbTexture2D& texture = textureIt.Value();
			if( texture.handle.IsValid() )
			{
				NGpu::DeleteTexture( texture.handle );
				texture.handle.SetNil();
			}
			textureIt.MoveToNext();
		}
	}
	{
		ClumpList::Iterator< TbDepthTarget >	depthTargetIt( _clump );
		while( depthTargetIt.IsValid() )
		{
			TbDepthTarget& depth_target = depthTargetIt.Value();
			if( depth_target.handle.IsValid() )
			{
				NGpu::DeleteDepthTarget( depth_target.handle );
				depth_target.handle.SetNil();
			}
			depthTargetIt.MoveToNext();
		}
	}
	{
		ClumpList::Iterator< TbColorTarget >	colorTargetIt( _clump );
		while( colorTargetIt.IsValid() )
		{
			TbColorTarget& colorTarget = colorTargetIt.Value();
			if( colorTarget.handle.IsValid() )
			{
				NGpu::DeleteColorTarget( colorTarget.handle );
				colorTarget.handle.SetNil();
			}				
			colorTargetIt.MoveToNext();
		}
	}
	return ALL_OK;
}

/// this gets called before the main viewport has been deallocated for resizing
ERet ReleaseResourcesDependentOnBackBuffer( const ClumpList& _clump )
{
	{
		ClumpList::Iterator< TbColorTarget >	colorTargetIt( _clump );
		while( colorTargetIt.IsValid() )
		{
			TbColorTarget& colorTarget = colorTargetIt.Value();
			if( colorTarget.DependsOnBackBufferSize() && colorTarget.handle.IsValid() ) {
				NGpu::DeleteColorTarget( colorTarget.handle );
				colorTarget.handle.SetNil();
			}
			colorTargetIt.MoveToNext();
		}
	}
	{
		ClumpList::Iterator< TbDepthTarget >	depthTargetIt( _clump );
		while( depthTargetIt.IsValid() )
		{
			TbDepthTarget& depth_target = depthTargetIt.Value();
			if( depth_target.DependsOnBackBufferSize() && depth_target.handle.IsValid() ) {
				NGpu::DeleteDepthTarget( depth_target.handle );
				depth_target.handle.SetNil();
			}
			depthTargetIt.MoveToNext();
		}
	}
	return ALL_OK;
}


void ReleaseRenderResources( const ClumpList& _clump )
{
#if 0
	for( UINT32 iProgram = 0; iProgram < m_programs.num(); iProgram++ )
	{
		FxProgram& program = m_programs[ iProgram ];
		GL::DeleteProgram(program.handle);
		program.handle.SetNil();
	}

	for( UINT32 shaderType = 0; shaderType < mxCOUNT_OF(m_shaders); shaderType++ )
	{
		for( UINT32 iShader = 0; iShader < m_shaders[shaderType].num(); iShader++ )
		{
			GL::DeleteShader( m_shaders[shaderType][iShader] );
			m_shaders[shaderType][iShader].SetNil();
		}
	}

	for( UINT32 iTechnique = 0; iTechnique < m_techniques.num(); iTechnique++ )
	{
		TbShader& technique = m_techniques[ iTechnique ];
		for( UINT32 iCB = 0; iCB < technique.locals.num(); iCB++ )
		{
			TbCBuffer& rCB = technique.locals[ iCB ];
			GL::DeleteBuffer( rCB.handle );
			rCB.handle.SetNil();
		}
	}

	for( UINT32 iCB = 0; iCB < m_globalCBuffers.num(); iCB++ )
	{
		TbCBuffer& rCB = m_globalCBuffers[ iCB ];
		GL::DeleteBuffer( rCB.handle );
		rCB.handle.SetNil();
	}
	//for( UINT32 iSR = 0; iSR < shaderResources.num(); iSR++ )
	//{
	//	//XShaderResource& rSR = shaderResources[ iSR ];
	//	UNDONE;
	//}
#endif
#if 0
	for( UINT32 iStateBlock = 0; iStateBlock < m_stateBlocks.num(); iStateBlock++ )
	{
		NwRenderState& stateBlock = m_stateBlocks[ iStateBlock ];
		stateBlock.rasterizerState.SetNil();
		stateBlock.depthStencilState.SetNil();
		stateBlock.blendState.SetNil();
	}
	m_stateBlocks.destroyAndEmpty();
#endif

	{
		ClumpList::Iterator< FxBlendState >	blendStateIt( _clump );
		while( blendStateIt.IsValid() )
		{
			FxBlendState& blendState = blendStateIt.Value();
			NGpu::DeleteBlendState( blendState.handle );
			blendState.handle.SetNil();
			blendStateIt.MoveToNext();
		}
	}
	{
		ClumpList::Iterator< FxRasterizerState >	rasterizerStateIt( _clump );
		while( rasterizerStateIt.IsValid() )
		{
			FxRasterizerState& rasterizerState = rasterizerStateIt.Value();
			NGpu::DeleteRasterizerState( rasterizerState.handle );
			rasterizerState.handle.SetNil();
			rasterizerStateIt.MoveToNext();
		}
	}
	{
		ClumpList::Iterator< FxDepthStencilState >	depthStencilStateIt( _clump );
		while( depthStencilStateIt.IsValid() )
		{
			FxDepthStencilState& depthStencilState = depthStencilStateIt.Value();
			NGpu::DeleteDepthStencilState( depthStencilState.handle );
			depthStencilState.handle.SetNil();
			depthStencilStateIt.MoveToNext();
		}
	}
	{
		ClumpList::Iterator< FxSamplerState >	samplerStateIt( _clump );
		while( samplerStateIt.IsValid() )
		{
			FxSamplerState& samplerState = samplerStateIt.Value();
			NGpu::DeleteSamplerState( samplerState.handle );
			samplerState.handle.SetNil();
			samplerStateIt.MoveToNext();
		}
	}
}


static void _destroyVertexFormat( TbVertexFormat *vertex_format )
{
	mxASSERT( vertex_format->input_layout_handle.IsValid() );

	NGpu::destroyInputLayout( vertex_format->input_layout_handle );

	vertex_format->input_layout_handle.SetNil();
}

void _destroyVertexFormats()
{
	//
	TbVertexFormat* current_vertex_format = TbVertexFormat::s_all;
	while( current_vertex_format )
	{
		TbVertexFormat* next_vertex_format = current_vertex_format->_next;

		//
		_destroyVertexFormat(
			current_vertex_format
			);

		//
		current_vertex_format = next_vertex_format;
	}
}

void DestroyGraphicsResources( RenderSystemData& data, const ClumpList& _clumps )
{
	data._renderTargetPool.Shutdown();
	ReleaseRenderTargets(_clumps);
	ReleaseRenderResources(_clumps);
	
	NGraphics::DestroyGlobalConstantBuffer(data._global_uniforms.per_frame.handle);
	NGraphics::DestroyGlobalConstantBuffer(data._global_uniforms.per_camera.handle);
	NGraphics::DestroyGlobalConstantBuffer(data._global_uniforms.per_model.handle);
	NGraphics::DestroyGlobalConstantBuffer(data._global_uniforms.lighting.handle);
	NGraphics::DestroyGlobalConstantBuffer(data._global_uniforms.skinning_data.handle);
	NGraphics::DestroyGlobalConstantBuffer(data._global_uniforms.voxel_terrain.handle);

	//NGraphics::DestroyGlobalConstantBuffer(data._cb_instance_matrices);		data._cb_instance_matrices.SetNil();

	_destroyVertexFormats();
}

void destroy_RenderObjects_in_Clump_in_CorrectOrder( NwClump & clump )
{
	clump.destroyAllObjectsOfType( MeshInstance::metaClass() );
	clump.destroyAllObjectsOfType( NwMesh::metaClass() );
	clump.destroyAllObjectsOfType( RrTransform::metaClass() );

	// low-level buffers will be destroyed later in NwGeometryBufferPool::Shutdown()
	//clump.destroyAllObjectsOfType( NwGeometryBuffer::metaClass() );

	clump.destroyAllObjectsOfType( Material::metaClass() );
}

void shutdownResourceLoaders( AllocatorI & allocator )
{
	mxDELETE( NwSkinnedMesh::metaClass().loader, allocator );
	mxDELETE( NwAnimClip::metaClass().loader, allocator );
	
	mxDELETE( Material::metaClass().loader, allocator );
	mxDELETE( NwMesh::metaClass().loader, allocator );
}

}//namespace


void Shutdown()
{
	AllocatorI& allocator = GetAllocator();

	shutdownResourceLoaders(allocator);

	RenderSystemData& data = *g_data;

#if nwRENDERER_CFG_WITH_SKELETAL_ANIMATION
	_skeletal_animation_system.ShutDown();
#endif

#if nwRENDERER_CFG_ENABLE_idTech4
	_id_material_system.Shutdown();
#endif

#if nwRENDERER_ENABLE_SHADOWS
	_shadow_map_renderer.Shutdown();
#endif

	//Atmosphere_Rendering::NwAtmosphereModuleI::destroy(_atmosphere_renderer);
	//_atmosphere_renderer = nil;

	data._deferred_renderer.Shutdown();

#if nwRENDERER_ENABLE_LIGHT_PROBES
	_lightProbeGrid.Shutdown();
#endif

#if nwRENDERER_CFG_WITH_VOXEL_GI
	_gpu_voxels.Shutdown();
#endif

	//_shadow_map_renderer.Shutdown();
	//m_shadowRenderer.Shutdown();

	data._debug_renderer.Shutdown();

	destroy_RenderObjects_in_Clump_in_CorrectOrder( *data._object_storage_clump );

	data._geometry_buffers_pool.Shutdown();

	DestroyGraphicsResources(data, ClumpList::g_loadedClumps);

	data._object_storage_clump = nil;

	nwDESTROY_OBJECT(g_data._ptr, allocator);
}

}//namespace Globals
}//namespace Rendering
