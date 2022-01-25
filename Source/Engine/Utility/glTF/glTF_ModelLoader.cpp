// glTF model loader for displaying objects with PBR materials.
// based on https://github.com/google/filament/blob/master/libs/gltfio/src/ResourceLoader.cpp
//
// TODO: mesh sharing with transform hierarchy is not implemented -
// - meshes will be pre-transformed and duplicated.
// - recursive node transforms not finished
#include <Base/Base.h>
#pragma hdrstop

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#include <Base/Template/Containers/Blob.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/ObjectModel/Clump.h>

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/Mesh.h>

#include <Developer/Mesh/MeshImporter.h>

#include <Utility/glTF/glTF_ModelLoader.h>


#if MX_DEVELOPER

namespace glTF
{
	static AllocatorI& tempMemoryHeap()
	{
		return MemoryHeaps::temporary();
	}

	struct cgltf_memory_callbacks
	{
		static void* allocFun(void* user, cgltf_size size)
		{
			return tempMemoryHeap().Allocate( size, DEFAULT_MEMORY_ALIGNMENT );
		}
		static void freeFun(void* user, void* ptr)
		{
			return tempMemoryHeap().Deallocate( ptr );
		}
	};

	struct cgltf_file_callbacks
	{
		static cgltf_result read(
			const struct cgltf_memory_options* memory_options,
			const struct cgltf_file_options* file_options,
			const char* path,
			cgltf_size* size,
			void** data
			)
		{
			ptPRINT( "Opening file '%s'...", path );

			//
			FileReader	file;
			mxENSURE( file.Open( path ) == ALL_OK,
				cgltf_result_file_not_found,
				"failed to open '%s'", path
				);

			const cgltf_size file_size = file.Length();

			//
			void* file_contents = memory_options->alloc( memory_options->user_data, file_size );
			mxENSURE( file_contents != nil,
				cgltf_result_out_of_memory,
				"failed to alloc %lu bytes", file_size
				);

			//
			mxENSURE( file.Read( file_contents, file_size ) == ALL_OK,
				cgltf_result_io_error,
				"failed to read %lu bytes", file_size
				);

			//
			*size = file_size;
			*data = file_contents;

			return cgltf_result_success;
		}

		static void release(
			const struct cgltf_memory_options* memory_options,
			const struct cgltf_file_options* file_options,
			void* data
			)
		{
			memory_options->free( memory_options->user_data, data );
		}
	};

	typedef nonstd::optional<M44f> OptionalMatrix4x4f;

	static
	OptionalMatrix4x4f getNodeTransform( const cgltf_node& node )
	{
		if( node.has_scale || node.has_rotation || node.has_translation )
		{
			M44f xform;
			memcpy( &xform, node.matrix, sizeof(xform) );

			return OptionalMatrix4x4f( xform );
		}

		return OptionalMatrix4x4f();
	}

	///
	const char* cgltf_primitive_type_to_chars( const cgltf_primitive_type primitive_type )
	{
		switch( primitive_type )
		{
		case cgltf_primitive_type_points:
			return "type_points";
		
		case cgltf_primitive_type_lines:
			return "type_lines";
		
		case cgltf_primitive_type_line_loop:
			return "line_loop";
		
		case cgltf_primitive_type_line_strip:
			return "line_strip";

		case cgltf_primitive_type_triangles:
			return "triangles";

		case cgltf_primitive_type_triangle_strip:
			return "triangle_strip";

		case cgltf_primitive_type_triangle_fan:
			return "triangle_fan";

			mxDEFAULT_UNREACHABLE(return "unknown");
		}
	}

	///
	const char* cgltf_component_type_to_chars( const cgltf_component_type component_type )
	{
		switch( component_type )
		{
		case cgltf_component_type_invalid:
			return "invalid";

		case cgltf_component_type_r_8: /* BYTE */
			return "r_8";

		case cgltf_component_type_r_8u: /* UNSIGNED_BYTE */
			return "r_8u";

		case cgltf_component_type_r_16: /* SHORT */
			return "r_16";

		case cgltf_component_type_r_16u: /* UNSIGNED_SHORT */
			return "r_16u";

		case cgltf_component_type_r_32u: /* UNSIGNED_INT */
			return "r_32u";

		case cgltf_component_type_r_32f: /* FLOAT */
			return "r_32f";

			mxDEFAULT_UNREACHABLE(return "unknown");
		}
	}

