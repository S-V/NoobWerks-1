// Renderer for Red Faction 1 level files (*.rfl).

// for std::sort()
#include <algorithm>

#include <Base/Base.h>

#include <Graphics/Public/graphics_shader_system.h>	// NwShaderEffect

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Scene/CameraView.h>
#include <Rendering/Private/RenderSystemData.h>

#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Common.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_LevelRenderer.h>


#define LOAD_VBM_FILES	(1)


namespace RedFaction
{

ERet loadMaterialTexture(
						 RFL_Material & material
						 , NwClump * asset_storage
						 )
{
	const char* filename = AssetId_ToChars( material.texture._id );

#if !LOAD_VBM_FILES

	if( Str::HasExtensionS( filename, "vbm" ) )
	{
		ptWARN( "cannot load texture '%s'", filename );

		mxDO(Resources::Load(
			material.texture._ptr,
			MakeAssetID("missing_texture"),
			asset_storage
			));

		return ALL_OK;
	}

#endif // !LOAD_VBM_FILES

	mxDO(material.texture.Load( asset_storage ));

	return ALL_OK;
}

ERet loadLightmapTexture(
						 RFL_Lightmap & lightmap
						 , NwClump * asset_storage
						 )
{
	NwTexture2DDescription textureDesc;
	textureDesc.format = NwDataFormat::RGBA8;
	textureDesc.width = lightmap.texture_width;
	textureDesc.height = lightmap.texture_height;
	textureDesc.numMips = 1;

	lightmap.texture_handle = NGpu::createTexture2D(
		textureDesc
		, NGpu::copy( lightmap.texture_data.raw(), lightmap.texture_data.rawSize() )
		);

	return ALL_OK;
}

V3f computeFaceNormal(
					  const AuxVertex* vertices
					  , const UINT num_face_vertices
					  )
{
	CV3f	center(0);

	for( UINT i = 0; i < num_face_vertices; i++ )
	{
		center += vertices[ i ].xyz;
	}
	center *= float(1) / num_face_vertices;

	//
	CV3f	normal(0);

	V3f prev = vertices[ num_face_vertices - 1 ].xyz;	// last

	for( UINT i = 0; i < num_face_vertices; i++ )
	{
		const V3f	curr = vertices[ i ].xyz;
		const V3f	relV0 = prev - center;
		const V3f	relV1 = curr - center;
		normal += relV1 ^ relV0;
		prev = curr;
	}

	normal.normalize();

	return normal;
}

typedef U32 VertIndexT;

struct DrawCallBatcher
{
	const Rendering::NwCameraView& camera_view;

	int iCurrentTexture;
#if RF_RENDERER_SUPPORT_LIGHTMAPS
	int iCurrentLightmap;
#endif // RF_RENDERER_SUPPORT_LIGHTMAPS

	U32	num_draw_calls;

	DynamicArray< AuxVertex >	batched_vertices;
	DynamicArray< VertIndexT >	batched_indices;

public:
	DrawCallBatcher(
		const Rendering::NwCameraView& camera_view
		, AllocatorI & allocator = MemoryHeaps::temporary()
		)
		: camera_view( camera_view )
		, batched_vertices( allocator )
		, batched_indices( allocator )
	{
		iCurrentTexture = -1;
#if RF_RENDERER_SUPPORT_LIGHTMAPS
		iCurrentLightmap = -1;
#endif
		num_draw_calls = 0;
	}

