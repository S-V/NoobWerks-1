#include <Base/Base.h>
#pragma hdrstop

#include <Rendering/Public/Settings.h>
#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Scene/CameraView.h>
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Private/ShaderInterop.h>
#include <Rendering/Private/Modules/VoxelGI/_vxgi_system.h>

#include <Voxels/public/vx5.h>
#include <Voxels/private/debug/vxp_debug.h>
#include <Voxels/private/Scene/vx5_octree_world.h>

#include <VoxelsSupport/VoxelTerrainChunk.h>
#include <VoxelsSupport/VoxelTerrainRenderer.h>
#include <Utility/Meshok/Volumes.h>


//tbPRINT_SIZE_OF(VoxelTerrainChunk);

namespace
{
ERet _SetSolidWireframeSettings(
	Rendering::Material *material,
	const Rendering::SolidWireSettings& settings
	)
{
	//
	mxDO(material->setUniform(mxHASH_STR("LineWidth"),		&settings.line_width));
	mxDO(material->setUniform(mxHASH_STR("FadeDistance"),	&settings.fade_distance));
	mxDO(material->setUniform(mxHASH_STR("PatternPeriod"),	&settings.pattern_period));
	mxDO(material->setUniform(mxHASH_STR("WireColor"),		&settings.wire_color));
	mxDO(material->setUniform(mxHASH_STR("PatternColor"),	&settings.pattern_color));
	return ALL_OK;
}

ERet RenderVoxelTerrainChunks(
	const Rendering::RenderCallbackParameters& parameters
	)
{
	NGpu::NwRenderContext & render_context = parameters.render_context;

	//
	const Rendering::RrGlobalSettings& renderer_settings = parameters.renderer_settings;

	//
	Rendering::TbPassMaskT	allowed_passes_mask = ~0;


	//
	const bool	enable_debug_wireframe
		= renderer_settings.debug.flags & Rendering::DebugRenderFlags::DrawWireframe
		;
	//
	const Rendering::ScenePassInfo scene_pass_info = parameters.render_path.getPassInfo( Rendering::ScenePasses::DebugWireframe );
	SET_FLAGS( allowed_passes_mask, BIT(scene_pass_info.filter_index), enable_debug_wireframe );
	//
	if( enable_debug_wireframe )
	{
		Rendering::Material *	standard_material;
		Resources::Load( standard_material, MakeAssetID("material_voxel_terrain_dlod") );

		Rendering::Material *	uv_textured_material;
		Resources::Load( uv_textured_material, MakeAssetID("material_voxel_terrain_uv_textured") );

		//
		const Rendering::SolidWireSettings& settings = renderer_settings.debug.wireframe;

		_SetSolidWireframeSettings( standard_material, settings );
		_SetSolidWireframeSettings( uv_textured_material, settings );
	}

	//
	const Rendering::NwCameraView& scene_view = parameters.scene_view;

	//
	const VoxelTerrainChunk*const*const terrain_chunks = (const VoxelTerrainChunk**) parameters.entities._data;

	for( int iChunk = 0; iChunk < parameters.entities._count; iChunk++ )
	{
		const VoxelTerrainChunk& terrain_chunk = *(terrain_chunks[ iChunk ]);

		//
		V3f	chunk_aabb_min_corner_in_world_space
			= V3f::fromXYZ(terrain_chunk.seam_octree_bounds_in_world_space.min_corner)
			;

#if MX_DEVELOPER
		chunk_aabb_min_corner_in_world_space += terrain_chunk.dbg_mesh_offset;
#endif

#if 0//vx5_CFG_DEBUG_MESHING
		{
			VX5::ChunkID chunk_id;
			chunk_id.M = terrain_chunk.debug_id;
			UNDONE;
			//chunk_aabb_min_corner_in_world_space += VX5::dbg_getChunkOffset(
			//	VX5::OctreeWorld::Get()._settings.debug_chunk_offset_scale, chunk_id
			//	);
		}
#endif // vx5_CFG_DEBUG_MESHING

		// Scale from [0..1] to ChunkSize and translate to AABB Min Corner
		const M44f local_to_world_matrix = M44_BuildTS(
			chunk_aabb_min_corner_in_world_space,
			terrain_chunk.seam_octree_bounds_in_world_space.side_length
			);

		////
		//const float distance_to_near_clipping_plane =
		//	Plane_PointDistance( scene_view.near_clipping_plane, chunk_aabb_min_corner_in_world_space );mxTEMP
		//const U32 sortKey = 0;//Rendering::Sorting::DepthToBits( distance_to_near_clipping_plane );

		// Get the camera position relative to the chunk's min corner and divided by the chunk's size.
		// see Proland documentation, 'Core module: 3.2.2. Continuous level of details': http://proland.inrialpes.fr
		//const V3f localCameraPosition = (terrainCamera - terrain_chunk.AabbMins) / terrain_chunk.AabbSize;

		Rendering::submitModelWithCustomTransform(
			*terrain_chunk.mesh_inst
			, local_to_world_matrix
			, scene_view
			, render_context
			, allowed_passes_mask
			);
	}//For each visible terrain chunk.

	//

	return ALL_OK;
}
#if 0

ERet renderVoxelTerrainChunks_DLoD_Planet(
	const Rendering::RenderCallbackParameters& parameters
	)
{
	tbPROFILE_FUNCTION;

	GL::NwRenderContext & render_context = parameters.render_context;

	//
	const VoxelTerrainRenderer* terrain = (VoxelTerrainRenderer*) parameters.user_data;

	//
	const Rendering::RrGlobalSettings& renderer_settings = parameters.renderer_settings;

	//
	Rendering::TbPassMaskT	allowed_passes_mask = ~0;

	//
	const bool	enable_debug_wireframe = renderer_settings.drawModelWireframes;
	//
	const Rendering::ScenePassInfo scene_pass_info = parameters.render_path.getPassInfo( Rendering::ScenePasses::DebugWireframe );
	SET_FLAGS( allowed_passes_mask, BIT(scene_pass_info.filter_index), enable_debug_wireframe );
	//
	if( enable_debug_wireframe )
	{
		Rendering::Material *	standard_material;
		Resources::Load( standard_material, MakeAssetID("material_voxel_terrain_dlod") );

		Rendering::Material *	uv_textured_material;
		Resources::Load( uv_textured_material, MakeAssetID("material_voxel_terrain_uv_textured") );

		//
		const Rendering::SolidWireSettings& settings = renderer_settings.wireframe;

		_SetSolidWireframeSettings( standard_material, settings );
		_SetSolidWireframeSettings( uv_textured_material, settings );
	}

	//
	const Rendering::NwCameraView& scene_view = parameters.camera;

	//const Rendering::NwFloatingOrigin	floating_origin = *parameters.floating_origin;

	//
	const VoxelTerrainChunk*const*const terrain_chunks = (const VoxelTerrainChunk**) parameters.entities._data;

	//
	for( int iChunk = 0; iChunk < parameters.entities._count; iChunk++ )
	{
		const VoxelTerrainChunk& terrain_chunk = *(terrain_chunks[ iChunk ]);

//		const RrVisibleObjectXForm& visible_xform = parameters.xforms[ iChunk ];
mxOPTIMIZE(":");
		// Scale from [0..1] to ChunkSize and translate to AABB Min Corner
		//const M44f local_to_world_matrix = M44_BuildTS(
		//	visible_xform.relative_aabb.min_corner,
		//	visible_xform.relative_aabb.size().x
		//	);

		const V3f	min_corner_pos_relative_to_floating_origin = floating_origin.getRelativePosition(
			terrain_chunk.seam_octree_bounds_in_world_space.min_corner
			);

		const M44f local_to_world_matrix = M44_BuildTS(
			min_corner_pos_relative_to_floating_origin + terrain_chunk.dbg_mesh_offset,
			terrain_chunk.seam_octree_bounds_in_world_space.side_length
			);

		//
		Rendering::submitModelWithCustomTransform(
			*terrain_chunk.mesh_inst
			, local_to_world_matrix
			, scene_view
			, render_context
			, allowed_passes_mask
			);

	}//For each visible terrain chunk.

	return ALL_OK;
}

ERet renderShadowDepthMap_DLoD(
	const Rendering::RenderCallbackParameters& parameters
	)
{
UNDONE;
//QQQ
return ALL_OK;
	tbPROFILE_FUNCTION;

//DBGOUT("%s(): cascade[%d]: render %d chunks, frame=%d"
//	   ,__FUNCTION__,parameters.shadowCascadeIndex, parameters.entities._count, GL::getNumRenderedFrames());

	const Rendering::NwCameraView& _sceneView = parameters.camera;
	const Rendering::RenderPath& render_path = parameters.render_path;
	GL::NwRenderContext & render_context = parameters.render_context;

	const VoxelTerrainRenderer* terrain = (VoxelTerrainRenderer*) parameters.user_data;

	const VoxelTerrainChunk*const*const terrain_chunks = (const VoxelTerrainChunk**) parameters.entities._data;
UNDONE;
	//
	const U32 renderPassIndex = 0;//render_path.getPassDrawingOrder( Rendering::ScenePasses::RenderDepthOnly ) + parameters.shadowCascadeIndex;

	// Load shaders.
	NwShaderEffect* shader = nil;
	// A null pixel shader is bound because only the depth is needed when rendering shadow maps.
	mxDO(Resources::Load( shader, MakeAssetID("voxel_terrain_dlod_depth") ));

	//
	GL::Cmd_Draw	batch;
	batch.reset();
	IF_DEBUG batch.src_loc = GFX_SRCFILE_STRING;

	// Update shader constants and bind shader inputs.
	{
		GL::NwPushCommandsOnExitingScope	applyShaderCommands(
			render_context,
			GL::buildSortKey( renderPassIndex, 2 )
			);

		const NwShaderEffect::Variant& defaultVariant = shader->getDefaultVariant();

		batch.states = defaultVariant.render_state;
		batch.program = defaultVariant.program_handle;

		mxDO(shader->pushShaderParameters( render_context._command_buffer ));
	}
UNDONE;
	//
//	mxDO(RenderVoxelTerrainChunks( parameters, terrainMaterial, renderPassIndex ));

	return ALL_OK;
}
#endif

ERet VoxelizeChunksOnGPU_DLoD(
	const Rendering::RenderCallbackParameters& parameters
	)
{
	const VoxelTerrainRenderer& voxel_renderer = *(VoxelTerrainRenderer*) parameters.user_data;

	const Rendering::RenderPath& render_path = Rendering::Globals::GetRenderPath();
	const NGpu::LayerID scene_pass_index = render_path.getPassDrawingOrder(
		Rendering::ScenePasses::VXGI_Voxelize + parameters.cascade_index
		);

	const Rendering::VXGI::VoxelGrids& vxgi = *voxel_renderer._vxgi;

	NGpu::NwRenderContext & render_context = parameters.render_context;

	//
	NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

	//
	NwShaderEffect* technique = nil;
	mxDO(Resources::Load( technique, MakeAssetID("vxgi_voxelize") ));

	//
	mxDO(technique->setUav(
		mxHASH_STR("rwsb_voxelGrid"),
		NGpu::AsOutput(vxgi.radiance_voxel_grid_buffer)
		));

	mxDO(technique->SetInput(
		nwNAMEHASH("t_sdf_atlas3d"),
		NGpu::AsInput(vxgi.brickmap->h_sdf_brick_atlas_tex3d)
		));

	//
	const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];
	const HProgram program_handle = pass0.default_program_handle;

