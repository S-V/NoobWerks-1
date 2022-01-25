/*
=============================================================================
	idTech4 (Doom 3) shader/material support
=============================================================================
*/
#include <Base/Base.h>
#pragma hdrstop
#include <Core/Assets/AssetManagement.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Serialization/Serialization.h>
#include <GPU/Public/graphics_device.h>

#include <Rendering/BuildConfig.h>
#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Scene/CameraView.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>
#include <Rendering/Private/Modules/idTech4/idRenderModel.h>
#include <Rendering/Private/ShaderInterop.h>


namespace Rendering
{
mxBEGIN_FLAGS( idRenderModelFlagsT )
	mxREFLECT_BIT( NeedsWeaponDepthHack,	idRenderModelFlags::NeedsWeaponDepthHack ),
mxEND_FLAGS


mxDEFINE_CLASS( idRenderModel );
mxBEGIN_REFLECTION( idRenderModel )
	mxMEMBER_FIELD( local_to_world ),
	mxMEMBER_FIELD( flags ),
	mxMEMBER_FIELD( mesh ),
	mxMEMBER_FIELD( materials ),
	mxMEMBER_FIELD( joint_matrices ),
	mxMEMBER_FIELD( shader_parms ),
mxEND_REFLECTION
idRenderModel::idRenderModel()
{
	local_to_world = g_MM_Identity;

	flags.raw_value = 0;

	TSetStaticArray( shader_parms.f, 0.0f );

	world_proxy_handle.SetNil();
	Arrays::initializeWithExternalStorage( materials, z__materials_storage );
}


namespace idRenderModel_
{
	ERet CreateFromMesh(
		idRenderModel *&new_model_
		, const TResPtr< NwMesh >& mesh
		, NwClump & clump
		)
	{
		idRenderModel* new_model;
		mxDO(clump.New( new_model ));

		//
		new_model->local_to_world = g_MM_Identity;

		//
		new_model->mesh = mesh;

		//
		mxDO(new_model->materials.setNum( mesh->default_materials_ids.num() ));

		//
		TResPtr< idMaterial >	default_material(
			MakeAssetID("models_characters_male_npc_hazmat_visor")
			);
		mxDO(default_material.Load());

		//
		nwFOR_EACH_INDEXED(
			const AssetID& material_id,
			mesh->default_materials_ids,
			i
			)
		{
			idMaterial* material;
			/*mxDO*/(Resources::Load(
				material,
				material_id,
				&clump,
				LoadFlagsT(0),
				default_material._ptr
				));
			new_model->materials[i] = material;
		}

		new_model_ = new_model;
		return ALL_OK;
	}