	///
	const char* cgltf_attribute_type_to_chars( const cgltf_attribute_type attribute_type )
	{
		switch( attribute_type )
		{
		case cgltf_attribute_type_invalid:
			return "invalid";

		case cgltf_attribute_type_position:
			return "position";

		case cgltf_attribute_type_normal:
			return "normal";

		case cgltf_attribute_type_tangent:
			return "tangent";

		case cgltf_attribute_type_texcoord:
			return "texcoord";

		case cgltf_attribute_type_color:
			return "color";

		case cgltf_attribute_type_joints:
			return "joints";

		case cgltf_attribute_type_weights:
			return "weights";

			mxDEFAULT_UNREACHABLE(return "unknown");
		}
	}

	///
	static
	ERet copyTextureView(
		Tb_TextureView &dst_texture_view_
		, const cgltf_texture_view& src_texture_view
		, const cgltf_options& options
		, const String& folder
		)
	{
		if( !src_texture_view.texture ) {
			return ALL_OK;
		}

		const cgltf_image& src_image = *src_texture_view.texture->image;

		const char* image_name = src_image.name ? src_image.name : "<nil>";

		const bool isBase64 = strstr( src_image.uri, "base64," ) != nil;
		const char* image_uri_name = isBase64 ? "<base64>" : src_image.uri;

		//
		if( src_image.buffer_view && src_image.buffer_view->buffer )
		{
			DEVOUT("[glTF]: Loading texture '%s' from buffer...",
				image_name
				);

			const cgltf_buffer& src_image_buffer = *src_image.buffer_view->buffer;
			//
			UNDONE;
		}
		else
		{
			if( isBase64 )
			{
				DEVOUT("[glTF]: Loading texture '%s' embedded as base64...",
					image_uri_name
					);

				UNDONE;
			}
			else
			{
				DEVOUT("[glTF]: Loading texture '%s' from external file...",
					image_uri_name
					);

				//
				FilePathStringT	file_path;
				mxDO(Str::ComposeFilePath( file_path, folder.c_str(), src_image.uri ));

				//
				cgltf_size	image_size;
				void *		image_data;

				const cgltf_result result = options.file.read(
					&options.memory,
					&options.file,
					file_path.c_str(),
					&image_size,
					&image_data
					);
				mxENSURE(
					result == cgltf_result_success
					, ERR_FAILED_TO_LOAD_DATA
					, "couldn't load image file '%s'", src_image.uri
					);

				//
				mxDO(dst_texture_view_.data.setNum( image_size ));
				memcpy( dst_texture_view_.data.raw(), image_data, image_size );

				options.file.release(
					&options.memory,
					&options.file,
					image_data
					);

				//
				dst_texture_view_.id = make_AssetID_from_raw_string( src_image.uri );
				mxASSERT(AssetId_IsValid(dst_texture_view_.id));
			}
		}

		return ALL_OK;
	}

	static
	ERet copyMaterial(
		TcMaterial &dst_material_
		, const cgltf_material& src_material
		, const cgltf_options& options
		, const String& folder
		)
	{
		if( src_material.name ) {
			dst_material_.name = make_AssetID_from_raw_string( src_material.name );
		}

		if( src_material.has_pbr_metallic_roughness )
		{
			const cgltf_pbr_metallic_roughness& src_pbr_metallic_roughness = src_material.pbr_metallic_roughness;

			dst_material_.pbr_metallic_roughness = Tb_pbr_metallic_roughness();
			Tb_pbr_metallic_roughness &dst_pbr_metallic_roughness = *dst_material_.pbr_metallic_roughness;

			//
			copyTextureView(
				dst_pbr_metallic_roughness.base_color_texture,
				src_pbr_metallic_roughness.base_color_texture,
				options,
				folder
				);
			copyTextureView(
				dst_pbr_metallic_roughness.metallic_roughness_texture,
				src_pbr_metallic_roughness.metallic_roughness_texture,
				options,
				folder
				);

			TCopyStaticArray( dst_pbr_metallic_roughness.base_color_factor, src_pbr_metallic_roughness.base_color_factor );
			dst_pbr_metallic_roughness.metallic_factor = src_pbr_metallic_roughness.metallic_factor;
			dst_pbr_metallic_roughness.roughness_factor = src_pbr_metallic_roughness.roughness_factor;
		}

		if( src_material.has_pbr_specular_glossiness )
		{
			UNDONE;
		}

		if( src_material.has_clearcoat )
		{
			UNDONE;
		}

		copyTextureView(
			dst_material_.normal_texture, src_material.normal_texture,
			options,
			folder
			);
		copyTextureView(
			dst_material_.occlusion_texture, src_material.occlusion_texture,
			options,
			folder
			);

		copyTextureView(
			dst_material_.emissive_texture, src_material.emissive_texture,
			options,
			folder
			);

		return ALL_OK;
	}

