// compiler for animated (skinned) models (e.g. idTech 4's MD5)
#include "stdafx.h"
#pragma hdrstop

#include <set>

#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Assets/AssetBundle.h>

#include <AssetCompiler/AssetCompilers/Doom3/Doom3_AssetUtil.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_AnimatedModelCompiler.h>
#include <AssetCompiler/AssetBundleBuilder.h>
#include <AssetCompiler/AssetDatabase.h>

#include <Utility/Animation/AnimationUtil.h>
#include <Developer/UniqueNameHashChecker.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_ModelDef_Parser.h>


namespace AssetBaking
{

using namespace Rendering;

mxDEFINE_CLASS(Doom3_AnimatedModelCompiler);

const TbMetaClass* Doom3_AnimatedModelCompiler::getOutputAssetClass() const
{
	return &Rendering::NwSkinnedMesh::metaClass();
}

/// Composes an absolute/full filepath for opening an md5 mesh/anim file.
static
ERet ComposeMd5FileName(
						String &absolute_filepath_

						// e.g.: D:/dev/TowerDefenseGame/models/zombies/zcc_chaingun/monster_zombie_commando_cgun.def
						, const char* containing_folder

						// e.g.: models/md5/monsters/zcc/chaingun_wallrotleft_D.md5anim
						, const char* relative_filepath
						)
{
	String256	filename;
	mxDO(Str::CopyS( filename, relative_filepath ));
	Str::StripPath( filename );

	return Str::ComposeFilePath(
		absolute_filepath_
		, containing_folder
		, filename.c_str()
		);
}

/// NOTE: always appends to the existing blob!
class ozzBlobWriter: public ozz::io::Stream
{
	NwBlobWriter	_blob_writer;

public:
	ozzBlobWriter( NwBlob & blob )
		: _blob_writer( blob, blob.num() )
	{
	}

	// Tests whether a file is opened.
	virtual bool opened() const { return true; }

	// Reads _size bytes of data to _buffer from the stream. _buffer must be big
	// enough to store _size bytes. The position indicator of the stream is
	// advanced by the total amount of bytes read.
	// Returns the number of bytes actually read, which may be less than _size.
	virtual size_t Read(void* _buffer, size_t _size) override
	{
		mxUNREACHABLE;
		return 0;
	}

	// Writes _size bytes of data from _buffer to the stream. The position
	// indicator of the stream is advanced by the total number of bytes written.
	// Returns the number of bytes actually written, which may be less than _size.
	virtual size_t Write(const void* _buffer, size_t _size) override
	{
		return _blob_writer.Write( _buffer, _size ) == ALL_OK ? _size : 0;
	}

	// Sets the position indicator associated with the stream to a new position
	// defined by adding _offset to a reference position specified by _origin.
	// Returns a zero value if successful, otherwise returns a non-zero value.
	virtual int Seek(int _offset, Origin _origin) override
	{
		mxUNREACHABLE;
		return 0;
	}

	// Returns the current value of the position indicator of the stream.
	// Returns -1 if an error occurs.
	virtual int Tell() const override
	{
		return _blob_writer.Tell();
	}

