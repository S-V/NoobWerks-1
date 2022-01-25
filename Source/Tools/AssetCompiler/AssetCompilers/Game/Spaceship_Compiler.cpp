#include "stdafx.h"
#pragma hdrstop


#if 1//SG_USE_SDF_VOLUMES
#include <SDFGen/makelevelset3.h>
#pragma comment(lib, "SDFGen")
#endif // SG_USE_SDF_VOLUMES


#include <meshoptimizer/src/meshoptimizer.h>

#include <Core/Serialization/Serialization.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include <Engine/Model.h>

#include <Developer/Mesh/MeshImporter.h>

#include <Utility/MeshLib/Simplification.h>

#include <AssetCompiler/AssetCompilers/Graphics/MeshCompiler.h>
#include <AssetCompiler/AssetCompilers/Game/Spaceship_Compiler.h>


#include <../Games/SpaceGame/rendering/lod_models.h>
#include <../Games/SpaceGame/rendering/sdf_volume.h>



namespace AssetBaking
{

mxDEFINE_CLASS( SpaceshipBuildSettings );
mxBEGIN_REFLECTION( SpaceshipBuildSettings )
	mxMEMBER_FIELD( rotate ),
	mxMEMBER_FIELD( scale ),
	mxMEMBER_FIELD( flip_winding_order ),
mxEND_REFLECTION
SpaceshipBuildSettings::SpaceshipBuildSettings()
{
	rotate = M44_Identity();
	scale = 1;
	flip_winding_order = false;
}



mxDEFINE_CLASS(Spaceship_Compiler);

const TbMetaClass* Spaceship_Compiler::getOutputAssetClass() const
{
	return &Rendering::NwMesh::metaClass();
}


static
ERet CopyMeshData(
				  const TcModel& imported_model
				  , std::vector<Vec3ui>	&triangles_
				  , std::vector<Vec3f>	&vertex_positions_
				  )
{
	mxENSURE(imported_model.meshes.num() == 1
		, ERR_INVALID_PARAMETER
		, "only single src_mesh is supported"
		);
	const TcTriMesh& src_mesh = *imported_model.meshes[0];

	mxASSERT(src_mesh.indices._count % 3 == 0);
	const UINT num_triangles = src_mesh.indices._count / 3;
	triangles_.resize(num_triangles);

	vertex_positions_.resize(src_mesh.positions._count);

	//
	for( UINT tri_idx = 0; tri_idx < num_triangles; tri_idx++ )
	{
		Vec3ui &dst_tri = triangles_[ tri_idx ];

		dst_tri.v[0] = src_mesh.indices[ tri_idx*3 + 0 ];
		dst_tri.v[1] = src_mesh.indices[ tri_idx*3 + 1 ];
		dst_tri.v[2] = src_mesh.indices[ tri_idx*3 + 2 ];
	}

	//
	for( UINT vtx_idx = 0; vtx_idx < src_mesh.positions._count; vtx_idx++ )
	{
		Vec3f &dst_pos = vertex_positions_[ vtx_idx ];

		const V3f& src_pos = src_mesh.positions[ vtx_idx ];
		dst_pos.v[0] = src_pos.x;
		dst_pos.v[1] = src_pos.y;
		dst_pos.v[2] = src_pos.z;
	}

	return ALL_OK;
}


#if SG_USE_SDF_VOLUMES

static
ERet CreateSDFVolume(
					 const TcModel& imported_model
					 , CompiledAssetData &compiled_sdf_volume_asset_
					 )
{
	std::vector<Vec3ui>	triangles;
	std::vector<Vec3f>	vertex_positions;

	//
	mxDO(CopyMeshData(
		imported_model
		, triangles
		, vertex_positions
		));

	//
	const int size_x = SDF_VOL_RES;
	const int size_y = SDF_VOL_RES;
	const int size_z = SDF_VOL_RES;

	/// the grid spacing - specifies the length of grid cell in the resulting distance field
	const float dx = 1.0f / SDF_VOL_RES;

	//TODO: padding

	Array3f	dist_grid;
	//
	make_level_set3(
		triangles, vertex_positions
		, Vec3f(0,0,0)//const Vec3f &origin
		, dx
		, size_x, size_y, size_z
		, dist_grid
		//, const int exact_band=1
		);

	//
    const Vec3f diagonal = Vec3f(
        dx * float(dist_grid.ni),
        dx * float(dist_grid.nj),
        dx * float(dist_grid.nk)
		);

    const float diagonal_length = mag(diagonal);

	//
	const unsigned long total_num_grid_cells = dist_grid.a.size();

	const size_t sdf_volume_texture_size = total_num_grid_cells * sizeof(SDFValue);

	//
	mxDO(compiled_sdf_volume_asset_.object_data.setCountExactly(
		sizeof(TextureHeader_d) + sdf_volume_texture_size
		));

	//
	TextureHeader_d& texture_header = *(TextureHeader_d*) compiled_sdf_volume_asset_.object_data.raw();
	mxZERO_OUT(texture_header);

	texture_header.magic	= TEXTURE_FOURCC;
	texture_header.size		= sdf_volume_texture_size;
	texture_header.width	= SDF_VOL_RES;
	texture_header.height	= SDF_VOL_RES;
	texture_header.depth	= SDF_VOL_RES;
	texture_header.type		= TEXTURE_3D;
	
#if SG_DISTANCE_FIELD_FORMAT_USED == SG_DISTANCE_FIELD_FORMAT_FLOAT
	texture_header.format	= NwDataFormat::R32F;
#elif SG_DISTANCE_FIELD_FORMAT_USED == SG_DISTANCE_FIELD_FORMAT_UINT16
	texture_header.format	= NwDataFormat::R32F;
#else
#	error Unimpl
#endif

	texture_header.num_mips	= 1;

	//
	SDFValue *dst_sdf_voxel_grid_ = (SDFValue*) (&texture_header + 1);

	//
	for (unsigned long i = 0; i < total_num_grid_cells; ++i)
	{
		float v = dist_grid.a[i];
		float v_01 = (v / diagonal_length) * 0.5f + 0.5f;

#if SG_DISTANCE_FIELD_FORMAT_USED == SG_DISTANCE_FIELD_FORMAT_FLOAT
		dst_sdf_voxel_grid_[i] = v_01;
#elif SG_DISTANCE_FIELD_FORMAT_USED == SG_DISTANCE_FIELD_FORMAT_UINT16
		dst_sdf_voxel_grid_[i] = U16(v_01 * 65535.0f);
#endif
	}

	return ALL_OK;
}

#endif



namespace
{
	ERet CopyToolchainMeshIntoTriMesh(
		Meshok::TriMesh & dst_mesh_
		, const TcTriMesh& src_mesh
		)
	{
		mxDO(dst_mesh_.vertices.setNum(src_mesh.positions.num()));

		const U32 num_tris = src_mesh.indices.num()/3;
		mxDO(dst_mesh_.triangles.setNum(num_tris));

		for(UINT iVtx = 0; iVtx < src_mesh.positions.num(); iVtx++)
		{
			dst_mesh_.vertices._data[iVtx].xyz = src_mesh.positions._data[iVtx];
		}

		for(UINT iTri = 0; iTri < num_tris; iTri++)
		{
			Meshok::TriMesh::Triangle& dst_tri = dst_mesh_.triangles._data[iTri];
			dst_tri.x = src_mesh.indices._data[ iTri*3 + 0 ];
			dst_tri.y = src_mesh.indices._data[ iTri*3 + 1 ];
			dst_tri.z = src_mesh.indices._data[ iTri*3 + 2 ];
		}

		return ALL_OK;
	}