	static
	ERet addMesh(
		TcModel & model_
		, const String& folder
		, const cgltf_mesh& mesh
		, const cgltf_options& options
		, const OptionalMatrix4x4f& global_transform
		, TArray< TRefPtr< TcMaterial > > &submesh_materials_ // for debug rendering
		)
	{
		DEVOUT("[glTF] Add mesh [%lu]: '%s': %lu primitives",
			model_.meshes.num(), mesh.name, mesh.primitives_count
			);

		//
		for( size_t iPrim = 0; iPrim < mesh.primitives_count; iPrim++ )
		{
			const cgltf_primitive& prim = mesh.primitives[ iPrim ];

			const cgltf_material* material = prim.material;

			const cgltf_accessor* indices = prim.indices;


			//
			String128	indices_info;
			if( indices ) {
				Str::Format(
					indices_info,
					"indices: type='%s', count=%lu",
					cgltf_component_type_to_chars(indices->component_type), indices->count
					);
			} else {
				Str::Format(
					indices_info,
					"no indices"
					);
			}

			DEVOUT("\t Prim [%lu]: material='%s', type='%s', attribs_count=%lu; %s",
				iPrim, material->name, cgltf_primitive_type_to_chars( prim.type ), prim.attributes_count, indices_info.c_str()
				);

			mxENSURE( prim.type == cgltf_primitive_type_triangles,
				ERR_UNSUPPORTED_FEATURE,
				"Only triangle lists are supported! (got '%s')", cgltf_primitive_type_to_chars( prim.type )
				);


			//
			TcTriMesh * new_mesh = new TcTriMesh( model_.allocator );
			mxDO(model_.meshes.add( new_mesh ));


			if( material->name ) {
				Str::CopyS( new_mesh->name, prim.material->name );
			}


			// Copy material data.
			{
				TRefPtr< TcMaterial >	new_material = new TcMaterial();
				mxDO(copyMaterial(
					*new_material,
					*material,
					options,
					folder
					));

				new_mesh->material = new_material;

				mxDO(submesh_materials_.add( new_material ));
			}


			// Copy vertex data.

			//
			UINT num_prim_vertices = 0;

			mxASSERT(prim.attributes_count > 0);
			for( size_t iAttrib = 0; iAttrib < prim.attributes_count; iAttrib++ )
			{
				const cgltf_attribute& attrib = prim.attributes[ iAttrib ];

				const cgltf_accessor* attrib_data = attrib.data;

				DEVOUT("\t\t Attrib [%lu]: '%s', type='%s', index=%lu, stride=%lu, count=%lu",
					iAttrib, attrib.name, cgltf_attribute_type_to_chars( attrib.type ), attrib.index, attrib_data->stride, attrib_data->count
					);
				if( num_prim_vertices ) {
					mxENSURE( num_prim_vertices == attrib_data->count, ERR_FAILED_TO_PARSE_DATA, "Vertex count differs among attributes!" );
				} else {
					num_prim_vertices = attrib_data->count;
				}
			}//for each attribute

			//
			for( size_t iAttrib = 0; iAttrib < prim.attributes_count; iAttrib++ )
			{
				const cgltf_attribute& attrib = prim.attributes[ iAttrib ];

				const cgltf_accessor *		attrib_data_accessor = attrib.data;
				const cgltf_buffer_view *	buffer_view = attrib_data_accessor->buffer_view;
				const cgltf_buffer *		buffer = buffer_view->buffer;

				mxASSERT(attrib_data_accessor->count == num_prim_vertices);

				//
				void *	buffer_data = buffer->data;

				//
				const cgltf_size num_float_components = cgltf_num_components( attrib_data_accessor->type );

				const cgltf_size attrib_floats_count = attrib_data_accessor->count * num_float_components;
				const cgltf_size attrib_data_size = attrib_floats_count * sizeof(float);

				float* attrib_floats_data = (float*) options.memory.alloc( options.memory.user_data, attrib_data_size );
				mxENSURE( attrib_floats_data, ERR_OUT_OF_MEMORY, "failed to alloc %lu bytes", attrib_data_size );

				const cgltf_size unpacked_floats_count = cgltf_accessor_unpack_floats( attrib_data_accessor, attrib_floats_data, attrib_floats_count );
				mxASSERT(unpacked_floats_count == attrib_floats_count);

				//
				switch( attrib.type )
				{
				case cgltf_attribute_type_position:
					{
						const V3f* positions = (V3f*) attrib_floats_data;
						mxDO(Arrays::setFrom( new_mesh->positions, positions, num_prim_vertices ));
					}
					break;

				case cgltf_attribute_type_normal:
					{
						const V3f* normals = (V3f*) attrib_floats_data;
						mxDO(Arrays::setFrom( new_mesh->normals, normals, num_prim_vertices ));
					}
					break;

				case cgltf_attribute_type_tangent:
					{
						const V3f* tangents = (V3f*) attrib_floats_data;
						mxDO(Arrays::setFrom( new_mesh->tangents, tangents, num_prim_vertices ));
					}
					break;

				case cgltf_attribute_type_texcoord:
					{
						const V2f* texCoords = (V2f*) attrib_floats_data;
						mxDO(Arrays::setFrom( new_mesh->texCoords, texCoords, num_prim_vertices ));
					}
					break;

					//case cgltf_attribute_type_color:
					//	return "color";

					//case cgltf_attribute_type_joints:
					//	return "joints";

					//case cgltf_attribute_type_weights:
					//	return "weights";

				default:
					DEVOUT(
						"glTF: unknown attrib type: '%s'",
						cgltf_attribute_type_to_chars(attrib.type)
						);
					UNDONE;
					continue;
				}

			}//for each attribute


			// Pre-transform vertex data.
			if( global_transform.has_value() )
			{
				// make an 16-byte aligned copy, because nonstd::optional<> returns an unaligned pointer
				M44f aligned_matrix = *global_transform;
				new_mesh->transformVerticesBy( aligned_matrix );
			}


			// Copy index data.

			if( indices )
			{
				mxASSERT( prim.type == cgltf_primitive_type_triangles );

				const UINT num_prim_indices = indices->count;
				const UINT num_triangles = num_prim_indices / 3;

				mxDO(new_mesh->indices.setNum( num_prim_indices ));

				for( cgltf_size ii = 0; ii < num_prim_indices; ii++ )
				{
					new_mesh->indices[ii] = cgltf_accessor_read_index( indices, ii );
				}
			}
			else
			{
				UNDONE;
			}

		}//for each primitive
		return ALL_OK;
	}