	//void RegisterCallbackForRenderingEntitiesOfType(
	//	RenderCallbacks & render_callbacks
	//	)
	//{
	//	render_callbacks.code[ RE_idRenderModel ] = &RenderInstances;
	//	render_callbacks.data[ RE_idRenderModel ] = nil;
	//}



static void UpdateTextureMatrixUniforms(
	idMaterialUniforms &uniforms_
	, const idMaterialPass& pass
	, const idExpressionEvaluationScratchpad& eval_scratchpad
	)
{
	for( int i = 0; i < 3; i++ ) {
		uniforms_.texture_matrix_row0.a[i] = eval_scratchpad.regs[ pass.texture_matrix_row0[i] ];
	}
	// the. w component is unused
	//uniforms_.texture_matrix_row0.w = 1;

	for( int i = 0; i < 3; i++ ) {
		uniforms_.texture_matrix_row1.a[i] = eval_scratchpad.regs[ pass.texture_matrix_row1[i] ];
	}
	// the. w component is unused
	//uniforms_.texture_matrix_row1.w = 1;
}



ERet RenderInstance(
	const idRenderModel& render_model
	, const M44f& local_to_world_matrix
	, const NwCameraView& scene_view
	, const RrGlobalSettings& renderer_settings
	, NGpu::NwRenderContext & render_context
	, const float custom_FoV_Y_in_degrees
	)
{
	const RenderSystemData& render_system = *Globals::g_data;

#if nwRENDERER_CFG_ENABLE_idTech4

	const idMaterialSystem& material_system = render_system._id_material_system;

	const RenderPath& render_path = render_system.GetRenderPath();

	//
	const float distance_to_near_clipping_plane = Plane_PointDistance(
		scene_view.near_clipping_plane,
		M44_getTranslation(local_to_world_matrix)
		);
	const U32 sort_key_from_depth = Sorting::DepthToBits( distance_to_near_clipping_plane );



	// MD5 models always have joints
	G_SkinningData	*	cb_skinning_data = nil;
	mxDO(ShaderGlobals::SetupSkinningMatrices(
		&cb_skinning_data
		, Arrays::GetSpan( render_model.joint_matrices )
		, render_context
		));

	//
	const NwMesh& mesh = *render_model.mesh;


	//
	const bool is_first_person_weapon_model =
		render_model.flags & idRenderModelFlags::NeedsWeaponDepthHack
		;

	const RrGameSpecificSettings& game_specific_settings = renderer_settings.game_specific;

	//
	G_PerObject	*	cb_per_object = nil;

	if(mxLIKELY(!is_first_person_weapon_model))
	{
		mxDO(ShaderGlobals::SetupPerObjectMatrices(
			&cb_per_object
			, local_to_world_matrix
			, scene_view
			, render_context
			));
		cb_per_object->g_object_uint4_params = UInt4::set(0,0,0,0);
	}
	else
	{
		// Change the projection matrix
		// to bring the first-person weapon model very close to the near plane.

		float FoV_Y_in_degrees = custom_FoV_Y_in_degrees;
		mxASSERT2(FoV_Y_in_degrees > 0, "Did you forget to pass custom FoV for weapon?");
		FoV_Y_in_degrees = maxf(FoV_Y_in_degrees, 50);

		const float custom_FoV_Y_in_radians = DEG2RAD(FoV_Y_in_degrees);

		//
		M44f	projection_matrix, inverse_projection_matrix;

		if(game_specific_settings.use_infinite_far_clip_plane)
		{
			M44_ProjectionAndInverse_D3D_ReverseDepth_InfiniteFarPlane(
				&projection_matrix, &inverse_projection_matrix
				, custom_FoV_Y_in_radians
				, scene_view.aspect_ratio

				// Bring the near clipping plane very close to prevent near-depth clipping the weapon model.
				// But not too close (like 1e-3), because the weapon model will poke into walls and enemies.
				, game_specific_settings.near_clip_plane_for_weapon_depth_hack
				);
		}
		else
		{
			M44_ProjectionAndInverse_D3D_ReverseDepth(
				&projection_matrix, &inverse_projection_matrix
				, custom_FoV_Y_in_radians
				, scene_view.aspect_ratio
				, game_specific_settings.near_clip_plane_for_weapon_depth_hack
				, game_specific_settings.far_clip_plane_for_weapon_depth_hack
				);
		}

		const M44f	view_projection_matrix = scene_view.view_matrix * projection_matrix;

		const M44f worldViewMatrix = M44_Multiply(local_to_world_matrix, scene_view.view_matrix);
		const M44f worldViewProjectionMatrix = M44_Multiply(local_to_world_matrix, view_projection_matrix);

		mxDO(ShaderGlobals::SetupPerObjectMatrices(
			&cb_per_object
			, local_to_world_matrix
			, scene_view
			, render_context
			));
		cb_per_object->g_world_matrix = M44_Transpose( local_to_world_matrix );
		cb_per_object->g_world_view_matrix = M44_Transpose( worldViewMatrix );
		cb_per_object->g_world_view_projection_matrix = M44_Transpose( worldViewProjectionMatrix );
		cb_per_object->g_object_uint4_params = UInt4::set(0,0,0,1);
	}



	//
	NGpu::Cmd_Draw	draw_cmd;

	draw_cmd.input_layout = mesh.vertex_format->input_layout_handle;
	draw_cmd.primitive_topology = mesh.topology;
	draw_cmd.use_32_bit_indices = mesh.flags & MESH_USES_32BIT_IB;

	draw_cmd.VB = mesh.geometry_buffers.VB->buffer_handle;
	draw_cmd.IB = mesh.geometry_buffers.IB->buffer_handle;


	//
	const Submesh* submeshes = mesh.submeshes.raw();
	const UINT num_submeshes = mesh.submeshes.num();

	idMaterial*const*const materials = render_model.materials._data;

	idExpressionEvaluationScratchpad	eval_scratchpad;

	//
	for( UINT submesh_index = 0; submesh_index < num_submeshes; submesh_index++ )
	{
		idMaterial & material = *materials[ submesh_index ];
		
		//if(mxLIKELY( material.flags & idMaterialFlags::HasOnlyConstantRegisters ))
		//{
		//	UNDONE;
		//}
		//else
		{
			material_system.EvaluateExpressions(
				material
				, render_model.shader_parms
				, eval_scratchpad
				);
		}

		//
		const idMaterial::LoadedData& material_data = *material.data;

		const idMaterialPass* passes = material_data.passes._data;
		const UINT num_passes = material_data.passes.num();

		//
		for( UINT pass_index = 0
			; pass_index < num_passes
			; pass_index++ )
		{
			const idMaterialPass& pass = passes[ pass_index ];

			// if expr returns 0, the material should not be rendered
			const bool should_render_this_pass = eval_scratchpad.regs[ pass.condition_register ] != 0;
			//
			if( should_render_this_pass )
			{
				mxOPTIMIZE("reduce pointer chasing");
				
				//
				NGpu::RenderCommandWriter	cmd_writer( render_context );

				//
				//mxDO(material_data.command_buffer.copyInto( render_context._command_buffer ));
				//
				//
				cmd_writer.SetInput( 0, pass.diffuse_map->m_resource );
				cmd_writer.setSampler(0, BuiltIns::g_samplers[BuiltIns::DiffuseMapSampler]);
				
				if(pass.normal_map.IsValid()) {
					cmd_writer.SetInput( 1, pass.normal_map->m_resource );
					cmd_writer.setSampler(1, BuiltIns::g_samplers[BuiltIns::NormalMapSampler]);
				}

				if(pass.specular_map.IsValid()) {
					cmd_writer.SetInput( 2, pass.specular_map->m_resource );
					cmd_writer.setSampler(2, BuiltIns::g_samplers[BuiltIns::SpecularMapSampler]);
				}


#if 1
			//
				//if(is_first_person_weapon_model)
				//{
				//	NwRenderState *	rs;
				//	mxDO(Resources::Load(rs, MakeAssetID("idtech4_fp_weapon")));
				//	cmd_writer.SetRenderState( *rs );
				//}
				//else
				{
					cmd_writer.SetRenderState( pass.render_state );
				}
#else
				//if(is_first_person_weapon_model)
				//{
				//	NwRenderState *	rs;
				//	mxDO(Resources::Load(rs, MakeAssetID("idtech4_fp_weapon")));
				//	cmd_writer.SetRenderState( *rs );
				//}
				//else
				{
					NwRenderState *	rs;
					mxDO(Resources::Load(rs, MakeAssetID("idtech4_default")));
					cmd_writer.SetRenderState( *rs );
				}
#endif


				//if(strstr(material.id.d.c_str(), "models/weapons/shotgun/shotgun_mflashb")) {
				////	mxASSERT(0);
				//}


				// Update material uniforms, if needed.

				const bool has_texture_transform = ( pass.flags & idMaterialPassFlags::HasTextureTransform );
				const bool is_alpha_tested = ( pass.flags & idMaterialPassFlags::HasAlphaTest );
				{
					idMaterialUniforms	uniforms;

					if( has_texture_transform | is_alpha_tested )
					{
						if( has_texture_transform )
						{
							UpdateTextureMatrixUniforms(uniforms, pass, eval_scratchpad);
						}

						uniforms.alpha_threshold = is_alpha_tested
							? eval_scratchpad.regs[ pass.alpha_test_register ]
							: 0
							;

						cmd_writer.BindCBufferDataCopy(
							uniforms
							, 5 // from shader source file
							, HBuffer::MakeNilHandle()
							IF_DEVELOPER , "material_uniforms"
							);
					}
				}



				//
				const HProgram program_handle = pass.program_handle;

				draw_cmd.setup( program_handle );

				// Ensure that the uniform buffers are filled with model matrices.
				// The uniform buffers will be updated only when necessary.
				{
					mxOPTIMIZE("these are using memcpy!");

					mxDO(cmd_writer.BindCBufferDataCopy(
						*cb_per_object
						, render_system._global_uniforms.per_model.slot
						, render_system._global_uniforms.per_model.handle
						, (MX_DEVELOPER ? "cb_per_object" : nil)
						));

					mxDO(cmd_writer.BindCBufferDataCopy(
						*cb_skinning_data
						, render_system._global_uniforms.skinning_data.slot
						, render_system._global_uniforms.skinning_data.handle
						, (MX_DEVELOPER ? "cb_skinning_data" : nil)
						));
				}

				//
				const Submesh& submesh = submeshes[ submesh_index ];
				//
				draw_cmd.base_vertex = 0;
				draw_cmd.vertex_count = 0;
				draw_cmd.start_index = submesh.start_index;
				draw_cmd.index_count = submesh.index_count;

				IF_DEBUG draw_cmd.src_loc = GFX_SRCFILE_STRING;


	#if nwRENDERER_DEBUG_DRAW_CALLS
				cmd_writer.SetMarker(RGBAi::GREEN, "%s", material.id.d.c_str());
	#endif

				cmd_writer.Draw( draw_cmd );

				//
				mxOPTIMIZE("weapons should be rendered first!");
				//
				cmd_writer.SubmitCommandsWithSortKey(
					is_first_person_weapon_model
					?
					// Draw the first-person weapon model above everything else
					(
					game_specific_settings.draw_fp_weapons_last
					? NGpu::buildSortKey( pass.view_id, ~0 ) // last!
					: NGpu::buildSortKey( pass.view_id, 1 ) // first! - not working :(
					)
					: NGpu::buildSortKey( pass.view_id, program_handle, sort_key_from_depth )
					);

			}//if this pass should be rendered

		}//for each pass in material

	}//For each submesh.

#else

UNDONE;

#endif

	return ALL_OK;
}

