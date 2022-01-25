/*
=============================================================================
	File:	
	Desc:	
=============================================================================
*/
#pragma once

#include <Core/ObjectModel/Clump.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Scene/Light.h>
//#include <Renderer/scene/VisibilityCulling.h>
//#include <Renderer/Shadows.h>
//#include <Renderer/PostProcessing.h>
#include <Rendering/Private/Modules/VoxelGI/_vxgi_system.h>
#include <Rendering/Private/Pipelines/TiledDeferred/TiledCommon.h>
//#include <Renderer/private/shadows/shadow_map_renderer.h>


namespace Rendering
{
class RenderSystemData;
}


namespace Rendering {



mxREMOVE_THIS
/// provides basic functionality for most rendering pipelines/render loops/paths
/// e.g. TiledDeferredRenderer, ForwardPlusRenderer, etc.
class ASceneRenderer: public AObject, NonCopyable
{
public:
	virtual ~ASceneRenderer() {}

	//mxREFACTOR("should render only visibility results")
	//virtual ERet DrawVisibleBatches(
	//	const NwCameraView& sceneView,
	//	const Viewport& viewport,
	//	const NwClump& sceneData
	//) = 0;

	/// for debugging
	//virtual void VGetInfo( String & _info );

public:
	static ASceneRenderer* createDeferredRenderer( AllocatorI & storage );

protected:
	ASceneRenderer() {}
};


struct TransientPointLight
{
	V3f	position;
	float	radius;
	V3f	color;
	float	_pad;
};

///
class DeferredRenderer: public ASceneRenderer
{
public:
	TPtr< NwClump >	m_rendererData;	//!< storage for graphics resources

	TPtr< RenderPath >	m_renderPath;

	// Geometry Buffer:
	TPtr< TbColorTarget >	m_colorRT0;	//!< RGB: world-space normal
	TPtr< TbColorTarget >	m_colorRT1;	//!< RGB: diffuse reflectance
	//TPtr< TbColorTarget >	m_colorRT2;	//!< RGB: specular color
	TPtr< TbDepthTarget >	m_depthRT;	//!< main depth-stencil buffer

	TPtr< TbColorTarget >	m_litTexture;	//!< HDR light accumulator

	TPtr< TbColorTarget >	m_adaptedLuminance[2];	//!< gray-scale luminance values from previous frames
	UINT					m_iCurrentLuminanceRT;	//!< index of the current texture with average luminance

	// buffer for tiled deferred shading
	HBuffer	_deferred_lights_buffer_handle;

#if DEBUG_EMIT_POINTS
	// 'Append' buffers for debugging
	//HBuffer	m_debug_lines;
	HBuffer	u_debugPoints;
	enum { MAX_DEBUG_POINTS = 4096 };
#endif

//	TPtr< ACullingSystem >	m_cullingSystem;
//	RenderEntityList	m_visibleObjects;	//!< list of currently visible objects

	//ShadowRenderer	m_shadowRenderer;
	//RrShadowMapRenderer	_shadow_map_renderer;

	//AmbientObscurance	m_AO;
	//ReduceDepth_CS_Util	_depth_reducer_CS;

	TArray< TransientPointLight >	transient_point_lights;

public:
	DeferredRenderer();
	~DeferredRenderer();

	ERet Initialize( U32 _screenWidth, U32 _screenHeight );
	void Shutdown();


	virtual ERet DrawEntities(
		const NwCameraView& scene_view
		, const RenderEntityList& visible_entities
		, const RenderSystemData& render_system
		//, const SpatialDatabaseI* world
		, const RrGlobalSettings& renderer_settings
	) /*override*/;

	//virtual void VGetInfo( String & _info ) override;

private:
	ERet _DrawCommon(
		const NwCameraView& scene_view
		, const RrGlobalSettings& renderer_settings
		//, const Atmosphere_Rendering::NwAtmosphereRenderingParameters& atmosphere_rendering_parameters
	);


public:
	ERet applyPostProcessingAndBlit(
		const NwCameraView& scene_view
		,NGpu::NwRenderContext & render_context
		, const RrGlobalSettings& renderer_settings
		, const RenderSystemData& render_system
		//, const Atmosphere_Rendering::NwAtmosphereRenderingParameters& atmosphere_rendering_parameters
		);

	ERet addDebugRenderCommandsIfNeeded(
		const NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		, const RrGlobalSettings& renderer_settings
		) const;
	//ERet EndRender_GBuffer();
};

}//namespace Rendering