	// Returns the current size of the stream.
	virtual size_t Size() const override
	{
		return _blob_writer.Tell();
	}
};

static
ERet saveAnimationToBlob(
						 NwBlob &compiled_animation_data_
						 , const Rendering::NwAnimClip& animation_clip
					  )
{
	//
	NwBlobWriter	blob_writer(compiled_animation_data_);
	mxDO(Serialization::SaveMemoryImage(
		animation_clip
		, blob_writer
		));

	mxDO(blob_writer.alignBy(Rendering::OZZ_DATA_ALIGNMENT));

	//
	ozzBlobWriter		ozz_blob_writer( compiled_animation_data_ );
	ozz::io::OArchive	ozz_output_archive( &ozz_blob_writer );

	animation_clip.ozz_animation.Save( ozz_output_archive );

	return ALL_OK;
}

static
ERet CompileAnimation(
					  AssetID &animation_asset_id_
					  , const char* animation_filename
					  , const Doom3::md5Animation& md5_animation
					  , const Doom3::ModelDef::Anim& anim_def
					  , const ozz::animation::Skeleton& ozz_skeleton
					  , AssetDatabase & asset_database
					  )
{
	const AssetID new_asset_id = Doom3::PathToAssetID( animation_filename );

	if( asset_database.containsAssetWithId(
		new_asset_id
		, Rendering::NwAnimClip::metaClass()
		) )
	{
		return ALL_OK;
	}

	//
	Rendering::NwAnimClip	new_anim_clip;

	//
	mxDO(convert_animation_from_md5_to_ozz(
		new_anim_clip.ozz_animation
		, md5_animation
		, ozz_skeleton
		));

	// Events
	{
		new_anim_clip.events = anim_def.events;

		// In each animation event list, patch 'time' from 'frame_number' to quantized time.
		for( UINT i = 0; i < new_anim_clip.events._frame_lookup.num(); i++ )
		{
			const U32 frame_number = new_anim_clip.events._frame_lookup[i].time;
			const float normalized_time = (float)frame_number / (float)md5_animation.numFrames;
			mxASSERT(normalized_time >= 0.0f && normalized_time <= 1.0f);

			const U32 integer_range = ( 1u << BYTES_TO_BITS(sizeof(new_anim_clip.events._frame_lookup[i].time)) ) - 1;
			const U32 quantized_time = U32( normalized_time * (float)integer_range );

			new_anim_clip.events._frame_lookup[i].time = quantized_time;
		}
	}

	//
	{
		const Doom3::md5Skeleton& first = md5_animation.frameSkeletons.getFirst();
		const Doom3::md5Skeleton& last = md5_animation.frameSkeletons.getLast();

		// no need to convert from Doom3 to meters - the animation was already scaled down on load
		const V3f root_joint_pos_in_last_frame = last[ Doom3::MD5_ROOT_JOINT_INDEX ].position;

		const V3f total_movement_delta_of_root_joint
			= root_joint_pos_in_last_frame - first[ Doom3::MD5_ROOT_JOINT_INDEX ].position
			;

		const V3f average_movement_speed_of_root_joint
			= total_movement_delta_of_root_joint * (1.0f / md5_animation.totalDurationInSeconds())
			;

		//LogStream(LL_Info),"Anim ",anim_filename," speed: ",average_movement_speed_of_root_joint;

		new_anim_clip.root_joint_pos_in_last_frame = root_joint_pos_in_last_frame;
		new_anim_clip.root_joint_average_speed = average_movement_speed_of_root_joint;
	}

	//new_animation.frame_count = md5_animation.numFrames;

	new_anim_clip.id = new_asset_id;

	//
	CompiledAssetData	compiled_animation( MemoryHeaps::temporary() );
	mxDO(saveAnimationToBlob( compiled_animation.object_data, new_anim_clip ));

	mxDO(asset_database.addOrUpdateGeneratedAsset(
		new_asset_id
		, Rendering::NwAnimClip::metaClass()
		, compiled_animation
		));

	return ALL_OK;
}

template<>
struct THashTrait< std::string >
{
	static inline U32 ComputeHash32( const std::string& key )
	{
		return GetDynamicStringHash( key.c_str() );
	}
};

typedef THashMap< std::string, U32 >	AnimationFrameCountByNameT;

static
ERet CompileAnimations(
					   AnimationFrameCountByNameT *available_md5_animations_
					  , const Doom3::ModelDef& model_def
					  , const Doom3::MD5LoadOptions& md5_load_options
					  , const ozz::animation::Skeleton& ozz_skeleton
					  , AssetDatabase & asset_database
					  , const char* containing_folder
					  )
{
	NwUniqueNameHashChecker	anim_name_hash_checker;
	NwUniqueNameHashChecker	md5_anim_name_hash_checker;

	nwFOR_EACH(const Doom3::ModelDef::Anim& anim_def, model_def.anims)
	{
		mxDO(anim_name_hash_checker.InsertUniqueName(
			anim_def.name.c_str(),
			GetDynamicStringHash(anim_def.name.c_str())
			));

		// NOTE: there can several model anims referring to the same MD5 anim,
		// e.g., in "archvile.def" model anims "melee_attack2" and "melee_attack4"
		// both refer to the same 'models/md5/monsters/archvile/attack2.md5anim'.

		if(mxFAILED(md5_anim_name_hash_checker.InsertUniqueName(
			anim_def.md5_anim_id.c_str(),
			GetDynamicStringHash(anim_def.md5_anim_id.c_str())
			)))
		{
			// You'd better check that all anims have the same events.
			ptWARN("Model '%s' uses MD5 anim '%s' (under name '%s') more than once!",
				model_def.mesh_name.c_str(), anim_def.md5_anim_id.c_str(), anim_def.name.c_str()
				);
		}
	}

	//
	Doom3::md5Animation			md5_animation;

	nwFOR_EACH(const Doom3::ModelDef::Anim& anim_def, model_def.anims)
	{
		//DBGOUT("%s", (*it).c_str());

		if(available_md5_animations_->Contains(anim_def.md5_anim_id.c_str()))
		{
			DBGOUT("'%s': Skipping duplicate anim '%s'",
				model_def.mesh_name.c_str(), anim_def.md5_anim_id.c_str());
			continue;
		}

		//
		String256	md5_anim_filepath;
		mxDO(ComposeMd5FileName(
			md5_anim_filepath
			, containing_folder
			, anim_def.md5_anim_id.c_str()
			));

		//
		if(mxSUCCEDED(Doom3::MD5_LoadAnimFromFile(
			md5_animation
			, md5_anim_filepath.c_str()
			, md5_load_options
			)))
		{
			mxENSURE( md5_animation.IsValid(), ERR_FAILED_TO_PARSE_DATA, "Invalid md5 anim" );

			//
			AssetID	animation_asset_id;

			mxDO(CompileAnimation(
				animation_asset_id
				, anim_def.md5_anim_id.c_str()
				, md5_animation
				, anim_def
				, ozz_skeleton
				, asset_database
				));

			available_md5_animations_->Insert(
				anim_def.md5_anim_id.c_str(),
				md5_animation.numFrames
				);
		}
		else
		{
			ptWARN("failed to open md5 anim: '%s'", md5_anim_filepath.c_str());
		}
	}

	return ALL_OK;
}

static
ERet WriteAnimations(
					 Rendering::NwSkinnedMesh &skinned_mesh_
					 , const Doom3::ModelDef& model_def
					 , const AnimationFrameCountByNameT& available_md5_animations
					 , const ozz::animation::Skeleton& ozz_skeleton
					 , const char* containing_folder
					 )
{
	const UINT total_num_anims = available_md5_animations.NumEntries();

	// NOTE: important - must not be resized when adding animations! pointers must not be invalidated!
	mxDO(skinned_mesh_.anim_clips.Reserve( total_num_anims ));

	//
	NwUniqueNameHashChecker	anim_name_hash_checker;

	//
	for( UINT i = 0; i < model_def.anims.num(); i++ )
	{
		const Doom3::ModelDef::Anim& anim_def = model_def.anims[i];

		const U32* animation_frame_count = available_md5_animations.FindValue( anim_def.md5_anim_id.c_str() );

		if( !animation_frame_count ) {
			continue;
		}

		//
		const NameHash32 anim_name_hash = GetDynamicStringHash( anim_def.name.c_str() );

		mxDO(anim_name_hash_checker.InsertUniqueName(anim_def.name.c_str(), anim_name_hash));

		//
		const TResPtr< Rendering::NwAnimClip >	anim_clip_asset_ref(
			Doom3::PathToAssetID( anim_def.md5_anim_id.c_str() )
			);
		mxDO(skinned_mesh_.anim_clips.add( anim_name_hash, anim_clip_asset_ref ));

	}//for each anim

	//
	skinned_mesh_.anim_clips.sort();

	return ALL_OK;
}

static
ERet createMeshAsset(
					 const AssetID& mesh_asset_id
					 , const Doom3::md5Model& md5_model
					 , const TbSkinnedMeshData& skinned_mesh_data
					 , AssetDatabase & asset_database
					 , AllocatorI & scratchpad
					 )
{
	//LogStream(LL_Info),"Creating mesh: ",mesh_asset_id;

	CompiledAssetData	compiled_mesh_data( scratchpad );
	NwBlobWriter			blob_writer( compiled_mesh_data.object_data );

	//
	const UINT total_num_submeshes = skinned_mesh_data.submeshes.num();

	//
	DynamicArray< const TbSkinnedMeshData::Submesh* >	drawable_submeshes( scratchpad );
	mxDO(drawable_submeshes.reserve( total_num_submeshes ));

	UINT	total_num_vertices = 0;
	UINT	total_num_indices = 0;

	for( UINT iSubmesh = 0; iSubmesh < total_num_submeshes; iSubmesh++ )
	{
		const TbSkinnedMeshData::Submesh& submesh = skinned_mesh_data.submeshes[iSubmesh];
		if( submesh.can_skip_rendering ) {
			continue;
		}

		drawable_submeshes.add( &submesh );

		total_num_vertices += submesh.vertices.num();
		total_num_indices += submesh.indices.num();
	}

	//
	const U32	vertex_stride = sizeof(Vertex_Skinned);

	const bool	use_32_bit_indices = ( total_num_vertices >= MAX_UINT16 );
	const U32	index_stride = use_32_bit_indices ? sizeof(U32) : sizeof(U16);


	Rendering::NwMesh::TbMesh_Header_d	header;
	mxZERO_OUT(header);
	{
		header.magic = Rendering::NwMesh::TbMesh_Header_d::MAGIC;
		header.version = Rendering::NwMesh::TbMesh_Header_d::VERSION;

		header.local_aabb = md5_model.computeBoundingBox();

		header.vertex_count = total_num_vertices;
		header.index_count = total_num_indices;

		header.submesh_count = drawable_submeshes.num();

		header.vertex_format = Vertex_Skinned::metaClass().GetTypeGUID();

		header.topology = NwTopology::TriangleList;

		if( use_32_bit_indices ) {
			header.flags |= Rendering::MESH_USES_32BIT_IB;
		}
	}
	mxDO(blob_writer.Put( header ));

#if MX_DEBUG
	const U32	submeshes_offset = blob_writer.Tell();
#endif

	//
	{
		UINT	num_indices_written = 0;

		for( UINT i = 0; i < drawable_submeshes.num(); i++ )
		{
			const TbSkinnedMeshData::Submesh& submesh = *drawable_submeshes[i];

			const UINT	num_indices_in_submesh = submesh.indices.num();

			//
			Rendering::Submesh	dst_submesh;
			{
				dst_submesh.start_index = num_indices_written;
				dst_submesh.index_count = num_indices_in_submesh;
			}
			mxDO(blob_writer.Put( dst_submesh ));

			num_indices_written += num_indices_in_submesh;
		}
	}

	//
	mxDO(Writer_AlignForward( blob_writer, blob_writer.Tell(), vertex_stride ));

	//
#if MX_DEBUG
	const U32	mesh_data_offset = blob_writer.Tell();
#endif

	// Write vertex and index data.
	const UINT	vertex_data_size = total_num_vertices * vertex_stride;
	const UINT	index_data_size = total_num_indices * index_stride;
	{
		DynamicArray< BYTE >	index_data( scratchpad );
		mxDO(index_data.setNum( index_data_size ));

		UINT	num_vertices_written = 0;
		UINT	num_indices_written = 0;

		// Write vertex data.
		for( UINT i = 0; i < drawable_submeshes.num(); i++ )
		{
			const TbSkinnedMeshData::Submesh& submesh = *drawable_submeshes[i];

			//
			mxDO(blob_writer.Write( submesh.vertices.raw(), submesh.vertices.rawSize() ));

			//
			const UINT	num_vertices_in_submesh = submesh.vertices.num();
			const UINT	num_indices_in_submesh = submesh.indices.num();

			// NOTE: need to reverse Doom3's winding
			//for( UINT i = 0; i < num_indices_in_submesh; i++ ) {
			//	all_indices[ num_indices_written + i ] = submesh.indices[i] + num_vertices_written;
			//}
			const UINT	num_triangles_in_submesh = num_indices_in_submesh / 3;

			if( use_32_bit_indices )
			{
				U32 *raw_indices = (U32*) index_data.raw();

				for( UINT i = 0; i < num_triangles_in_submesh; i++ )
				{
					const UINT i0 = submesh.indices[ i*3 + 0 ];
					const UINT i1 = submesh.indices[ i*3 + 1 ];
					const UINT i2 = submesh.indices[ i*3 + 2 ];

					raw_indices[ num_indices_written + i*3 + 0 ] = i2 + num_vertices_written;
					raw_indices[ num_indices_written + i*3 + 1 ] = i1 + num_vertices_written;
					raw_indices[ num_indices_written + i*3 + 2 ] = i0 + num_vertices_written;
				}
			}
			else
			{
				U16 *raw_indices = (U16*) index_data.raw();

				for( UINT i = 0; i < num_triangles_in_submesh; i++ )
				{
					const UINT i0 = submesh.indices[ i*3 + 0 ];
					const UINT i1 = submesh.indices[ i*3 + 1 ];
					const UINT i2 = submesh.indices[ i*3 + 2 ];

					raw_indices[ num_indices_written + i*3 + 0 ] = i2 + num_vertices_written;
					raw_indices[ num_indices_written + i*3 + 1 ] = i1 + num_vertices_written;
					raw_indices[ num_indices_written + i*3 + 2 ] = i0 + num_vertices_written;
				}
			}

			num_vertices_written += num_vertices_in_submesh;
			num_indices_written += num_indices_in_submesh;
		}

		// Write index data.
		mxDO(blob_writer.Write( index_data.raw(), index_data.rawSize() ));
	}



#if MX_DEBUG
	const U32	material_ids_offset = blob_writer.Tell();
#endif

	// Write material IDs.
	for( UINT i = 0; i < drawable_submeshes.num(); i++ )
	{
		const TbSkinnedMeshData::Submesh& submesh = *drawable_submeshes[i];
//LogStream(LL_Warn),"Mesh [",i,"]: material: ", submesh.material;
		mxDO(writeAssetID( submesh.material, blob_writer ));
	}


	//
#if 0//MX_DEBUG

	DrawVertex*	dbg_verts = (DrawVertex*) mxAddByteOffset(
		compiled_mesh_data.object_data.raw(), mesh_data_offset
		);

	U16*	dbg_inds = (U16*) mxAddByteOffset(
		dbg_verts, vertex_data_size
		);

	if(strstr(AssetId_ToChars(mesh_asset_id), "trite"))
	{
		printf("!");
	}

#endif

	//
	mxDO(asset_database.addOrUpdateGeneratedAsset(
	mesh_asset_id
	, Rendering::NwMesh::metaClass()
	, compiled_mesh_data
	));

	return ALL_OK;
}

ERet Doom3_AnimatedModelCompiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	const Doom3AssetsConfig& doom3_ass_cfg = inputs.cfg.doom3;

