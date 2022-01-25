/*
=============================================================================
	File:	SceneRenderer.h
	Desc:	High-level scene renderer interface.
=============================================================================
*/
#pragma once

namespace Rendering {



///
struct MeshRendererState
{
	Material*	current_material;

public:
	MeshRendererState()
	{
		current_material = nil;
	}
};


ERet SubmitEntities(
	const NwCameraView& scene_view
	, const RenderEntityList& visible_entities
	, const RrGlobalSettings& renderer_settings
	);


/// will be applied as a full-screen pass
ERet SubmitDirectionalLight(
							const RrDirectionalLight& light
							, const NwCameraView& scene_view
							, const RrGlobalSettings& renderer_settings
							, NGpu::NwRenderContext & render_context


							// Voxel Occupancy/Visibility
							, const VXGI::VoxelGrids* voxel_grids

							// DDGI
							, const VXGI::IrradianceField* irradiance_field = nil
							);



ERet SubmitTransientPointLight(
							   const V3f position
							   , const float radius
							   , const V3f color
							   );


///
struct RenderCallbackParameters
{
	const TSpan< const RenderEntityBase* >	entities;

	// shadow or voxel clipmap cascade
	const U32	cascade_index;

	/// valid only when voxelizing meshes
	const CubeMLf	voxel_cascade_bounds;	// 16

	const NwCameraView &	scene_view;
	const RenderPath &		render_path;
	NGpu::NwRenderContext &	render_context;
	const RrGlobalSettings&	renderer_settings;

	void * const			user_data;

public:
	RenderCallbackParameters(
		const TSpan< const RenderEntityBase* > entities
		, const U32 cascade_index
		, const CubeMLf& voxel_cascade_bounds
		, const NwCameraView& scene_view
		, const RenderPath& render_path
		, NGpu::NwRenderContext& render_context
		, const RrGlobalSettings& renderer_settings
		, void* user_data
		)
		: entities(entities)
		, cascade_index(cascade_index)
		, voxel_cascade_bounds(voxel_cascade_bounds)
		, scene_view(scene_view)
		, render_path(render_path)
		, render_context(render_context)
		, renderer_settings(renderer_settings)
		, user_data(user_data)
	{}
};

/// called for each entity type
typedef ERet RenderCallback( const RenderCallbackParameters& parameters );

ERet RegisterCallbackForRenderingEntitiesOfType(
							const ERenderEntity entity_type
							, RenderCallback* render_callback
							, void* user_data
							);


}//namespace Rendering