	///
	static
	ERet addMeshesFromNode_Recursive(
		TcModel & model_
		, const String& folder
		, const cgltf_node& node
		, const cgltf_options& options
		, const OptionalMatrix4x4f& global_transform
		, TArray< TRefPtr< TcMaterial > > &submesh_materials_ // for debug rendering
		)
	{
		OptionalMatrix4x4f node_transform = getNodeTransform( node );

		if( node.mesh )
		{
			mxDO(addMesh(
				model_
				, folder
				, *node.mesh
				, options
				, node_transform
				, submesh_materials_
				));
		}

		//
		for( size_t iChildNode = 0; iChildNode < node.children_count; iChildNode++ )
		{
			const cgltf_node* child_node = node.children[ iChildNode ];

			mxDO(addMeshesFromNode_Recursive(
				model_
				, folder
				, *child_node
				, options
				, node_transform
				, submesh_materials_
				));
		}//for each mesh

		return ALL_OK;
	}


	///
	static
	ERet buildModelFromParsedData(
		TcModel & model_
		, const String& folder
		, const cgltf_data* data
		, const cgltf_options& options
		, TArray< TRefPtr< TcMaterial > > &submesh_materials_ // for debug rendering
		)
	{
		DEVOUT("%lu meshes", data->meshes_count);

#if 0

		for( size_t iMesh = 0; iMesh < data->meshes_count; iMesh++ )
		{
			const cgltf_mesh& mesh = data->meshes[ iMesh ];

			mxDO(addMesh(
				model_
				, folder
				, mesh
				, options
				, submesh_materials_
				));
		}//for each mesh

#else
		
		//
		for( size_t iRootNode = 0; iRootNode < data->scene->nodes_count; iRootNode++ )
		{
			const cgltf_node* root_node = data->scene->nodes[ iRootNode ];

			OptionalMatrix4x4f node_transform = getNodeTransform( *root_node );

			mxDO(addMeshesFromNode_Recursive(
				model_
				, folder
				, *root_node
				, options
				, node_transform
				, submesh_materials_
				));
		}//for each mesh

#endif
		return ALL_OK;
	}

