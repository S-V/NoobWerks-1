#pragma once

//#include <Core/Editor.h>
#include <Core/ObjectModel/Clump.h>

#include <GPU/Public/graphics_types.h>
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <Graphics/Public/Utility/RenderTargetPool.h>
//
#include <Rendering/ForwardDecls.h>
#include <Rendering/Public/Core/VertexFormats.h>
#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/Material.h>	// idMaterialSystem
#include <Rendering/Private/Pipelines/TiledDeferred/TiledDeferred.h>
//#include <Renderer/private/shadows/shadow_map_renderer.h>

#include <Rendering/Private/Modules/Atmosphere/Atmosphere.h>
#include <Rendering/Private/Modules/VoxelGI/IrradianceField.h>
#include <Rendering/Private/Modules/idTech4/idMaterial.h>
#include <Rendering/Private/Modules/Animation/_AnimationSystem.h>


namespace Rendering
{


struct GlobalUniformBufferBinding
{
	HBuffer	handle;	// constant buffer handle
	U16		slot;	// constant buffer slot
};

struct GlobalUniformBufferBindings
{
	GlobalUniformBufferBinding	per_frame;
	GlobalUniformBufferBinding	per_camera;
	GlobalUniformBufferBinding	per_model;
	GlobalUniformBufferBinding	lighting;	//!< global lighting parameters (e.g. sun/sky light)
	GlobalUniformBufferBinding	skinning_data;
	GlobalUniformBufferBinding	voxel_terrain;	//!< materials for rendering a voxel terrain
};



/// contains common variables (i.e. shared by all rendering pipelines)
struct RenderSystemData
{
	TbPrimitiveBatcher	_debug_renderer;	//!< for debug lines, etc.
	TbDebugLineRenderer	_debug_line_renderer;

	//
	RenderCallbacks			_render_callbacks;
	//RenderCallbacks			_shadow_render_callbacks;
	//VoxelizationCallbacks	_voxelization_callbacks;

	//
	//GlobalShadowConfig	_global_shadow;

	// global constant buffers
	GlobalUniformBufferBindings	_global_uniforms;

	//NGraphics::ShaderResourceLUT	global_resources;

	// Events
	//TCallback< void ( U32 /* shadowMapAtlasDim */ ) >	onShadowAtlasResized;




	//
	NwRenderTargetPool	_renderTargetPool;

	//
	U32	_screen_width;
	U32	_screen_height;

	// dynamic geometry
	NwGeometryBufferPool	_geometry_buffers_pool;


	DeferredRenderer	_deferred_renderer;

	// Game-specific:

	/////
	//mxREFACTOR("move to the space game");
	//HBuffer		_cb_instance_matrices;

	//

	//TEMP:DISABLED
	//TPtr< Atmosphere_Rendering::NwAtmosphereModuleI >	_atmosphere_renderer;

#if nwRENDERER_ENABLE_SHADOWS
	RrShadowMapRenderer	_shadow_map_renderer;

	// These matrices are updated only when cascades move:
	// they are used for shadow cascade selection
	V3f			cascadeScales[MAX_SHADOW_CASCADES];
	V3f			cascadeOffsets[MAX_SHADOW_CASCADES];
	M44f		cascadeViewProjectionZBiasMatrices[MAX_SHADOW_CASCADES];

	// These matrices must be updated whenever the main camera rotates (usually, each frame):
	M44f		cameraToLightViewSpace;
	// converts pixel positions from view space into shadow texture space; these are used when rendering the shadowed scene
	M44f		cameraToCascadeTextureSpace[MAX_SHADOW_CASCADES];
#endif


	//
#if nwRENDERER_CFG_WITH_VOXEL_GI
	VoxelGrids	_gpu_voxels;
#endif // nwRENDERER_CFG_WITH_VOXEL_GI

#if nwRENDERER_ENABLE_LIGHT_PROBES
	IrradianceField		_lightProbeGrid;
	bool				_test_need_to_rebuild_sky_irradiance_map;
	bool				_test_need_to_relight_light_probes;
#endif // nwRENDERER_ENABLE_LIGHT_PROBES


#if nwRENDERER_CFG_ENABLE_idTech4
	//
	idMaterialSystem	_id_material_system;
#endif


#if nwRENDERER_CFG_WITH_SKELETAL_ANIMATION
	NwAnimationSystem	_skeletal_animation_system;
#endif

	TPtr< NwClump >			_object_storage_clump;

//	AllocatorI &			_allocator;
//
//public:
//	RenderSystemData( AllocatorI& allocator )
//		: _allocator( allocator )
//	{
//	}
};

namespace Globals
{

extern TPtr< RenderSystemData >	g_data;

inline AllocatorI& GetAllocator()
{ return MemoryHeaps::renderer(); }

}//namespace Globals

}//namespace Rendering
