//
#include <Base/Base.h>
#pragma hdrstop

#include <Core/Serialization/Serialization.h>

#include <AssetCompiler/AssetPipeline.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_AssetUtil.h>
#include <AssetCompiler/AssetBundleBuilder.h>


using namespace Rendering;

/*
-----------------------------------------------------------------------------
	Doom3AssetsConfig
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( Doom3AssetsConfig );
mxBEGIN_REFLECTION( Doom3AssetsConfig )
	mxMEMBER_FIELD( asset_folder ),
	mxMEMBER_FIELD( scale_meshes_to_meters ),
mxEND_REFLECTION
Doom3AssetsConfig::Doom3AssetsConfig()
{
	scale_meshes_to_meters = false;
}
void Doom3AssetsConfig::fixPathsAfterLoading()
{
	Str::NormalizePath( asset_folder );
}

/*
-----------------------------------------------------------------------------
	Doom3_AssetBundles
-----------------------------------------------------------------------------
*/
Doom3_AssetBundles::Doom3_AssetBundles( AllocatorI & allocator )
	: textures			( allocator, Doom3::PathToAssetID("doom3_textures") )
	, materials			( allocator, Doom3::PathToAssetID("doom3_materials") )
	//, material_tables	( allocator, Doom3::PathToAssetID("doom3_material_tables") )

	, audio_files		( allocator, Doom3::PathToAssetID("doom3_audio_files") )
	, sound_shaders		( allocator, Doom3::PathToAssetID("doom3_sound_shaders") )

	, meshes			( allocator, Doom3::PathToAssetID("doom3_meshes") )
	, anims				( allocator, Doom3::PathToAssetID("doom3_anims") )
	, models			( allocator, Doom3::PathToAssetID("doom3_models") )
{
}

Doom3_AssetBundles::~Doom3_AssetBundles()
{
}

ERet Doom3_AssetBundles::initialize( AssetDatabase& asset_database )
{
	mxDO(_SetupBundle(textures,			asset_database ));
	mxDO(_SetupBundle(materials,		asset_database ));
	//mxDO(_SetupBundle(material_tables,asset_database ));

	mxDO(_SetupBundle(audio_files,		asset_database ));
	mxDO(_SetupBundle(sound_shaders,	asset_database ));

	mxDO(_SetupBundle(meshes,			asset_database ));
	mxDO(_SetupBundle(anims,			asset_database ));
	mxDO(_SetupBundle(models,			asset_database ));

	return ALL_OK;
}

ERet Doom3_AssetBundles::saveAndClose( AssetDatabase & asset_database )
{
	mxDO(_CloseBundle( textures			, asset_database));
	mxDO(_CloseBundle( materials		, asset_database));

	mxDO(_CloseBundle( audio_files		, asset_database));
	mxDO(_CloseBundle( sound_shaders	, asset_database));

	mxDO(_CloseBundle( meshes			, asset_database));
	mxDO(_CloseBundle( anims			, asset_database));
	mxDO(_CloseBundle( models			, asset_database));

	//
	mxDO(asset_database._asset_bundles.saveInFolderIfNeeded(
		asset_database.pathToContainingFolder()
		));

	//
	material_tables.tables.shrink();

	//
	for( UINT i = 0; i < material_tables.tables.num(); i++ )
	{
		idDeclTable & table = material_tables.tables[i];
		table._index = i;
	}

	//
	CompiledAssetData	compiled_tables( MemoryHeaps::temporary() );
	if( !material_tables.tables.IsEmpty() )
	{
		mxDO(Serialization::SaveMemoryImage(
			material_tables
			, NwBlobWriter(compiled_tables.object_data)
			));

		mxDO(asset_database.addOrUpdateGeneratedAsset(
			Rendering::idDeclTableList::getAssetId()
			, Rendering::idDeclTableList::metaClass()
			, compiled_tables
			));
	}

	return ALL_OK;
}


ERet Doom3_AssetBundles::_SetupBundle(
	NwAssetBundleT & asset_bundle
	, AssetDatabase & asset_database
	)
{

#if nwUSE_LOOSE_ASSETS

	//const char* asset_database_folder = asset_database.pathToContainingFolder();
	//mxDO(asset_bundle.CreateFolderInFolder( asset_database_folder ));

#else

	mxDO(asset_bundle.initialize( 1024 ));

#endif

	////
	//asset_database._asset_bundles.addAssetBundleIfNeeded(
	//	asset_bundle._asset_bundle_id
	//	);

	return ALL_OK;
}

ERet Doom3_AssetBundles::_CloseBundle(
	NwAssetBundleT & asset_bundle
	, AssetDatabase & asset_database
	)
{
	//mxDO(AssetBaking::TbDevAssetBundleUtil::saveAndClose( asset_bundle ));
	return ALL_OK;
}

namespace Doom3
{

const AssetID PathToAssetID(
	const char* relative_filepath
	)
{
	mxASSERT(strlen(relative_filepath) > 0);

	String64	fixed_relative_filepath;
	Str::CopyS( fixed_relative_filepath, relative_filepath );
	Str::StripFileExtension( fixed_relative_filepath );
	Str::ReplaceChar( fixed_relative_filepath, '/', '_' );

	const AssetID asset_id = AssetID_from_FilePath( fixed_relative_filepath.c_str() );
	return asset_id;
}

ERet composeFilePath(
	String &fullpath_
	, const char* relative_path
	, const Doom3AssetsConfig& doom3
	)
{
	return Str::ComposeFilePath(
		fullpath_
		, doom3.asset_folder.c_str()
		, relative_path
		);
}

bool shouldBeCompiledSeparately( const char* source_file_path )
{
	return strstr( source_file_path, "_$dev" ) != nil;
}

}//namespace Doom3