	ERet CopyTriMeshIntoToolchainMesh(
		TcTriMesh &dst_mesh_
		, const Meshok::TriMesh & src_mesh
		)
	{
		mxDO(dst_mesh_.positions.setNum(src_mesh.vertices.num()));

		const U32 num_tris = src_mesh.triangles.num();
		mxDO(dst_mesh_.indices.setNum(num_tris*3));

		for(UINT iVtx = 0; iVtx < src_mesh.vertices.num(); iVtx++)
		{
			dst_mesh_.positions._data[iVtx] = src_mesh.vertices._data[iVtx].xyz;
		}

		for(UINT iTri = 0; iTri < num_tris; iTri++)
		{
			const Meshok::TriMesh::Triangle& src_tri = src_mesh.triangles._data[iTri];

			dst_mesh_.indices._data[ iTri*3 + 0 ] = src_tri.x;
			dst_mesh_.indices._data[ iTri*3 + 1 ] = src_tri.y;
			dst_mesh_.indices._data[ iTri*3 + 2 ] = src_tri.z;
		}

		return ALL_OK;
	}


	ERet GenerateCoarserLODs_BAD(
		const TcTriMesh& src_mesh
		, const AssetID& original_mesh_id
		, AllocatorI & scratch_allocator
		)
	{
		Meshok::TriMesh	tri_mesh(scratch_allocator);
		mxDO(CopyToolchainMeshIntoTriMesh(tri_mesh, src_mesh));

		//
		{
			const int grid_res = 1024;

#if 0
			Meshok::DecimatorVC	decimator_vc;

			DynamicArray< U32 > vertexRemap(scratch_allocator);

			mxDO(decimator_vc.Decimate(
				tri_mesh,
				tri_mesh.RecomputeAABB(),
				grid_res,
				scratch_allocator,
				vertexRemap
				));
#else

			Meshok::DecimatorVC2	decimator_vc(scratch_allocator);

			Meshok::DecimatorVC2::Settings	settings;
			settings.bounds = tri_mesh.RecomputeAABB();
			settings.gridSize = grid_res;

			mxDO(decimator_vc.Decimate(
				tri_mesh,
				settings
				));
#endif
		}

		//
		mxASSERT(tri_mesh.IsValid());


		//
		Meshok::MeshSimplifier	decimator_ec(scratch_allocator);

		Meshok::MeshSimplifier::Settings	meshsimp_settings;
		meshsimp_settings.error_threshold = BIG_NUMBER;
		
		U32	prev_triangle_count = tri_mesh.triangles.num();
		for(UINT iLOD = 1; iLOD < MAX_MESH_LODS; iLOD++)
		{
			meshsimp_settings.target_number_of_triangles *= 0.5f;

			mxDO(decimator_ec.Simplify(tri_mesh, meshsimp_settings));

			//
			const UINT num_tris_after_simplification = tri_mesh.triangles.num();

			DEVOUT("Simplified '%s' (LOD=%d) to %d tris, %d verts",
				original_mesh_id.c_str(), iLOD,
				num_tris_after_simplification, tri_mesh.vertices.num()
				);

			if(num_tris_after_simplification < prev_triangle_count)
			{
				String256	lod_mesh_name;
				Str::Format(lod_mesh_name, "%s_%d", original_mesh_id.c_str(), iLOD);

				//
				FilePathStringT	dst_mesh_savefile_path;
				Str::ComposeFilePath(dst_mesh_savefile_path, "R:/", lod_mesh_name.c_str(), "obj");

				tri_mesh.Debug_SaveToObj(dst_mesh_savefile_path.c_str());
			}
			else
			{
				DBGOUT("Cannot simplify %s further. Max LoD: %d", original_mesh_id.c_str(), iLOD);
				break;
			}

			prev_triangle_count = num_tris_after_simplification;
		}

		//
		return ALL_OK;
	}
}//namespace


namespace
{