	ERet RenderInstances(
		const RenderCallbackParameters& parameters
		)
	{
		tbPROFILE_FUNCTION;

		NGpu::NwRenderContext & render_context = parameters.render_context;

		//
		const NwCameraView& scene_view = parameters.scene_view;

		//const NwFloatingOrigin	floating_origin = *parameters.floating_origin;
UNDONE;
#if 0
		//
		const idRenderModel*const*const render_models = (const idRenderModel**) parameters.objects;

		//
		for( int i = 0; i < parameters.count; i++ )
		{
			const idRenderModel& render_model = *(render_models[ i ]);

			const V3f	pos_relative_to_floating_origin = floating_origin.getRelativePosition(
				V3d::fromXYZ( render_model.local_to_world.GetTranslation() )
				);

			M44f relative_local_to_world_matrix = render_model.local_to_world;
			relative_local_to_world_matrix.setTranslation(pos_relative_to_floating_origin);

			//
			RenderInstance(
				render_model
				, relative_local_to_world_matrix
				, scene_view
				, parameters.renderer_settings
				, render_context
				);

		}//For each visible terrain chunk.
#endif

		return ALL_OK;
	}

}//namespace


#if 0
namespace SpatialDatabaseI_
{
	void Add_idRenderModel(
		SpatialDatabaseI* spatial_database
		, idRenderModel* render_model
		)
	{
		mxASSERT(render_model->world_proxy_handle.IsNil());

		render_model->world_proxy_handle = spatial_database->AddEntity(
			render_model,
			RE_idRenderModel,
			AABBd::fromOther(
				render_model->getAabbInWorldSpace()
			)
		);
	}

	void Remove_idRenderModel(
		SpatialDatabaseI* spatial_database
		, idRenderModel* render_model
		)
	{
		mxASSERT(render_model->world_proxy_handle.IsValid());

		spatial_database->RemoveEntity(
			render_model->world_proxy_handle
			);

		render_model->world_proxy_handle.SetNil();
	}

}//namespace SpatialDatabaseI_
#endif

}//namespace Rendering
