// setting shader parameters
#pragma once

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Scene/SceneRenderer.h>


namespace Rendering
{

struct RenderCallbacks
{
	RenderCallback *	code[ RE_MAX ];	//!< render function for each entity type
	void *				data[ RE_MAX ];
public:
	RenderCallbacks()
	{
		mxZERO_OUT_ARRAY(code);
		mxZERO_OUT_ARRAY(data);
	}
};


///
struct VoxelizationCallbackParameters
{
	const void **			objects;
	int						count;

	// voxel volume in world space
	CubeCRd					voxelVolumeBoundsWorld;
	const RenderPath *		render_path;
	NGpu::NwRenderContext *		render_context;

	void *					user_data;

	NGpu::LayerID			drawingOrder;
	U8						cascadeIndex;
};

typedef ERet VoxelizationCallback(
								  const VoxelizationCallbackParameters& parameters
								  );

struct VoxelizationCallbacks
{
	VoxelizationCallback *	code[ RE_MAX ];	//!< render function for each entity type
	void *					data[ RE_MAX ];
public:
	VoxelizationCallbacks()
	{
		mxZERO_OUT_ARRAY(code);
		mxZERO_OUT_ARRAY(data);
	}
};




template< class PARAMETERS >
struct RenderCallbacksWithParameters
{
	typedef ERet RenderCallbackT( const PARAMETERS& parameters );

	RenderCallbackT *	callback[ RE_MAX ];	//!< render function for each entity type
	void *				userData[ RE_MAX ];

public:
	RenderCallbacksWithParameters()
	{
		mxZERO_OUT_ARRAY(code);
		mxZERO_OUT_ARRAY(data);
	}
};

template< class PARAMETERS >
void renderObjectLists(
					   const RenderEntityList& objectLists
					   , const RenderCallbacksWithParameters<PARAMETERS>& cbs
					   , const PARAMETERS& parameters
					   )
{
	const void*const*const objects = visible_objects.ptrs.raw();

	RenderCallbackParameters	parameters;

	parameters.cascade_index = cascade_index;

	for( UINT entity_type = 0; entity_type < RE_MAX; entity_type++ )
	{
		const UINT num_entities_to_draw = visible_objects.count[ entity_type ];
		RenderCallback * render_callback = render_callbacks.code[ entity_type ];

		if( num_entities_to_draw && render_callback )
		{
			const U32 start_offset = visible_objects.offset[ entity_type ];
			const void*const*const entities_to_draw = objects + start_offset;
			void* render_callback_parameter = render_callbacks.data[ entity_type ];

			//
			parameters.objects = (const void**) entities_to_draw;
			parameters.count = num_entities_to_draw;

			parameters.camera = &scene_view;
			parameters.render_path = &render_path;
			parameters.render_context = &render_context;

			parameters.user_data = render_callback_parameter;

			(*render_callback)( parameters );
		}
	}//For each entity type.
}





void RenderVisibleObjects(
	const RenderEntityList& visible_objects
	, const U32 cascade_index
	, const CubeMLf& voxel_cascade_bounds

	, const NwCameraView& scene_view

	, const RenderCallbacks& render_callbacks
	, const RenderPath& render_path
	, const RrGlobalSettings& renderer_settings

	, NGpu::NwRenderContext & render_context
	);

ERet ViewState_From_ScenePass( NGpu::ViewState &viewState, const ScenePassData& passData, const NwViewport& viewport );

}//namespace Rendering