	const AssetID ComposeMeshLodAssetId(const AssetID& original_mesh_id, int iLOD)
	{
		String256	lod_mesh_name;
		Str::Format(lod_mesh_name, "%s_%d", original_mesh_id.c_str(), iLOD);

		return MakeAssetID(lod_mesh_name.c_str());
	}


	ERet CompileMesh(
		const TcTriMesh& src_mesh
		, const TbVertexFormat& vertex_format
		, CompiledAssetData &outputs_
		, AllocatorI & scratchpad
		)
	{
		TcModel	tmp_model( scratchpad );

		TcTriMesh::Ref	tmp_mesh = new TcTriMesh(scratchpad);
		mxDO(tmp_mesh->CopyFrom(src_mesh));
		mxDO(tmp_model.meshes.add(tmp_mesh));

		//
		TbRawMeshData	raw_mesh_data;
		mxDO(MeshLib::CompileMesh(
			tmp_model
			, vertex_format
			, raw_mesh_data
			));

		//
		mxDO(Rendering::NwMesh::CompileMesh(
			NwBlobWriter(outputs_.object_data)
			, raw_mesh_data
			, scratchpad
			));

		return ALL_OK;
	}

	///
	ERet GenerateCoarserLODs(
		const TcTriMesh& src_mesh
		, const AssetID& original_mesh_id
		, const TbVertexFormat& vertex_format
		, AssetDatabase & asset_database
		, AllocatorI & scratch_allocator
		)
	{
		TcTriMesh	tmp_mesh(scratch_allocator);
		mxDO(tmp_mesh.CopyFrom(src_mesh));

		const size_t num_src_indices = tmp_mesh.indices._count;

		//
		DynamicArray<U32>	simplified_indices(scratch_allocator);
		mxDO(simplified_indices.setNum(
			num_src_indices
			));

		//
		U32 target_num_indices = num_src_indices;
		const float target_error = BIG_NUMBER;

		//
		U32	prev_index_count = num_src_indices;

		for(UINT iLOD = 1; iLOD < MAX_MESH_LODS; iLOD++)
		{
			target_num_indices *= 0.5f;
			target_num_indices = largest(target_num_indices, 100);

			//
			float resulting_error = 0;

			// Start with the original high-resolution mesh.
			{
				const size_t num_simplified_indices = meshopt_simplify(
					simplified_indices._data

					, src_mesh.indices._data
					, src_mesh.indices._count

					, (const float*)src_mesh.positions._data
					, src_mesh.positions._count
					, sizeof(src_mesh.positions[0])

					, target_num_indices
					, target_error
					, &resulting_error
					);

				//
				mxDO(simplified_indices.setNum(
					num_simplified_indices
					));
				mxDO(Arrays::Copy(tmp_mesh.indices, simplified_indices));
			}


			//
			const UINT num_tris_after_simplification = tmp_mesh.indices._count / 3;

			DEVOUT("Simplified '%s' (LOD=%d) to %d tris, %d verts (target = %d)",
				original_mesh_id.c_str(), iLOD,
				num_tris_after_simplification, tmp_mesh.positions._count,
				target_num_indices
				);

			//
			if(tmp_mesh.indices._count > target_num_indices * 1.5f)
			{
				// could not reach the desired polycount using edge collapses,
				// try vertex clustering

				const size_t num_simplified_indices2 = meshopt_simplifySloppy(
					simplified_indices._data

					, src_mesh.indices._data
					, src_mesh.indices._count

					, (const float*)src_mesh.positions._data
					, src_mesh.positions._count
					, sizeof(src_mesh.positions[0])

					, target_num_indices
					, target_error
					, &resulting_error
					);
				//
				mxDO(simplified_indices.setNum(
					num_simplified_indices2
					));
				mxDO(Arrays::Copy(tmp_mesh.indices, simplified_indices));

				//
				DEVOUT("\t Re-Simplified '%s' (LOD=%d) using vertex clustering to %d tris, %d verts (target = %d)",
					original_mesh_id.c_str(), iLOD,
					num_simplified_indices2/3, tmp_mesh.positions._count,
					target_num_indices
					);
			}//


			//
			const size_t num_unique_vertices = meshopt_optimizeVertexFetch(
				tmp_mesh.positions._data

				, tmp_mesh.indices._data
				, tmp_mesh.indices._count

				, (const float*)src_mesh.positions._data
				, src_mesh.positions._count
				, sizeof(src_mesh.positions[0])
				);
			mxDO(tmp_mesh.positions.setNum(num_unique_vertices));

			//
			meshopt_optimizeVertexCache(
				simplified_indices._data

				, tmp_mesh.indices._data
				, tmp_mesh.indices._count

				, tmp_mesh.positions._count
				);

			//
			mxDO(Arrays::Copy(tmp_mesh.indices, simplified_indices));

			//





			//
			const AssetID	lod_mesh_asset_id = ComposeMeshLodAssetId(original_mesh_id, iLOD);

#if 0//MX_DEVELOPER
			//
			FilePathStringT	dst_mesh_savefile_path;
			Str::ComposeFilePath(dst_mesh_savefile_path, "R:/", lod_mesh_asset_id.c_str(), "obj");

			tmp_mesh.Debug_SaveToObj(dst_mesh_savefile_path.c_str());
#endif


			//
			CompiledAssetData	compiled_lod_mesh_data( scratch_allocator );
			mxDO(CompileMesh(
				tmp_mesh
				, vertex_format
				, compiled_lod_mesh_data
				, scratch_allocator
				));

			mxDO(asset_database.addOrUpdateGeneratedAsset(
				lod_mesh_asset_id
				, Rendering::NwMesh::metaClass()
				, compiled_lod_mesh_data
				));

			//

			//
			prev_index_count = num_tris_after_simplification;
		}

		//
		return ALL_OK;
	}
}//namespace



ERet Spaceship_Compiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	AllocatorI & scratchpad = MemoryHeaps::temporary();