	//
	NGpu::Cmd_Draw	draw_command;

	// Update shader constants and bind shader inputs.
	{
		NGpu::NwPushCommandsOnExitingScope	applyShaderCommands(
			render_context,
			NGpu::buildSortKey( scene_pass_index, NGpu::FIRST_SORT_KEY )	// first
			nwDBG_CMD_SRCFILE_STRING
			);
		draw_command.program = program_handle;

#if nwRENDERER_ENABLE_SHADOWS
		technique->SetInput(nwNAMEHASH("t_shadowDepthMap"), NGpu::AsInput(render_system._shadow_map_renderer._shadow_depth_map_handle) );
#endif
		technique->SetInput(nwNAMEHASH("t_diffuseMaps"),
			NGpu::AsInput(voxel_renderer.materials.hTerrainDiffuseTextures)
			);

		mxDO(technique->pushShaderParameters( render_context._command_buffer ));
	}

	//
	const VoxelTerrainChunk*const*const terrain_chunks = (const VoxelTerrainChunk**) parameters.entities._data;

//DBGOUT("%s(): render %d chunks",__FUNCTION__,parameters.entities._count);

	for( int iChunk = 0; iChunk < parameters.entities._count; iChunk++ )
	{
		const VoxelTerrainChunk& terrain_chunk = *(terrain_chunks[ iChunk ]);

		V3f	chunk_aabb_min_corner_in_world_space
			= V3f::fromXYZ(terrain_chunk.seam_octree_bounds_in_world_space.min_corner)
			;

#if MX_DEVELOPER
		chunk_aabb_min_corner_in_world_space += terrain_chunk.dbg_mesh_offset;
#endif

		//
		NGpu::NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			NGpu::buildSortKey( scene_pass_index, program_handle, NGpu::FIRST_SORT_KEY )
			nwDBG_CMD_SRCFILE_STRING
			);

IF_DEVELOPER NGpu::Dbg::GpuScope	dbgscope( render_context._command_buffer, RGBAi::GREEN, "Voxelize Chunk (cascade=%d)", parameters.cascade_index);

