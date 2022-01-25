// parses and compiles *.def files which contain entity definitions
#include "stdafx.h"
#pragma hdrstop

#include <set>

#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Assets/AssetBundle.h>

#include <Rendering/Private/Modules/idTech4/idEntity.h>

#include <Sound/SoundResources.h>

#include <Utility/Animation/AnimationUtil.h>
#include <Developer/UniqueNameHashChecker.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_ModelDef_Parser.h>
#include <Developer/ThirdPartyGames/Doom3/Doom3_Def_FileParser.h>

#include <AssetCompiler/AssetCompilers/Doom3/Doom3_AssetUtil.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_idEntityDef_Compiler.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_AnimatedModelCompiler.h>
#include <AssetCompiler/AssetBundleBuilder.h>
#include <AssetCompiler/AssetDatabase.h>


using namespace Rendering;

namespace AssetBaking
{

mxDEFINE_CLASS(Doom3_idEntityDef_Compiler);

const TbMetaClass* Doom3_idEntityDef_Compiler::getOutputAssetClass() const
{
	return &idEntityDef::metaClass();
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
	mxDO(asset_database.addOrUpdateGeneratedAsset(
		mesh_asset_id
		, Rendering::NwMesh::metaClass()
		, compiled_mesh_data
		));

	return ALL_OK;
}

ERet SetupEntityDef(
					idEntityDef &entity_def_
					, const Doom3::EntityDef& src_entity_def
					, const Doom3::ModelDef& src_model_def
					)
{
	entity_def_.skinned_mesh._setId(
		Doom3::PathToAssetID(src_model_def.name.c_str())
		);

	// Copy sound refs.
	{
		mxDO(entity_def_.sounds.ReserveExactly(
			src_entity_def.sounds.num()
			));

		nwFOR_EACH(const Doom3::EntityDef::SoundRef& sound_ref, src_entity_def.sounds)
		{
			const TResPtr< NwSoundSource >	sound_asset_ref(
				MakeAssetID(sound_ref.sound_resource.c_str())
				);
			const NameHash32 sound_name_hash =
				GetDynamicStringHash(sound_ref.sound_local_name.c_str())
				;
			mxASSERT(sound_name_hash != 0);

			entity_def_.sounds.add(
				sound_name_hash
				, sound_asset_ref
				);
		}
		entity_def_.sounds.sort();
		entity_def_.sounds.shrink();
	}

	return ALL_OK;
}

ERet Doom3_idEntityDef_Compiler::CompileAsset(
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
	Doom3::DefFileContents	def_file_contents( scratchpad );

	mxDO(Doom3::ParseDefFile(
		def_file_contents
		, inputs.path.c_str()
		));

	//
	const Doom3::EntityDef* entitydef_with_model = def_file_contents.FindFirstEntityDefWithModel();
	mxENSURE(entitydef_with_model, ERR_OBJECT_NOT_FOUND, "No valid entities found!");

	//
	const char* model_name = entitydef_with_model->FindUsedModelName();
	mxENSURE(model_name, ERR_OBJECT_NOT_FOUND, "");

	//
	const Doom3::ModelDef* model_def = def_file_contents.FindModelDefByName(model_name);
	mxENSURE(model_def, ERR_OBJECT_NOT_FOUND, "");



	//
	idEntityDef	entity_def;
	mxDO(SetupEntityDef(
		entity_def
		, *entitydef_with_model
		, *model_def
		));

	//
	mxDO(Doom3_AnimatedModelCompiler::Static_CompileModelFromDef(
		*model_def
		, containing_folder.c_str()
		, entity_def.skinned_mesh
		, inputs.cfg.doom3
		, inputs.asset_database
		, scratchpad
		));

	//
	mxDO(Serialization::SaveMemoryImage(
		entity_def
		, NwBlobWriter( outputs.object_data )
		));

	//
	LogStream(LL_Info),"Compiled entity ",inputs.path,", model: ",model_name;

	return ALL_OK;
}

}//namespace AssetBaking