	//
	SpaceshipBuildSettings	spaceship_build_settings;

	if(inputs.asset_build_rules_parse_node) {
		mxDO(SON::Decode(inputs.asset_build_rules_parse_node, spaceship_build_settings));
	}

	//
	const TbVertexFormat& vertex_format = SpaceshipVertex::metaClass();

	//
	Meshok::MeshImportSettings	mesh_import_settings;
	mesh_import_settings.vertex_format = &vertex_format;
	mesh_import_settings.normalize_to_unit_cube = true;
	mesh_import_settings.flip_winding_order = spaceship_build_settings.flip_winding_order;

	mesh_import_settings.mesh_optimization_level = Meshok::OL_Max;


	// we don't use texturing (yet)
	mesh_import_settings.force_regenerate_UVs = true;

	// some models don't have UVs => cannot generate tangents
	mesh_import_settings.force_ignore_uvs = true;


	//
	bool	has_pretransform = false;
	M44f	pretransform = M44_Identity();

	//
	if(spaceship_build_settings.scale != 1.0f)
	{
		pretransform = M44_uniformScaling(spaceship_build_settings.scale);
		has_pretransform = true;
	}

	//
	if( !M44_Equal( spaceship_build_settings.rotate, g_MM_Identity ) )
	{
		pretransform = pretransform * spaceship_build_settings.rotate;;
		has_pretransform = true;
	}