		{
			mxPREALIGN(16) struct Uniforms
			{
				M44f	u_local_to_world_space;
				M44f	u_local_to_voxel_grid_space_01;
				UInt4	u_voxelCascadeIndex;
			};

			Uniforms	*	uniforms;
			mxDO(technique->BindCBufferData(
				&uniforms
				, render_context._command_buffer
				));

			//
			// Scale from [0..1] to ChunkSize and translate to AABB Min Corner
			const M44f local_to_world_matrix = M44_BuildTS(
				chunk_aabb_min_corner_in_world_space
				, terrain_chunk.seam_octree_bounds_in_world_space.side_length
				);

			//
			const float inverseVoxelGridSizeWorld = 1.f / parameters.voxel_cascade_bounds.side_length;
			//const M44f worldToVoxelGrid01 = M44_BuildTS( -voxelGridMinWorld, CV3f(inverseVoxelGridSizeWorld) );
			const M44f worldToVoxelGrid01
				= M44_buildTranslationMatrix( -parameters.voxel_cascade_bounds.min_corner )
				* M44_uniformScaling( inverseVoxelGridSizeWorld )
				;

			//
			//const M44f voxelGrid01ToNDC = M44_BuildTS( CV3f(-0.5f), CV3f(2) );
			//
			uniforms->u_local_to_world_space = M44_Transpose( local_to_world_matrix );
			uniforms->u_local_to_voxel_grid_space_01 = M44_Transpose( local_to_world_matrix * worldToVoxelGrid01 );

			uniforms->u_voxelCascadeIndex = UInt4::replicate( parameters.cascade_index );
		}

//AABBf	chunkAabbWorld;
//chunkAabbWorld.mins = terrain_chunk.AabbMins;
//chunkAabbWorld.maxs = chunkAabbWorld.mins + CV3f(terrain_chunk.AabbSize);
//VX::g_DbgView->addBox( chunkAabbWorld, RGBAf::YELLOW );

