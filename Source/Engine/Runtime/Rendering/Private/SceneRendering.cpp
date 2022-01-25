#include <Base/Base.h>
#pragma hdrstop

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>
#include <Rendering/Private/ShaderInterop.h>
#include <Rendering/Private/SceneRendering.h>


namespace Rendering
{
void RenderVisibleObjects(
	const RenderEntityList& visible_objects
	, const U32 cascade_index
	, const CubeMLf& voxel_cascade_bounds

	, const NwCameraView& scene_view
	//, const NwFloatingOrigin& floating_origin

	, const RenderCallbacks& render_callbacks
	, const RenderPath& render_path
	, const RrGlobalSettings& renderer_settings
	, NGpu::NwRenderContext & render_context
	)
{
	const RenderEntityBase*const*const objects = visible_objects.ptrs.raw();

	//
	for( UINT entity_type = 0; entity_type < RE_MAX; entity_type++ )
	{
		const UINT num_entities_to_draw = visible_objects.count[ entity_type ];
		RenderCallback * render_callback = render_callbacks.code[ entity_type ];

#if MX_DEBUG || MX_DEVELOPER
		if( num_entities_to_draw )
		{
			mxASSERT_PTR(render_callback);
		}
#endif // MX_DEBUG || MX_DEVELOPER

		//
		if( num_entities_to_draw && render_callback )
		{
			const U32 start_offset = visible_objects.offset[ entity_type ];
			const RenderEntityBase*const*const entities_to_draw = objects + start_offset;

			RenderCallbackParameters	parameters(
				TSpan< const RenderEntityBase* >(
					(const RenderEntityBase**)entities_to_draw
					, num_entities_to_draw
				)
				, cascade_index
				, voxel_cascade_bounds
				, scene_view
				, render_path
				, render_context
				, renderer_settings
				, render_callbacks.data[ entity_type ]
				);

			(*render_callback)( parameters );
		}

	}//For each entity type.
}



ERet ViewState_From_ScenePass(
							  NGpu::ViewState &viewState
							  , const ScenePassData& passData
							  , const NwViewport& viewport
							  )
{
	mxASSERT(!isInvalidViewport( viewport ));

	const UINT numRenderTargets = passData.render_targets.num();
	for( UINT iRT = 0; iRT < numRenderTargets; iRT++ )
	{
		const NameHash32 renderTargetId = passData.render_targets[ iRT ];
		if( renderTargetId == ScenePassData::BACKBUFFER_ID ) {
			viewState.color_targets[ iRT ].SetDefault();
		} else {
			TbColorTarget *	renderTarget;
			mxDO(GetByHash(renderTarget, renderTargetId, ClumpList::g_loadedClumps));
			viewState.color_targets[ iRT ] = renderTarget->handle;
		}
	}
	viewState.target_count = numRenderTargets;

	if( passData.depth_stencil_name_hash != 0 ) {
		TbDepthTarget *	depthStencil;
		if(mxSUCCEDED(GetByHash(depthStencil, passData.depth_stencil_name_hash, ClumpList::g_loadedClumps))) {
			viewState.depth_target = depthStencil->handle;
		} else {
			//TODO: FIXME
			//mxASSERT(false);
		}
	}

	mxASSERT(!isInvalidViewport( viewport ));
	viewState.viewport = viewport;

	viewState.depth_clear_value = passData.depth_clear_value;
	viewState.stencil_clear_value = passData.stencil_clear_value;
	viewState.flags = passData.view_flags;

	return ALL_OK;
}





static
ERet _SetupModelUniforms(
						 G_PerObject	**cb_per_object_
						 , G_SkinningData **cb_skinning_data_
						 , const TSpan< const M44f >& joint_matrices
						 , const M44f& local_to_world_matrix
						 , const NwCameraView& scene_view
						 , const RenderPath& render_path
						 , NGpu::NwRenderContext & render_context
						 )
{
	{
		G_PerObject	*	cb_per_object;
		mxDO(render_context._command_buffer.Allocate( &cb_per_object ));

		ShaderGlobals::copyPerObjectConstants(
			cb_per_object
			, local_to_world_matrix
			, scene_view.view_matrix
			, scene_view.view_projection_matrix
			, 0	// const UINT vxgiCascadeIndex
			);

		*cb_per_object_ = cb_per_object;
	}

	//
	if( UINT num_bone_matrices = joint_matrices.num() )
	{
		G_SkinningData	*	cb_skinning_data;
		mxDO(render_context._command_buffer.Allocate( &cb_skinning_data ));

		ShaderGlobals::setSkinningMatrices(
			cb_skinning_data
			, joint_matrices.raw()
			, num_bone_matrices
			);

		*cb_skinning_data_ = cb_skinning_data;
	}

	return ALL_OK;
}

static
ERet _SubmitMeshWithInstanceData(
	const NwMesh& mesh
	, const M44f& local_to_world_matrix
	, const NwCameraView& scene_view
	, const TSpan< const M44f >& joint_matrices
	, const TSpan< Material*const >& submesh_materials
	, NGpu::NwRenderContext & render_context
	, const TbPassMaskT allowed_passes_mask
	)
{
	//
	MeshRendererState	mesh_renderer_state;

	//
	Rendering::RenderSystemData& render_system = *Rendering::Globals::g_data;
	const RenderPath& render_path = Rendering::Globals::GetRenderPath();

	//
	const float distance_to_near_clipping_plane = Plane_PointDistance(
		scene_view.near_clipping_plane,
		M44_getTranslation(local_to_world_matrix)
		);

	const U32 sort_key_from_depth = 0;//Rendering::Sorting::DepthToBits( distance_to_near_clipping_plane );

	//
	G_PerObject	*	cb_per_object = nil;	// always valid
	G_SkinningData	*	cb_skinning_data = nil;	// optional

	mxDO(Rendering::_SetupModelUniforms(
		&cb_per_object
		, &cb_skinning_data
		, joint_matrices
		, local_to_world_matrix
		, scene_view
		, render_path
		, render_context
		));

	//
	NGpu::Cmd_Draw	dip;

	SetMeshState(
		dip
		, mesh
		);

	//
	NGpu::RenderCommandWriter	cmd_writer( render_context );

	//
	const Submesh* submeshes = mesh.submeshes.raw();
	const UINT num_submeshes = mesh.submeshes.num();

	const Material*const*const submesh_materials_array = submesh_materials._data;

	//
	for( UINT submesh_index = 0; submesh_index < num_submeshes; submesh_index++ )
	{
		const Submesh& submesh = submeshes[ submesh_index ];

		const Material& material = *submesh_materials_array[ submesh_index ];

		const TSpan< const MaterialPass > passes = material.passes;
		//
		for( UINT pass_index = 0; pass_index < passes._count; pass_index++ )
		{
			const MaterialPass& pass = passes._data[ pass_index ];

			if( BIT(pass.filter_index) & allowed_passes_mask )
			{
				//
				//IF_DEVELOPER NGpu::Dbg::GpuScope	perfScope(
				//	render_context._command_buffer
				//	, RGBAf::GREEN.ToRGBAi().u
				//	, "Model dip[%d]: %s", submesh_index, AssetId_ToChars( material._id )
				//	);
				//cmd_writer.dbgPrintf(0,
				//"Prepare Model dip[%d]: %s", submesh_index, AssetId_ToChars( material._id )
				//);

				mxDO(material.command_buffer.PushCopyInto(
					render_context._command_buffer
					));

				//if(mesh_renderer_state.current_material != &material)
				//{
				//	UNDONE;
				//}

				dip.program = pass.program;

				// Ensure that the uniform buffers will be filled with model matrices.
				// The uniform buffers will be updated only when necessary.
				{
					mxDO(cmd_writer.BindCBufferDataCopy(
						*cb_per_object
						, render_system._global_uniforms.per_model.slot
						, render_system._global_uniforms.per_model.handle

						, (MX_DEVELOPER ? "cb_per_object" : nil)
						));

					if( cb_skinning_data )
					{
						mxDO(cmd_writer.BindCBufferDataCopy(
							*cb_skinning_data
							, render_system._global_uniforms.skinning_data.slot
							, render_system._global_uniforms.skinning_data.handle
							
							, (MX_DEVELOPER ? "cb_skinning_data" : nil)
							));
					}
				}

				//
				dip.base_vertex = 0;
				dip.vertex_count = 0;
				dip.start_index = submesh.start_index;
				dip.index_count = submesh.index_count;

//				IF_DEBUG snprintf(dip.dbgname, mxCOUNT_OF(dip.dbgname), "Model dip[%d]: %s", submesh_index, AssetId_ToChars( material._id ));

				//NGpu::CommandBufferWriter(render_context._command_buffer).dbgPrintf(0,
				//"Draw Model dip[%d]: %s", submesh_index, AssetId_ToChars( material._id )
				//);

				IF_DEBUG dip.src_loc = GFX_SRCFILE_STRING;

				//
				cmd_writer.SetRenderState(
					pass.render_state
					);

				//
				NGpu::Commands::Draw(
					dip
					, render_context._command_buffer
					);

				cmd_writer.SubmitCommandsWithSortKey(
					NGpu::buildSortKey(
						pass.draw_order,
						pass.program,
						sort_key_from_depth
					) + NGpu::FIRST_SORT_KEY

					nwDBG_CMD_SRCFILE_STRING
				);
			}//If pass is allowed
		}//For each pass.
	}//For each submesh.

	return ALL_OK;
}

ERet submitMesh(
	const NwMesh& mesh
	, const M44f& local_to_world_matrix
	, const NwCameraView& scene_view
	, NGpu::NwRenderContext & render_context
	, const TbPassMaskT allowed_passes_mask
	)
{
	mxOPTIMIZE("remove conversion!");
	UNDONE;
	TFixedArray< Material*, nwRENDERER_NUM_RESERVED_SUBMESHES >	default_materials;
	default_materials.setNum( mesh.default_materials_ids._count );
	UNDONE;
	//for( UINT i = 0; i < mesh._default_materials._count; i++ ) {
	//	default_materials._data[i] = mesh._default_materials._data[i]->getDefaultInstance();
	//}

	//
	mxDO(_SubmitMeshWithInstanceData(
		mesh
		, local_to_world_matrix
		, scene_view
		, TSpan< M44f >()
		, Arrays::GetSpan( default_materials )
		, render_context
		, allowed_passes_mask
		));

	return ALL_OK;
}

ERet submitMeshWithCustomMaterial(
	const NwMesh& mesh
	, const M44f& local_to_world_matrix
	, const NwCameraView& scene_view
	, const Material* material
	, NGpu::NwRenderContext & render_context
	, const TbPassMaskT allowed_passes_mask
	)
{
	UNDONE;
	return ALL_OK;
}

ERet submitModel( const MeshInstance& model
			   , const NwCameraView& scene_view
			   , NGpu::NwRenderContext & render_context
			   , const TbPassMaskT allowed_passes_mask
			   )
{
	return _SubmitMeshWithInstanceData(
		*model.mesh
		, model.transform->getLocalToWorld4x4Matrix()
		, scene_view
		, Arrays::GetSpan( model.joint_matrices )
		, Arrays::GetSpan( model.materials )
		, render_context
		, allowed_passes_mask
		);
}

ERet submitModelWithCustomTransform(
									const MeshInstance& model
									, const M44f& local_to_world_transform
									, const NwCameraView& scene_view
									, NGpu::NwRenderContext & render_context
									, const TbPassMaskT allowed_passes_mask
									)
{
	return _SubmitMeshWithInstanceData(
		*model.mesh
		, local_to_world_transform
		, scene_view
		, Arrays::GetSpan( model.joint_matrices )
		, Arrays::GetSpan( model.materials )
		, render_context
		, allowed_passes_mask
		);
}


ERet renderMeshInstances(
	const RenderCallbackParameters& parameters
	)
{
	tbPROFILE_FUNCTION;
UNDONE;
#if 0
	GL::NwRenderContext & render_context = parameters.render_context;

	//
	const ScenePassInfo scene_pass_info = parameters.render_path->getPassInfo( ScenePasses::FillGBuffer );
	const TbPassMaskT	allowed_passes_mask = BIT(scene_pass_info.filter_index);

	//
	const NwCameraView& scene_view = parameters.camera;

	//const NwFloatingOrigin	floating_origin = *parameters.floating_origin;

	//
	const MeshInstance*const*const mesh_instances = (const MeshInstance**) parameters.objects;

	//
	for( int i = 0; i < parameters.count; i++ )
	{
		const MeshInstance& mesh_instance = *(mesh_instances[ i ]);

		const V3f	min_corner_pos_relative_to_floating_origin = floating_origin.getRelativePosition(
			V3d::fromXYZ( mesh_instance.transform->local_to_world.GetTranslation() )
			);

		M44f relative_local_to_world_matrix = mesh_instance.transform->local_to_world;
		relative_local_to_world_matrix.setTranslation( min_corner_pos_relative_to_floating_origin );

		//
		Rendering::submitModelWithCustomTransform(
			mesh_instance
			, relative_local_to_world_matrix
			, scene_view
			, render_context
			, allowed_passes_mask
			);

	}//For each visible terrain chunk.
#endif
	return ALL_OK;
}

ERet SubmitEntities(
	const NwCameraView& scene_view
	, const RenderEntityList& visible_entities
	, const RrGlobalSettings& renderer_settings
	)
{
	RenderSystemData& data = *Globals::g_data;

	//
	//const NwFloatingOrigin floating_origin = spatial_database->getFloatingOrigin();

	RenderVisibleObjects(
		visible_entities
		, 0
		, CubeMLf::fromCenterRadius(CV3f(0), 0)

		, scene_view
		//, NwFloatingOrigin::GetDummy_TEMP_TODO_REMOVE()//floating_origin

		, data._render_callbacks
		, Globals::GetRenderPath()
		, renderer_settings

		, NGpu::getMainRenderContext()
		);

	//
	mxDO(data._deferred_renderer.DrawEntities(
		scene_view
		, visible_entities
		, data
		, renderer_settings
		));

	//
	//_debug_drawWorld( spatial_database, scene_view, renderer_settings );

	return ALL_OK;
}

}//namespace Rendering