	///
	ERet loadModelFromFile(
		TcModel *model_
		, const char* gltf_filepath
		, NwClump * storage
		, TArray< TRefPtr< TcMaterial > > &submesh_materials_ // for debug rendering
		)
	{
		// https://github.com/jkuhlmann/cgltf

		cgltf_options gltf_options;
		mxZERO_OUT(gltf_options);
		{
			gltf_options.type = cgltf_file_type_gltf;

			gltf_options.memory.alloc = &cgltf_memory_callbacks::allocFun;
			gltf_options.memory.free = &cgltf_memory_callbacks::freeFun;

			gltf_options.file.read = &cgltf_file_callbacks::read;
			gltf_options.file.release = &cgltf_file_callbacks::release;
		}


		//
		cgltf_data* gltf_data = NULL;


		//
		{
			const cgltf_result result = cgltf_parse_file(
				&gltf_options,
				gltf_filepath,
				&gltf_data
				);

			mxENSURE(
				result == cgltf_result_success
				, ERR_FAILED_TO_LOAD_DATA
				, "cgltf_parse_file() failed with %d", result
				);
		}

	
		//
		class CleanupOnScopeExit: NonCopyable {
			cgltf_data* _data;
		public:
			CleanupOnScopeExit( cgltf_data* data )
				: _data( data ) {
			}
			~CleanupOnScopeExit() {
				if( _data ) {
					cgltf_free(_data);
				}
			}
		} cleanupOnScopeExit( gltf_data );



		//

#if MX_DEBUG || MX_DEVELOPER
		{
			const cgltf_result result = cgltf_validate( gltf_data );

			mxENSURE(
				result == cgltf_result_success
				, ERR_VALIDATION_FAILED
				, "cgltf_validate() failed with %d", result
				);
		}
#endif

		// Read data from the file system and base64 URIs.
		{
			const cgltf_result result = cgltf_load_buffers(
				&gltf_options,
				gltf_data,
				gltf_filepath
				);

			mxENSURE(
				result == cgltf_result_success
				, ERR_FAILED_TO_LOAD_DATA
				, "cgltf_load_buffers() failed with %d", result
				);
		}


		//
		String256	folder;
		mxDO(Str::CopyS( folder, gltf_filepath ));
		Str::StripFileName( folder );

		mxDO(buildModelFromParsedData(
			*model_,
			folder,
			gltf_data,
			gltf_options,
			submesh_materials_
			));

		return ALL_OK;
	}

	ERet createMeshFromFile(
		Rendering::NwMesh **new_mesh_
		, const char* gltf_filepath
		, NwClump * storage
		, TArray< TRefPtr< TcMaterial > > &submesh_materials_ // for debug rendering
		)
	{
		AllocatorI & temp_allocator = MemoryHeaps::temporary();

		TcModel	model( temp_allocator );

		mxDO(loadModelFromFile(
			&model
			, gltf_filepath
			, storage
			, submesh_materials_ // for debug rendering
		));

		//
		const TbVertexFormat& vertex_format = DrawVertex::metaClass();

		TbRawMeshData	rawMesh;
		mxDO(MeshLib::CompileMesh( model, vertex_format, rawMesh ));

		//
		NwBlob	compiled_mesh_data_blob( temp_allocator );
		mxDO(Rendering::NwMesh::CompileMesh(
			NwBlobWriter(compiled_mesh_data_blob)
			, rawMesh
			, temp_allocator
			));

		//
		Rendering::NwMesh *	new_mesh;
		mxDO(storage->New( new_mesh ));

		mxDO(Rendering::NwMesh::LoadFromStream(
			*new_mesh,
			NwBlobReader(compiled_mesh_data_blob)
			));

		//
		*new_mesh_ = new_mesh;

		return ALL_OK;
	}

}//namespace glTF

#endif // MX_DEVELOPER