	//
	Doom3::MD5LoadOptions	md5_load_options;
	md5_load_options.scale_to_meters = doom3_ass_cfg.scale_meshes_to_meters;


	//
	String256	containing_folder;
	Str::Copy( containing_folder, inputs.path );
	Str::StripFileName( containing_folder );

	//
	Doom3::ModelDef	model_def;

	mxDO(Doom3::ParseModelDef(
		inputs.path.c_str()
		, model_def
		));

	//
	String256	md5_mesh_filepath;
	mxDO(ComposeMd5FileName(
		md5_mesh_filepath
		, containing_folder.c_str()
		, model_def.mesh_name.c_str()
		));

	//
	const U32 expected_asset_count = model_def.anims.num() + 1;

	const AssetID mesh_asset_id = Doom3::PathToAssetID(
		model_def.mesh_name.c_str()
		);

	//
	Doom3::md5Model				md5_model;

	mxDO(Doom3::MD5_LoadModel(
		md5_mesh_filepath.c_str()
		, md5_model
		, md5_load_options
		));

	mxENSURE(
		md5_model.NumJoints() <= nwRENDERER_MAX_BONES,
		ERR_UNSUPPORTED_FEATURE,
		"Too many joints: %d (max: %d)",
		md5_model.NumJoints(),
		nwRENDERER_MAX_BONES
		);

#if 0
	if( V3_LengthSquared(model_def.view_offset) > 0 ) {
		mxASSERT(0);
	}
	// add offset for weapon models in first-person view
	// so that player hands don't go through the weapon mesh
	md5_model.joints[0].position += model_def.view_offset;
#endif