	ERet drawFace(
		const RFL_Face& face
		, RFL_LevelGeo& geo
		, RFL_Level & level
		, NwShaderEffect& shader_effect
		, NwClump * asset_storage
		)
	{
		bool needToFlush = (iCurrentTexture != face.iTexture)
#if RF_RENDERER_SUPPORT_LIGHTMAPS
			|| (iCurrentLightmap != face.iLightmap)
#endif
			;
		if( needToFlush )
		{
			this->flush( shader_effect, geo, level, asset_storage );

			iCurrentTexture = face.iTexture;
#if RF_RENDERER_SUPPORT_LIGHTMAPS
			iCurrentLightmap = face.iLightmap;
#endif
		}

		// Draw the face as a triangle fan.

		const UINT num_face_vertices = face.vertices.num();
		const UINT num_face_triangles = num_face_vertices - 2;	
		const UINT num_face_indices = num_face_triangles * 3;

		//
		const UINT old_vertex_count = batched_vertices.num();
		mxDO(batched_vertices.setNum( old_vertex_count + num_face_vertices ));

		const UINT old_index_count = batched_indices.num();
		mxDO(batched_indices.setNum( old_index_count + num_face_indices ));

		//
		AuxVertex	*dst_face_vertices = batched_vertices.raw() + old_vertex_count;

		for( U32 iFaceVertex = 0; iFaceVertex < num_face_vertices; iFaceVertex++ )
		{
			const RFL_FaceVertex& faceVertex = face.vertices[ iFaceVertex ];
			AuxVertex & vertex = dst_face_vertices[ iFaceVertex ];

			vertex.xyz = RF1::toMyVec3( geo.positions[ faceVertex.idx ] );

			vertex.uv = V2_To_Half2(V2f( faceVertex.u, faceVertex.v ));
			vertex.uv2 = V2_To_Half2(V2f( faceVertex.lm_u, faceVertex.lm_v ));

			vertex.rgba.v = ~0;
		}

		//
		const V3f face_normal = computeFaceNormal( dst_face_vertices, num_face_vertices );

		for( U32 iFaceVertex = 0; iFaceVertex < num_face_vertices; iFaceVertex++ )
		{
			AuxVertex & vertex = dst_face_vertices[ iFaceVertex ];
			vertex.N = PackNormal( face_normal );
		}

		//
		VertIndexT	*dst_face_indices = batched_indices.raw() + old_index_count;

		// must reverse winding when displaying in our engine
		for( UINT iFaceTri = 0; iFaceTri < num_face_triangles; iFaceTri++ )
		{
			dst_face_indices[ iFaceTri*3 + 0 ] = old_vertex_count + iFaceTri + 2;
			dst_face_indices[ iFaceTri*3 + 1 ] = old_vertex_count + iFaceTri + 1;
			dst_face_indices[ iFaceTri*3 + 2 ] = old_vertex_count + 0;
		}

		return ALL_OK;
	}