	//
	if(has_pretransform)
	{
		mesh_import_settings.pretransform = &pretransform;
	}

	//
	TcModel	imported_model( scratchpad );
	mxDO(MeshCompiler::CompileMeshAsset(
		inputs
		, outputs
		, imported_model
		, mesh_import_settings
		));

	//
	//mxASSERT2(imported_model.meshes.num() == 1,
	//	"each spaceship model must be rendered in a single drawcall!");
	if(imported_model.meshes.num() != 1) {
		ptWARN("each spaceship model must be rendered in a single drawcall! but got %d submeshes", imported_model.meshes.num());
	}

	//
	const TcTriMesh& mesh = *imported_model.meshes[0];

	LogStream(LL_Info)
		,"Compiled spaceship mesh ",inputs.asset_id
		,": triangles = ", mesh.indices._count/3
		,", vertices = ", mesh.positions._count
		;

	//


#if SG_USE_SDF_VOLUMES

	//
	CompiledAssetData	compiled_sdf_volume_data( scratchpad );
	//
	mxDO(CreateSDFVolume(
		imported_model
		, compiled_sdf_volume_data
		));

	mxDO(inputs.asset_database.addOrUpdateGeneratedAsset(
		inputs.asset_id
		, NwTexture::metaClass()
		, compiled_sdf_volume_data
		));

#endif


#if SG_USE_LODS

	outputs.override_asset_id = ComposeMeshLodAssetId(inputs.asset_id, 0);

	//
	mxDO(GenerateCoarserLODs(
		mesh
		, inputs.asset_id
		, vertex_format
		, inputs.asset_database
		, scratchpad
		));
#endif

	return ALL_OK;
}

}//namespace AssetBaking