	//
	Rendering::NwSkinnedMesh	skinned_mesh;

	//
	skinned_mesh.bind_pose_aabb = md5_model.computeBoundingBox();

	skinned_mesh.view_offset = model_def.view_offset;

	mxDO(convert_md5_model_to_ozz_skeleton( md5_model, skinned_mesh.ozz_skeleton ));

	const int num_joints = skinned_mesh.ozz_skeleton.num_joints();

	{
		TbSkinnedMeshData	skinned_mesh_data;

		mxDO(buildSkinnedMeshData(
			md5_model
			, skinned_mesh_data
			, skinned_mesh.ozz_skeleton
			));

		//
		mxDO(createMeshAsset(
			mesh_asset_id
			, md5_model
			, skinned_mesh_data
			, inputs.asset_database
			, scratchpad
			));

		//
		mxDO(skinned_mesh.inverse_bind_pose_matrices.setNum( num_joints ));

		for( UINT i = 0; i < num_joints; i++ )
		{
			const M44f& inverse_bind_pose_matrix = skinned_mesh_data.inverseBindPoseMatrices[i];

			const ozz::math::Float4x4	inverse_bind_pose_matrix_ozz = {
				inverse_bind_pose_matrix.r0,
				inverse_bind_pose_matrix.r1,
				inverse_bind_pose_matrix.r2,
				inverse_bind_pose_matrix.r3,
			};

			skinned_mesh.inverse_bind_pose_matrices[i] = ozzMatricesToM44f(inverse_bind_pose_matrix_ozz);
		}
	}