	///
	ERet flush(
		NwShaderEffect& shader_effect
		, RFL_LevelGeo& geo
		, RFL_Level & level
		, NwClump * asset_storage
		)
	{
		if( batched_vertices.IsEmpty() || batched_indices.IsEmpty() ) {
			return ALL_OK;
		}
		
		const Rendering::RenderPath& render_path = Rendering::Globals::GetRenderPath();

		NGpu::NwRenderContext & render_context = NGpu::getMainRenderContext();

		//
		NwTransientBuffer	vertex_buffer;
		NwTransientBuffer	index_buffer;

		mxDO(NGpu::AllocateTransientBuffers(
			vertex_buffer, batched_vertices.num(), sizeof(batched_vertices[0]),
			index_buffer, batched_indices.num(), sizeof(batched_indices[0])
			));

		memcpy( vertex_buffer.data, batched_vertices.raw(), batched_vertices.rawSize() );

		memcpy( index_buffer.data, batched_indices.raw(), batched_indices.rawSize() );


		//
		RFL_Material& material = geo.materials[ iCurrentTexture ];

		//
		const U32 renderPassIndex = render_path.getPassDrawingOrder(
			material.is_translucent ? Rendering::ScenePasses::Deferred_Translucent : Rendering::ScenePasses::FillGBuffer
			);

		const NwShaderEffect::Pass* passes = shader_effect.getPasses()._data;
		const NwShaderEffect::Pass& pass = material.is_translucent ? passes[1] : passes[0];

		//
		{
			NGpu::NwPushCommandsOnExitingScope	applyShaderCommands(
				render_context,
				NGpu::buildSortKey( renderPassIndex, NGpu::FIRST_SORT_KEY )
				nwDBG_CMD_SRCFILE_STRING
				);

			//
			NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

			//
#if MX_DEVELOPER
			NGpu::Dbg::GpuScope	perfScope(
				render_context._command_buffer
				, RGBAf::ORANGE.ToRGBAi().u
				, "RF static geo: %s", AssetId_ToChars( material.texture._id )
				);
#endif // MX_DEVELOPER

			//
			const M44f	world_matrix = M44_BuildTS(CV3f(-100, -100, -100), 1);
			const M44f	world_view_matrix = world_matrix * camera_view.view_matrix;
			const M44f	world_view_projection_matrix = world_view_matrix * camera_view.projection_matrix;

			const M44f	world_view_matrix_T = M44_Transpose(world_view_matrix);
			const M44f	world_view_projection_matrix_T = M44_Transpose(world_view_projection_matrix);

			mxDO(shader_effect.setUniform( mxHASH_STR("world_view_matrix"), &world_view_matrix_T ));
			mxDO(shader_effect.setUniform( mxHASH_STR("world_view_projection_matrix"), &world_view_projection_matrix_T ));

			//
			if( material.texture.IsValid() )
			{
				const NwTexture& diffuse_texture = *material.texture;
				mxDO(shader_effect.SetInput(
					nwNAMEHASH("t_diffuse"),
					diffuse_texture.m_resource
					));
			}

			//
#if RF_RENDERER_SUPPORT_LIGHTMAPS
			if( iCurrentLightmap != -1 )
			{
				const U32 iLightmap = geo.lightmap_vertices[ iCurrentLightmap ];
				RFL_Lightmap & lightmap = level.lightmaps[ iLightmap ];
				if( lightmap.texture_handle.IsNil() ) {
					mxDO(loadLightmapTexture( lightmap, asset_storage ));
				}
				mxDO(shader_effect.setResource(
					mxHASH_STR("t_lightmap"),
					NGpu::AsInput( lightmap.texture_handle )
					));
			}
#endif

			//
			shader_effect.pushAllCommandsInto( render_context._command_buffer );


			//
			NGpu::Cmd_Draw	batch;
			{
				batch.program = pass.default_program_handle;

				batch.SetMeshState(
					AuxVertex_vertex_format.input_layout_handle,
					vertex_buffer.buffer_handle,
					index_buffer.buffer_handle,
					NwTopology::TriangleList,
					sizeof(batched_indices[0]) == sizeof(U32)
					);
				batch.base_vertex = vertex_buffer.base_index;
				batch.vertex_count = batched_vertices.num();
				batch.start_index = index_buffer.base_index;
				batch.index_count = batched_indices.num();
			}
			IF_DEBUG batch.src_loc = GFX_SRCFILE_STRING;
			cmd_writer.SetRenderState(pass.render_state);
			cmd_writer.Draw( batch );
		}//

		//
		++num_draw_calls;

		//
		batched_vertices.RemoveAll();
		batched_indices.RemoveAll();

		return ALL_OK;
	}
};

ERet drawLevelGeo(
	RFL_LevelGeo& geo
	, RFL_Level & level
	, NwShaderEffect& shader_effect
	, DrawCallBatcher& batcher
	, NwClump * asset_storage
	)
{
	mxASSERT2(
		!geo.sorted_face_indices.IsEmpty(),
		"Did you forget to call recalcRenderingData()?"
		);

	//
	for( U32 i = 0; i < geo.sorted_face_indices.num(); i++ )
	{
		const UINT iFace = geo.sorted_face_indices[ i ];
		const RFL_Face& face = geo.faces[ iFace ];
		const RFL_Room& room = geo.rooms[ face.iRoom ];

		const V3f room_center = room.bounds.center();

		RFL_Material& material = geo.materials[ face.iTexture ];

		if( material.texture.IsNil() ) {
			mxDO(loadMaterialTexture( material, asset_storage ));
		}

//if(pRoom->IsDetail() && pMaterial && pMaterial->HasAlpha())
//    pIrrBuf->Material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;

		mxDO(batcher.drawFace( face, geo, level, shader_effect, asset_storage ));
	}

	return ALL_OK;
}

ERet drawLevel(
				RFL_Level & level
				, const Rendering::NwCameraView& camera_view
				, NwClump * asset_storage
				, UINT *num_draw_calls_
				)
{
	//
	NwShaderEffect* shader_effect;
	mxDO(Resources::Load( shader_effect, MakeAssetID("redfaction_level"), asset_storage ));

	//
	DrawCallBatcher	batcher( camera_view );

	// reduce reallocations
	mxDO(batcher.batched_vertices.reserve(1<<13));
	mxDO(batcher.batched_indices.reserve(1<<13));


	//
	for( UINT iGeo = 0; iGeo < level.chunks.num(); iGeo++ )
	{
		RFL_LevelGeo & geo = level.chunks[iGeo];
		drawLevelGeo( geo, level, *shader_effect, batcher, asset_storage );

		if( iGeo == level.chunks.num()-1 ) {
			mxDO(batcher.flush( *shader_effect, geo, level, asset_storage ));
		}
	}

	*num_draw_calls_ = batcher.num_draw_calls;

	return ALL_OK;
}

}//namespace RedFaction