		mxOPTIMIZE("remove indirection");
		//
		const Rendering::NwMesh& mesh = *terrain_chunk.mesh_inst->mesh;

		//
		draw_command.SetMeshState(
			Vertex_DLOD::metaClass().input_layout_handle
			, mesh.geometry_buffers.VB->buffer_handle
			, mesh.geometry_buffers.IB->buffer_handle
			, NwTopology::TriangleList
			, mesh.flags & Rendering::MESH_USES_32BIT_IB
			);

		draw_command.base_vertex = 0;
		draw_command.vertex_count = mesh.vertex_count;
		draw_command.start_index = 0;
		draw_command.index_count = mesh.index_count;

//strcpy_s(draw_command.dbgmsg,"vox chunk");
#if 0//MX_DEBUG
		struct Foo {
			static void dbgbreak()
			{
				int x = 0;
				int a = x;
			}
		};
		draw_command.debug_callback = Foo::dbgbreak;
#endif

		cmd_writer.SetRenderState(pass0.render_state);

		cmd_writer.Draw( draw_command );

	}//For each visible terrain chunk.

	return ALL_OK;
}

}//namespace



ERet VoxelTerrainRenderer::Initialize(
									  NGpu::NwRenderContext & context
									  , AllocatorI & scratch
									  , NwClump & storage

									  // optional
									  , const Rendering::VXGI::VoxelGrids* vxgi
									  )
{
	const size_t sizeOfTerrainChunk = sizeof(VoxelTerrainChunk);
	DBGOUT("Size of VoxelTerrainChunk = %u", sizeOfTerrainChunk);
	_chunkPool.Initialize( sizeOfTerrainChunk, 64 /*block size*/ );

	//
	mxDO(materials.load(
		context
		, scratch
		, storage
		));

	_vxgi = vxgi;

	//
	Rendering::RegisterCallbackForRenderingEntitiesOfType(
		Rendering::RE_VoxelTerrainChunk,
		&RenderVoxelTerrainChunks,
		this
		);
	//
	Rendering::VXGI::RegisterCallbackForVoxelizingEntitiesOfType(
		Rendering::RE_VoxelTerrainChunk,
		&VoxelizeChunksOnGPU_DLoD,
		this
		);

	return ALL_OK;
}

void VoxelTerrainRenderer::Shutdown()
{
	_chunkPool.releaseMemory();
}

ERet VoxelTerrainRenderer::DebugDrawWorldIfNeeded(
	const VX5::WorldI* world
	, VX5::DebugDrawI& debug_drawer
	, const VX5::WorldDebugDrawSettings& world_debug_draw_settings
	) const
{
	world->DebugDraw(
		debug_drawer
		, world_debug_draw_settings
		);
	return ALL_OK;
}