	//
	AnimationFrameCountByNameT	available_md5_animations( scratchpad );
	mxDO(available_md5_animations.Initialize( 512, 32 ));

	mxDO(CompileAnimations(
		&available_md5_animations
		, model_def
		, md5_load_options
		, skinned_mesh.ozz_skeleton
		, inputs.asset_database
		, containing_folder.c_str()
		));

	mxDO(WriteAnimations(
		skinned_mesh
		, model_def
		, available_md5_animations
		, skinned_mesh.ozz_skeleton
		, containing_folder.c_str()
		));

	skinned_mesh.render_mesh._setId( mesh_asset_id );

	//
	skinned_mesh.anim_clips.shrink();

	//
	{
		CompiledAssetData	compiled_model( scratchpad );

		mxDO(Serialization::SaveMemoryImage(
			skinned_mesh
			, NwBlobWriter( compiled_model.object_data )
			));

		//
		ozzBlobWriter			stream( compiled_model.object_data );
		ozz::io::OArchive		output_archive( &stream );
		skinned_mesh.ozz_skeleton.Save( output_archive );

		outputs.object_data = compiled_model.object_data;


		// DEBUG

#if 0
		Rendering::NwSkinnedMesh *	new_instance;
		mxDO(Serialization::LoadMemoryImage(
			NwBlobReader(compiled_model.object_data)
			, new_instance
			, scratchpad
			));

		M44f* m = new_instance->inverse_bind_pose_matrices.raw();
mxASSERT(IS_16_BYTE_ALIGNED(m));
		if(U8* idx = new_instance->animations.find(mxHASH_STR("walk")))
		{
			ModelAnimation& anim = new_instance->animations_with_events[*idx];
			printf("hi");
		}
#endif
	}

