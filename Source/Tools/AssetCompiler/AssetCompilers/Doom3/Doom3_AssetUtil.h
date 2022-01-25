//
#pragma once

//#include <AssetCompiler/AssetPipeline.h>
#include <Rendering/Private/Modules/idTech4/idMaterial.h>


//namespace AssetBaking
//{
	class AssetDatabase;
//}//namespace AssetBaking


/*
-----------------------------------------------------------------------------
	Doom3AssetsConfig

	for stealing assets from Doom 3
-----------------------------------------------------------------------------
*/
struct Doom3AssetsConfig: CStruct
{
	// the path to the folder on the hard drive containing unpacked *.pk4 files
	String256	asset_folder;

	// Scale Doom 3 models to convert to meters?
	// One Doom 3 unit ~= 39.37 meters in our engine
	bool	scale_meshes_to_meters;

public:
	mxDECLARE_CLASS( Doom3AssetsConfig, CStruct );
	mxDECLARE_REFLECTION;

	Doom3AssetsConfig();

	void fixPathsAfterLoading();
};

/*
-----------------------------------------------------------------------------
	Doom3_AssetBundles

	for packaging assets stolen from Doom 3
-----------------------------------------------------------------------------
*/
struct Doom3_AssetBundles: TGlobal< Doom3_AssetBundles >
{
	NwAssetBundleT	textures;			// all textures

	// https://www.iddevnet.com/doom3/materials.html
	// https://wiki.splashdamage.com/index.php/Materials
	NwAssetBundleT	materials;			// tables parsed from Material decls (*.mtr files)
	//NwAssetBundleT	material_tables;	// tables parsed from Material decls (*.mtr files)

	//

	NwAssetBundleT	audio_files;		// all audio files (*.ogg, *.mp3)

	// https://www.iddevnet.com/doom3/sounds.html
	// https://modwiki.dhewm3.org/Sound_%28decl%29
	NwAssetBundleT	sound_shaders;		// sound shaders (*.sndshd files)

	//
	NwAssetBundleT	meshes;		// Rendering::NwSkinnedMesh = md5 meshes
	NwAssetBundleT	anims;		// Rendering::NwSkinnedMesh = md5 anims
	NwAssetBundleT	models;		// Rendering::NwSkinnedMesh = mesh refs + anim refs + anim events

public:
	//DynamicArray< idDeclTable >		table_decls;
	Rendering::idDeclTableList		material_tables;	// tables parsed from Material decls (*.mtr files)

public:
	Doom3_AssetBundles( AllocatorI & allocator );
	~Doom3_AssetBundles();

	ERet initialize(AssetDatabase& asset_database);

	ERet saveAndClose( AssetDatabase & asset_database );

private:
	ERet _SetupBundle(
		NwAssetBundleT & asset_bundle
		, AssetDatabase & asset_database
		);

	ERet _CloseBundle(
		NwAssetBundleT & asset_bundle
		, AssetDatabase & asset_database
		);
};

namespace Doom3
{

	/// Use this instead of make_AssetID_from_raw_string().
	/// It replaces path separators with underscores, e.g.:
	/// "models/md5/monsters/zcc/chaingun_stand_fire.md5anim"
	/// ->
	/// "models_md5_monsters_zcc_chaingun_stand_fire"
	const AssetID PathToAssetID(
		const char* relative_filepath
		);

	ERet composeFilePath(
		String &fullpath_
		, const char* relative_path
		, const Doom3AssetsConfig& doom3
		);

	/// returns true if this asset should be compiled into a separate file,
	/// i.e. not bundled with other assets of the same type.
	/// E.g. "rocketlauncher_materials_$bundle.mtr" will be compiled to 
	/// "rocketlauncher_materials_$bundle.AssetBundle",
	/// while "weapons.mtr" will be added to "doom3_materials.AssetBundle".
	/// 
	bool shouldBeCompiledSeparately( const char* source_file_path );

}//namespace Doom3