	//
	LogStream(LL_Info),"Compiled model ",inputs.path,", ",skinned_mesh.anim_clips.Num()," anims";

	return ALL_OK;
}

ERet Doom3_AnimatedModelCompiler::Static_CompileModelFromDef(
	const Doom3::ModelDef& model_def
	, const char* path_to_md5_files
	, const TResPtr< Rendering::NwSkinnedMesh >& skinned_mesh_id
	, const Doom3AssetsConfig& doom3_ass_cfg
	, AssetDatabase & asset_database
	, AllocatorI& scratchpad
	)
{
	//
	Rendering::NwSkinnedMesh	skinned_mesh;

	// Rendering::NwMesh
	const AssetID mesh_asset_id = Doom3::PathToAssetID(
		Str::GetFileName(model_def.mesh_name.c_str())
		);

	////
	//mxENSURE(
	//	!asset_database.containsAssetWithId(model_asset_id, skinned_mesh.metaClass())
	//	, ERR_DUPLICATE_OBJECT
	//	, ""
	//	);
	//mxENSURE(
	//	!asset_database.containsAssetWithId(mesh_asset_id, skinned_mesh.metaClass())
	//	, ERR_DUPLICATE_OBJECT
	//	, ""
	//	);

	//
	Doom3::MD5LoadOptions	md5_load_options;
	md5_load_options.scale_to_meters = doom3_ass_cfg.scale_meshes_to_meters;

	//
	String256	md5_mesh_filepath;
	mxDO(ComposeMd5FileName(
		md5_mesh_filepath
		, path_to_md5_files
		, model_def.mesh_name.c_str()
		));

	//
	Doom3::md5Model	md5_model;

	mxDO(Doom3::MD5_LoadModel(
		md5_mesh_filepath.c_str()
		, md5_model
		, md5_load_options
		));

	mxENSURE(
		md5_model.NumJoints() <= nwRENDERER_MAX_BONES,
		ERR_UNSUPPORTED_FEATURE,
		"Too many joints: %d (max: %d)",
		md5_model.NumJoints(),
		nwRENDERER_MAX_BONES
		);

#if 0
	if( V3_LengthSquared(model_def.view_offset) > 0 ) {
		mxASSERT(0);
	}
	// add offset for weapon models in first-person view
	// so that player hands don't go through the weapon mesh
	md5_model.joints[0].position += model_def.view_offset;
#endif

	//
	skinned_mesh.bind_pose_aabb = md5_model.computeBoundingBox();

	skinned_mesh.view_offset = model_def.view_offset;

	mxDO(convert_md5_model_to_ozz_skeleton( md5_model, skinned_mesh.ozz_skeleton ));

	const int num_joints = skinned_mesh.ozz_skeleton.num_joints();

	{
		TbSkinnedMeshData	skinned_mesh_data;

		mxDO(buildSkinnedMeshData(
			md5_model
			, skinned_mesh_data
			, skinned_mesh.ozz_skeleton
			));

		//
		mxDO(createMeshAsset(
			mesh_asset_id
			, md5_model
			, skinned_mesh_data
			, asset_database
			, scratchpad
			));

		//
		mxDO(skinned_mesh.inverse_bind_pose_matrices.setNum( num_joints ));

		for( UINT i = 0; i < num_joints; i++ )
		{
			const M44f& inverse_bind_pose_matrix = skinned_mesh_data.inverseBindPoseMatrices[i];

			const ozz::math::Float4x4	inverse_bind_pose_matrix_ozz = {
				inverse_bind_pose_matrix.r0,
				inverse_bind_pose_matrix.r1,
				inverse_bind_pose_matrix.r2,
				inverse_bind_pose_matrix.r3,
			};

			skinned_mesh.inverse_bind_pose_matrices[i] = ozzMatricesToM44f(inverse_bind_pose_matrix_ozz);
		}
	}

	//
	AnimationFrameCountByNameT	available_md5_animations( scratchpad );
	mxDO(available_md5_animations.Initialize( 512, 32 ));

	mxDO(CompileAnimations(
		&available_md5_animations
		, model_def
		, md5_load_options
		, skinned_mesh.ozz_skeleton
		, asset_database
		, path_to_md5_files
		));

	mxDO(WriteAnimations(
		skinned_mesh
		, model_def
		, available_md5_animations
		, skinned_mesh.ozz_skeleton
		, path_to_md5_files
		));

	skinned_mesh.render_mesh._setId( mesh_asset_id );

	//
	skinned_mesh.anim_clips.shrink();

	//
	{
		CompiledAssetData	compiled_mesh( scratchpad );

		mxDO(Serialization::SaveMemoryImage(
			skinned_mesh
			, NwBlobWriter( compiled_mesh.object_data )
			));

		//
		ozzBlobWriter			stream( compiled_mesh.object_data );
		ozz::io::OArchive		output_archive( &stream );
		skinned_mesh.ozz_skeleton.Save( output_archive );

		mxDO(asset_database.addOrUpdateGeneratedAsset(
			skinned_mesh_id._id
			, skinned_mesh.metaClass()
			, compiled_mesh
			));

		// DEBUG
#if 0
		if(strstr(mesh_asset_id.c_str(), "viewmodel_fists"))
		{
			Rendering::NwSkinnedMesh *	new_instance;
			mxDO(Serialization::LoadMemoryImage(
				new_instance
				, NwBlobReader(compiled_mesh.object_data)
				, scratchpad
				));

			M44f* m = new_instance->inverse_bind_pose_matrices.raw();
			mxASSERT(IS_16_BYTE_ALIGNED(m));
			//if(U8* idx = new_instance->animations.find(mxHASH_STR("walk")))
			//{
			//	ModelAnimation& anim = new_instance->animations_with_events[*idx];
			//	printf("hi");
			//}
		}
#endif
	}

	//
	LogStream(LL_Info),"Compiled model ",model_def.name,", ",skinned_mesh.anim_clips.Num()," anims";

	return ALL_OK;
}

}//namespace AssetBaking
