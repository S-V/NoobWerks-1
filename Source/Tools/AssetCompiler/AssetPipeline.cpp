/*
=============================================================================
Useful references:
Loading Based on Imperfect Data (GDC 2013)
https://s3-us-west-2.amazonaws.com/deplinenoise-misc/LoadingImperfectDataFinal.pdf

Content build tools:
https://github.com/MikePopoloski/ampere
https://github.com/lunt/lunt/wiki/Console
https://github.com/mono/MonoGame/tree/develop/MonoGame.Framework.Content.Pipeline
=============================================================================
*/
#include "stdafx.h"
#pragma hdrstop

#include <Base/Util/PathUtils.h>

#include <Core/Util/ScopedTimer.h>

#include <Scripting/Scripting.h>

#include <Core/Serialization/Text/TxTSerializers.h>

#include <Developer/EditorSupport/DevAssetFolder.h>

#include <AssetCompiler/AssetPipeline.h>
#include <AssetCompiler/AssetCompilers/Graphics/MeshCompiler.h>
#include <AssetCompiler/AssetCompilers/Graphics/MaterialCompiler.h>
#include <AssetCompiler/AssetCompilers/Graphics/ShaderEffectCompiler.h>
#include <AssetCompiler/AssetCompilers/ScriptCompiler.h>
#include <AssetCompiler/AssetCompilers/Graphics/TextureCompiler.h>
#include <AssetCompiler/AssetCompilers/Game/Spaceship_Compiler.h>


namespace
{
	AllocatorI& tempMemHeap() { return MemoryHeaps::temporary(); }
}

mxDEFINE_CLASS( FileAssociation );
mxBEGIN_REFLECTION( FileAssociation )
	mxMEMBER_FIELD( asset_compiler ),
	mxMEMBER_FIELD( extensions ),
mxEND_REFLECTION

mxDEFINE_CLASS( ScriptCompilerConfig );
mxBEGIN_REFLECTION( ScriptCompilerConfig )
	mxMEMBER_FIELD( strip_debug_info ),
mxEND_REFLECTION
ScriptCompilerConfig::ScriptCompilerConfig()
{
	strip_debug_info = true;
}

mxDEFINE_CLASS( TextureCompilerConfig );
mxBEGIN_REFLECTION( TextureCompilerConfig )
	mxMEMBER_FIELD( use_proprietary_texture_formats_instead_of_DDS ),
mxEND_REFLECTION
TextureCompilerConfig::TextureCompilerConfig()
{
	use_proprietary_texture_formats_instead_of_DDS = false;
}

mxDEFINE_CLASS( ShaderCompilerConfig );
mxBEGIN_REFLECTION( ShaderCompilerConfig )
	mxMEMBER_FIELD( path_to_shaderc ),
	mxMEMBER_FIELD( include_directories ),
	mxMEMBER_FIELD( effect_directories ),
	mxMEMBER_FIELD( optimize_shaders ),
	mxMEMBER_FIELD( strip_debug_info ),
	mxMEMBER_FIELD( disassembly_path ),
	mxMEMBER_FIELD( intermediate_path ),
mxEND_REFLECTION
ShaderCompilerConfig::ShaderCompilerConfig()
{
	optimize_shaders = true;
	strip_debug_info = true;
}

mxDEFINE_CLASS( RenderPathCompilerConfig );
mxBEGIN_REFLECTION( RenderPathCompilerConfig )
	mxMEMBER_FIELD( strip_debug_info ),
mxEND_REFLECTION
RenderPathCompilerConfig::RenderPathCompilerConfig()
{
	strip_debug_info = true;
}

namespace AssetBaking
{

/*
-----------------------------------------------------------------------------
	Settings
-----------------------------------------------------------------------------
*/

mxDEFINE_CLASS( AssetPipelineConfig );
mxBEGIN_REFLECTION( AssetPipelineConfig )
	mxMEMBER_FIELD( source_file_filters ),
	mxMEMBER_FIELD( source_paths ),
	mxMEMBER_FIELD( output_path ),
	mxMEMBER_FIELD( asset_database ),
	mxMEMBER_FIELD( file_associations ),
	mxMEMBER_FIELD( tracked_file_extensions ),

	mxMEMBER_FIELD( doom3 ),

	mxMEMBER_FIELD( script_compiler ),
	mxMEMBER_FIELD( texture_compiler ),
	mxMEMBER_FIELD( shader_compiler ),
	mxMEMBER_FIELD( renderpath_compiler ),

	mxMEMBER_FIELD( dump_clumps_to_text ),
	mxMEMBER_FIELD( beep_on_errors ),
mxEND_REFLECTION

AssetPipelineConfig::AssetPipelineConfig()
{
	dump_clumps_to_text = true;
	beep_on_errors = true;
}

void AssetPipelineConfig::fixPathsAfterLoading()
{
	doom3.fixPathsAfterLoading();

	for( UINT i = 0; i < source_paths.num(); i++ )
	{
		Str::NormalizePath( source_paths[i] );
	}

	Str::NormalizePath( output_path );
}


mxDEFINE_CLASS( MetaFile );
mxBEGIN_REFLECTION( MetaFile )
	mxMEMBER_FIELD( dummy_unused_var_just_to_make_size_nonempty ),
mxEND_REFLECTION
MetaFile::MetaFile()
{
	mxZERO_OUT(*this);
}


#if 0
mxDEFINE_CLASS( ImportSettings );
mxBEGIN_REFLECTION( ImportSettings )
	mxMEMBER_FIELD( fileName ),
mxEND_REFLECTION

AssetImporter::AssetImporter()
{
}
AssetImporter::~AssetImporter()
{
}
ERet AssetImporter::Initialize()
{
	//{
	//	m_fileTypes.insert(std::make_pair( "shader",	AssetTypes::SHADER ));

	//	ShaderImporter* shaderImporter = new ShaderImporter();
	//	m_importers.insert(std::make_pair( AssetTypes::SHADER, shaderImporter ));

	//	m_typeIDs.insert(std::make_pair( AssetTypes::SHADER, mxEXTRACT_TYPE_GUID(ShaderCompilerConfig) ));
	//}

#if 0
	{
		m_fileTypes.insert(std::make_pair( "material",	AssetTypes::MATERIAL ));

		MaterialImporter* materialCompiler = new MaterialImporter();
		m_importers.insert(std::make_pair( AssetTypes::MATERIAL, materialCompiler ));

		m_typeIDs.insert(std::make_pair( AssetTypes::MATERIAL, mxEXTRACT_TYPE_GUID(MaterialImportSettings) ));
	}
#endif

	{
		// Collada
		m_fileTypes.insert(std::make_pair( "dae",	AssetTypes::MESH ));
		m_fileTypes.insert(std::make_pair( "xml",	AssetTypes::MESH ));

		// Blender
		m_fileTypes.insert(std::make_pair( "blend",	AssetTypes::MESH ));

		// 3D Studio Max 3DS ( *.3ds )
		m_fileTypes.insert(std::make_pair( "3ds",	AssetTypes::MESH ));
		// 3D Studio Max ASE ( *.ase )
		m_fileTypes.insert(std::make_pair( "ase",	AssetTypes::MESH ));

		// Wavefront Object ( *.obj )
		m_fileTypes.insert(std::make_pair( "obj",	AssetTypes::MESH ));

		// Stanford Polygon Library ( *.ply )
		m_fileTypes.insert(std::make_pair( "ply",	AssetTypes::MESH ));

		// Valve Model ( *.smd,*.vta )
		m_fileTypes.insert(std::make_pair( "smd",	AssetTypes::MESH ));
		m_fileTypes.insert(std::make_pair( "vta",	AssetTypes::MESH ));

		// Quake I ( *.mdl )
		m_fileTypes.insert(std::make_pair( "mdl",	AssetTypes::MESH ));
		// Quake II ( *.md2 )
		m_fileTypes.insert(std::make_pair( "md2",	AssetTypes::MESH ));
		// Quake III ( *.md3 )
		m_fileTypes.insert(std::make_pair( "md3",	AssetTypes::MESH ));
		// Quake 3 BSP ( *.pk3 )
		m_fileTypes.insert(std::make_pair( "pk3",	AssetTypes::MESH ));
		// RtCW ( *.mdc )
		m_fileTypes.insert(std::make_pair( "mdc",	AssetTypes::MESH ));

		// Doom 3 ( *.md5mesh;*.md5anim;*.md5camera )
		m_fileTypes.insert(std::make_pair( "md5mesh",	AssetTypes::MESH ));
		m_fileTypes.insert(std::make_pair( "md5anim",	AssetTypes::MESH ));
		m_fileTypes.insert(std::make_pair( "md5camera",	AssetTypes::MESH ));

		// DirectX X ( *.x )
		m_fileTypes.insert(std::make_pair( "x",		AssetTypes::MESH ));

		// Ogre (*.mesh.xml, *.skeleton.xml, *.material)
		m_fileTypes.insert(std::make_pair( "mesh",	AssetTypes::MESH ));

		// Irrlicht Mesh ( *.irrmesh;*.xml )
		m_fileTypes.insert(std::make_pair( "irrmesh",	AssetTypes::MESH ));
		// Irrlicht Scene ( *.irr;*.xml )
		m_fileTypes.insert(std::make_pair( "irr",	AssetTypes::MESH ));

		// Milkshape 3D ( *.ms3d )
		m_fileTypes.insert(std::make_pair( "ms3d",	AssetTypes::MESH ));



#if 0
		MeshImporter* meshImporter = new MeshImporter();
		m_importers.insert(std::make_pair( AssetTypes::MESH, meshImporter ));
#endif
		m_typeIDs.insert(std::make_pair( AssetTypes::MESH, mxEXTRACT_TYPE_GUID(MeshImportSettings) ));
	}

	return ALL_OK;
}
void AssetImporter::Shutdown()
{
	ImportersByAssetType::iterator it( m_importers.begin() );
	while( it != m_importers.end() )
	{
		delete it->second;
		++it;
	}
	m_importers.clear();
}
AssetTypeT AssetImporter::GetAssetTypeByExtension( const std::string& extension )
{
	// find asset type corresponding to this file extension
	FileTypesByExtension::iterator it( m_fileTypes.find( extension ) );
	if( it == m_fileTypes.end() ) {
		ptWARN("Unknown file extension '%s'\n", extension.c_str());
		return AssetTypes::UNKNOWN;
	}
	return it->second;
}
Importer* AssetImporter::FindImporterByAssetType( const AssetTypeT& assetType )
{
	ImportersByAssetType::iterator it( m_importers.find( assetType ) );
	if( it == m_importers.end() ) {
		return NULL;
	}
	return it->second;
}
Importer* AssetImporter::GetImporterByExtension( const std::string& extension )
{
	AssetTypeT fileType = this->GetAssetTypeByExtension( extension );
	Importer* importer = this->FindImporterByAssetType( fileType );
	if( NULL == importer ) {
		ptWARN("No asset importer for extension '%s'\n", extension.c_str());
	}
	return importer;
}
const TbMetaClass* AssetImporter::GetSettingsTypeForAssetType( const AssetTypeT& assetType )
{
	SettingsTypeByAssetType::iterator it( m_typeIDs.find( assetType ) );
	if( it != m_typeIDs.end() ) {
		return TypeRegistry::FindClassByGuid( it->second );
	}
	return NULL;
}
#endif

AssetProcessor::AssetProcessor()
{
}

AssetProcessor::~AssetProcessor()
{
}

ERet AssetProcessor::Initialize( const AssetPipelineConfig& _settings )
{
	mxDO(NwScriptingEngineI::Create(
		_scripting._ptr
		, MemoryHeaps::scripting()
		));

	_RegisterAssetCompilersFromFileExtensions(_settings);
	_RegisterCustomAssetCompilers();

	for( U32 i = 0; i < m_registered_compilers.num(); i++ )
	{
		mxTRY(m_registered_compilers[i]->Initialize());
	}

	return ALL_OK;
}

void AssetProcessor::Shutdown()
{
	for( int i = 0; i < m_registered_compilers.num(); i++ )
	{
		m_registered_compilers[i]->Shutdown();
	}
	Arrays::deleteContents( m_registered_compilers );

	m_compilers_by_file_extension.clear();

	//
	NwScriptingEngineI::Destroy(_scripting);
	_scripting = nil;
}

void AssetProcessor::_RegisterAssetCompilersFromFileExtensions( const AssetPipelineConfig& _settings )
{
	for( U32 i = 0; i < _settings.file_associations.num(); i++ )
	{
		const FileAssociation& file_assoc = _settings.file_associations[ i ];

		const TbMetaClass* asset_compiler_class =
			TypeRegistry::FindClassByName( file_assoc.asset_compiler.c_str() );

		if( asset_compiler_class && asset_compiler_class->IsDerivedFrom(AssetCompilerI::metaClass()) )
		{
			LOG_TRACE("Registering '%s'...", file_assoc.asset_compiler.c_str());

			AssetCompilerI* asset_compiler = UpCast< AssetCompilerI >( asset_compiler_class->CreateInstance() );
			mxASSERT_PTR(asset_compiler);
			mxASSERT(NULL == this->findCompilerByClass( *asset_compiler_class ));

			m_registered_compilers.add(asset_compiler);

			for( U32 j = 0; j < file_assoc.extensions.num(); j++ )
			{
				m_compilers_by_file_extension.insert(std::make_pair( file_assoc.extensions[j], asset_compiler ));
			}
		}
		else
		{
			ptERROR(
				"No asset compiler class named '%s'!",
				file_assoc.asset_compiler.c_str()
				);
		}
	}
}

void AssetProcessor::_RegisterCustomAssetCompilers()
{
	m_registered_compilers.add(new Spaceship_Compiler());
}

AssetCompilerI* AssetProcessor::findCompilerByFileName( const char* _filename )
{
	const char* file_extension = Str::FindExtensionS( _filename );
	if( !file_extension ) {
		return NULL;
	}

	CompilersByExtension::iterator	it( m_compilers_by_file_extension.find( Chars(file_extension,_InitSlow) ) );
	if( it == m_compilers_by_file_extension.end() ) {
		//ptWARN("No asset asset_compiler for files with extension '%s'\n", file_extension );
		return NULL;
	}
	return it->second;
}

AssetCompilerI* AssetProcessor::findCompilerByClass( const TbMetaClass& asset_compiler_class )
{
	for( U32 i = 0; i < m_registered_compilers.num(); i++ )
	{
		if( m_registered_compilers[i]->IsInstanceOf( asset_compiler_class ) )
		{
			return m_registered_compilers[i];
		}
	}
	return NULL;
}

namespace
{
	enum { HASH_BUF_SIZE = 4096 };
}//namespace

AssetDataHash computeHashFromAssetData( const CompiledAssetData& compiled_asset_data )
{
	const AssetDataHash part0 = computeHashFromData(
		compiled_asset_data.object_data.raw(),
		compiled_asset_data.object_data.rawSize()
		);

	if( !compiled_asset_data.stream_data.IsEmpty() )
	{
		const AssetDataHash part1 = computeHashFromData(
			compiled_asset_data.stream_data.raw(),
			compiled_asset_data.stream_data.rawSize()
			);
		return part0 ^ part1;
	}

	return part0;
}

AssetDataHash computeHashFromData( const void* data, const size_t size )
{
	U64 result = 0;

	//NOTE: must be the same hashing process as used for files
	//mxASSERT(result == MurmurHash64( data, size, seed ));	//<= fails

	size_t numBytesHashed = 0;

	while( numBytesHashed < size )
	{
		const size_t numBytesToHash = smallest( size - numBytesHashed, HASH_BUF_SIZE );

		const void* memoryToHash = mxAddByteOffset( data, numBytesHashed );

		result = MurmurHash64( memoryToHash, numBytesToHash, result );

		numBytesHashed += numBytesToHash;
	}

	return result;
}

AssetDataHash computeFileHash( FileReader & file )
{
	const size_t prevPos = file.Tell();

	file.Seek(0);

	U64 result = 0;

	const size_t fileSize = file.Length();

	BYTE buffer[HASH_BUF_SIZE];

	size_t numBytesHashed = 0;

	while( numBytesHashed < fileSize )
	{
		const size_t numBytesToRead = smallest( fileSize - numBytesHashed, sizeof(buffer) );

		file.Read( buffer, numBytesToRead );

		result = MurmurHash64( buffer, numBytesToRead, result );

		numBytesHashed += numBytesToRead;
	}

	file.Seek(prevPos);

	return result;
}

const SourceFileInfo ComputeFileInfo(
							   FileReader & source_file_reader,
							   const int changeDetection = ChangeDetection::All
							   )
{
	SourceFileInfo	fileInfo;

	if( changeDetection & ChangeDetection::Length )
	{
		const size_t length = source_file_reader.Length();
		fileInfo.length = length;
	}

	if( changeDetection & ChangeDetection::Timestamp )
	{
		FileTimeT sourceTimeStamp;
		{
			FILETIME ftCreate, ftAccess, ftWrite;
			{
				const FileHandleT fileHandle = source_file_reader.GetFileHandle();	
				mxASSERT( ::GetFileTime( fileHandle, &ftCreate, &ftAccess, &ftWrite ) );
			}
			sourceTimeStamp.time = ftWrite;
		}
		fileInfo.timestamp = sourceTimeStamp;
	}

	if( changeDetection & ChangeDetection::Hash )
	{
		const U64 checksum = computeFileHash(source_file_reader);
		fileInfo.checksum = checksum;
	}

	return fileInfo;
}

static
bool Check_If_File_Has_Changed(
	FileReader &source_file_reader,
	const SourceFileInfo& oldFileInfo,
	const SourceFileInfo& _newFileInfo,
	const int changeDetection /*= ChangeDetection::All*/
)
{
	mxASSERT(source_file_reader.IsOpen());
	mxASSERT(&_newFileInfo != &oldFileInfo);

	if( changeDetection == ChangeDetection::None ) {
		return true;
	}

	if( (changeDetection & ChangeDetection::Length) )
	{
		if( _newFileInfo.length != oldFileInfo.length )
		{
			LOG_TRACE("File length differs (%s)", source_file_reader.DbgGetName());
			return true;
		}
	}
	if( (changeDetection & ChangeDetection::Timestamp) )
	{
		//if( _newFileInfo.source_file.length != oldFileInfo.source_file.length )
		if( OS::IO::AisNewerThanB( _newFileInfo.timestamp, oldFileInfo.timestamp ) )
		{
			LOG_TRACE("File timestamp differs (%s)", source_file_reader.DbgGetName());
			return true;
		}
	}
	if( (changeDetection & ChangeDetection::Hash) )
	{
		if( _newFileInfo.checksum != oldFileInfo.checksum )
		{
			LOG_TRACE("File hash differs (%s)", source_file_reader.DbgGetName());
			return true;
		}
	}

	return false;
}

ERet AssetProcessor::CompileAsset(
								  const char* source_file_path,
								  const AssetPipelineConfig& config,
								  AssetDatabase & asset_database,
								  const BuildSettings& settings
										)
{
	if( !config.source_file_filters.IsEmpty() )
	{
		bool ignore_this_file = true;

		nwFOR_EACH( const String& filter, config.source_file_filters )
		{
			if( strstr( source_file_path, filter.c_str() ) ) {
				ignore_this_file = false;
				break;
			}
		}

		if( ignore_this_file ) {
			//DEVOUT("Ignoring '%s', because it's not included in filters.", source_file_path);
			return ALL_OK;
		}
	}


	//
	AssetBaking::AssetCompilerI* asset_compiler = nil;







	// Check if the asset has a "Custom Build Rules" file.

	SON::Node* asset_build_rules_parse_node = nil;

	SON::NodeAllocator	asset_build_file_parse_node_pool(
		MemoryHeaps::temporary()
		);

	NwBlob	asset_build_rules_file_buf(MemoryHeaps::temporary());


	//
	const char* BUILD_RULES_FILE_EXTENSION = "son";



	if(!Str::HasExtensionS(source_file_path, BUILD_RULES_FILE_EXTENSION))
	{
		FilePathStringT	asset_build_rules_file_path;
		Str::CopyS(asset_build_rules_file_path, source_file_path);
		Str::StripFileExtension(asset_build_rules_file_path);
		Str::setFileExtension(asset_build_rules_file_path, BUILD_RULES_FILE_EXTENSION);

		//
		AssetBuildRules	asset_build_rules;

		if(mxSUCCEDED(NwBlob_::loadBlobFromFile(
			asset_build_rules_file_buf
			, asset_build_rules_file_path.c_str()
			)))
		{
			asset_build_rules_parse_node = SON::ParseTextBuffer(
				asset_build_rules_file_buf.raw(), asset_build_rules_file_buf.rawSize()
				, asset_build_file_parse_node_pool
				, asset_build_rules_file_path.c_str()
				);
			mxASSERT_PTR(asset_build_rules_parse_node);
		}

		if(asset_build_rules_parse_node != nil)
		{
			mxDO(SON::Decode(asset_build_rules_parse_node, asset_build_rules));

			const TbMetaClass* metaclass = TypeRegistry::FindClassByName( asset_build_rules.compiler.c_str() );
			if(metaclass && metaclass->IsDerivedFrom(AssetCompilerI::metaClass()))
			{
				asset_compiler = this->findCompilerByClass(*metaclass);
				mxASSERT2(asset_compiler, "no registered asset compiler of class '%s'", metaclass->GetTypeName());
			}
			else
			{
				mxDBG_UNREACHABLE2("couldn't find asset compiler class named '%s'", asset_build_rules.compiler.c_str());
			}
		}
	}


	// Try to find the compiler by file extension.
	if(!asset_compiler) {
		asset_compiler = this->findCompilerByFileName( source_file_path );
	}

	//
	const TbMetaClass* output_asset_type = asset_compiler ? asset_compiler->getOutputAssetClass() : nil;

	const char* output_asset_typename = output_asset_type ? output_asset_type->GetTypeName() : nil;


	//NOTE: we cannot ignore, say, HLSL files, because shaders and materials depend on them
	//if( !asset_compiler ) {
	//	LogStream(LL_Trace) << "Ignoring file '" << source_file_path << "' of unknown type";
	//	return ALL_OK;
	//}

	//LogStream(LL_Trace) << "File changed: '" << source_file_path;


	//
	const AssetID source_asset_id = MakeAssetID( source_file_path );
	mxENSURE(AssetId_IsValid(source_asset_id), ERR_INVALID_PARAMETER,
		"Couldn't get AssetID from path '%s'", source_file_path
		);

	//
	AssetInfo existing_asset_info;

	//
	DynamicArray< AssetInfo >	existing_assets_with_this_id( tempMemHeap() );
	mxTRY(asset_database.findAllAssetsWithId(
		source_asset_id
		, output_asset_type
		, existing_assets_with_this_id
		));

	// If the asset's type is known and we found several assets of this type, there must be an error:
	if( output_asset_type && !existing_assets_with_this_id.IsEmpty() )
	{
		mxENSURE(existing_assets_with_this_id.num() == 1, ERR_UNKNOWN_ERROR,
			"[Consistency Failure] There must be only one asset '%s' ('%s') of type '%s', but got %d"
			, AssetId_ToChars(source_asset_id)
			, source_file_path
			, output_asset_typename ? output_asset_typename : "<unknown>"
			, existing_assets_with_this_id.num()
			);
		existing_asset_info = existing_assets_with_this_id.getFirst();
	}





	// is the file registered in the asset database?
	const bool asset_already_exists_in_database = existing_asset_info.IsValid();



	FileReader	source_file_reader;
	mxTRY(source_file_reader.Open(source_file_path));


	const SourceFileInfo	source_file_info = ComputeFileInfo( source_file_reader );

	//
	bool ignore_this_file = (asset_compiler == nil);

	bool rebuild_this_asset = false;




	{
		String32	extension;
		Str::CopyS( extension, Str::FindExtensionS(source_file_path) );

		// if the file is not an asset, but there may be assets that depend on it (e.g. it's a *.hlsli file)
		if( config.tracked_file_extensions.contains( extension ) ) {
			ignore_this_file = false;
		}

		if( !asset_already_exists_in_database && !ignore_this_file )
		{
			// this file has been changed AND it may have dependencies (though it may not be an asset)
			rebuild_this_asset = true;
//			LogStream(LL_Trace) << source_file_path << " changed.";
		}

		//TODO: check if the output file was deleted
	}



	bool this_asset_was_rebuilt = false;

	//
	if( !ignore_this_file )
	{
//LogStream(LL_Trace) << "Processing file '" << source_file_path;

		if( asset_already_exists_in_database )
		{
			if( settings.inputChangeDetection != ChangeDetection::None )
			{
				const bool source_file_has_changed = Check_If_File_Has_Changed(
					source_file_reader
					, existing_asset_info.source_file
					, source_file_info
					, settings.inputChangeDetection
					);

				// Ok, the output file exists, check if it must be rebuilt.
				rebuild_this_asset = source_file_has_changed;
			}
			else
			{
				rebuild_this_asset = true;
			}
		}
		else
		{
			// a new asset of a known type has been created
			if( asset_compiler && output_asset_type ) {
				rebuild_this_asset = true;
			}
		}


		// Compile asset.
		if( asset_compiler )
		{
const char* destination_folder = config.output_path.c_str();
// make sure the destination directory exists
if( !OS::IO::FileOrPathExists( destination_folder ) )
{
	mxENSURE(OS::IO::MakeDirectory( destination_folder ), ERR_FAILED_TO_CREATE_FILE,
		"Couldn't create directory '%s'", destination_folder
		);
}

			if( rebuild_this_asset )
			{
				AssetCompilerInputs	compile_asset_input(
					source_file_path
					, source_file_reader
					, config
					, asset_database
					, asset_build_rules_parse_node
					);

				AssetCompilerOutputs	compile_asset_output;

				const ERet retCode = asset_compiler->CompileAsset( compile_asset_input, compile_asset_output );
				if(mxFAILED(retCode)) {
					LogStream(LL_Warn) << "Failed to compile asset: " << source_file_path;
					if( config.beep_on_errors ) {
						mxBeep(512);
					}
					return retCode;
				}

				//LogStream(LL_Info) << GetCurrentTimeString<String32>(':') << ": Compiled '" << output_asset_typename"' '" << source_asset_id.c_str();

				//
				if( compile_asset_output.asset_type )
				{
					output_asset_type = compile_asset_output.asset_type;
				}

				if( output_asset_type )
				{
					AssetInfo uptodate_asset_info;
					{
						if(compile_asset_output.override_asset_id.IsValid()) {
							uptodate_asset_info.id = compile_asset_output.override_asset_id;
						} else {
							uptodate_asset_info.id = source_asset_id;
						}

						mxTRY(Str::CopyS( uptodate_asset_info.class_name, output_asset_type->GetTypeName() ));

						Str::CopyS( uptodate_asset_info.source_file.path, source_file_path );
						uptodate_asset_info.source_file.length = source_file_info.length;
						uptodate_asset_info.source_file.timestamp = source_file_info.timestamp;
						uptodate_asset_info.source_file.checksum = source_file_info.checksum;
					}


					if( asset_already_exists_in_database ) {
						mxTRY(asset_database.updateAsset( uptodate_asset_info, compile_asset_output ));
					} else {
						mxTRY(asset_database.addNewAsset( uptodate_asset_info, compile_asset_output ));
					}
				}

				this_asset_was_rebuilt = true;

				//if( listener )
				//{
				//	DBGOUT("Asset changed: '%s'", AssetId_ToChars(source_asset_id));
				//	listener->OnSourceAssetChanged( source_asset_id );
				//}
			}
			else
			{
				//LogStream(LL_Info) << GetCurrentTimeString<String32>(':')
				//	<< ": Skipping up-to-date '" << sourceFileName << "' of type " << GetTypeOf_AssetTypeT().FindString(assetType);
			}
		}
		else
		{
			//LogStream(LL_Trace) << "Ignoring file '" << _file_path << "' of unknown type";
		}

	}//if not ignore_this_file



	// Rebuild dependent assets.

	if( this_asset_was_rebuilt )
	{
		if( settings.rebuildDependencies )
		{
			TInplaceArray< String64, 32 > dependentFiles;
			mxTRY(asset_database.getDependentFileNames( source_file_path, dependentFiles ));

			for( U32 iDependent = 0; iDependent < dependentFiles.num(); iDependent++ )
			{
				const String64& dependendent = dependentFiles[ iDependent ];
				LOG_TRACE("Rebuilding '%s' (depends on '%s')",
					Str::GetFileName(dependendent.c_str()), AssetId_ToChars(source_asset_id)
					);

				BuildSettings	newBuildSettings = settings;
				// Recompile regardless of whether the dependent file has changed or not
				newBuildSettings.inputChangeDetection = ChangeDetection::None;	// force rebuild
				newBuildSettings.outputChangeDetection = ChangeDetection::None;

				//String256	full_path_to_dependent_asset;
				//mxTRY(Slow_ComputeAbsoluteFilePath( config, dependendent.c_str(), full_path_to_dependent_asset ));

				this->CompileAsset(
					dependendent.c_str()
					, config
					, asset_database
					, newBuildSettings
					);
			}
		}
	}

	return ALL_OK;
}

ERet AssetProcessor::CompileAsset2(
	const char* source_file_path,
	const AssetPipelineConfig& config,
	AssetDatabase & asset_database
	)
{
	AssetBaking::AssetCompilerI* asset_compiler = this->findCompilerByFileName( source_file_path );
	mxENSURE(asset_compiler, ERR_OBJECT_NOT_FOUND, "");

	//
	FileReader	source_file_reader;
	mxTRY(source_file_reader.Open(source_file_path));

	//
	mxDO(OS::IO::MakeDirectoryIfDoesntExist2( config.output_path.c_str() ));

	//
	AssetCompilerInputs	compile_asset_input(
		source_file_path
		, source_file_reader
		, config
		, asset_database
		, nil
		);

	//
	AssetCompilerOutputs	compile_asset_output;

	const ERet retCode = asset_compiler->CompileAsset( compile_asset_input, compile_asset_output );
	if(mxFAILED(retCode)) {
		LogStream(LL_Warn) << "Failed to compile asset: " << source_file_path;
		if( config.beep_on_errors ) {
			mxBeep(512);
		}
		return retCode;
	}

	return ALL_OK;
}

ERet AssetProcessor::ProcessChangedFile(
										const char* source_file_path,
										const AssetPipelineConfig& config,
										AssetDatabase & asset_database,
										const BuildSettings& settings
										)
{
	return CompileAsset(
		source_file_path
		, config
		, asset_database
		, settings
		);
}

ERet AssetProcessor::BuildAssets(
								 const AssetPipelineConfig& config
								 , AssetDatabase & asset_database
								 )
{
	LogStream(LL_Info) << "Building all assets and saving to " << config.output_path.c_str();
	ScopedTimer time_to_build_assets;

	// make sure the destination directory exists
	if( !OS::IO::FileOrPathExists( config.output_path.c_str() ) )
	{
		bool success = OS::IO::MakeDirectory( config.output_path.c_str() );
		//bool success = !boost::filesystem::create_directories( boost::filesystem::path(QStringToANSI(outputFileFolder)) );
		if( !success )
		{
			return ERR_FAILED_TO_CREATE_FILE;
		}
	}

	struct FileProcessor: ADirectoryWalker
	{
		AssetProcessor & compiler;
		const AssetPipelineConfig& config;
		AssetDatabase & asset_database;
		bool hadErrors;

		U32 numCompiledAssets;

	public:
		FileProcessor( const AssetPipelineConfig& _config, AssetProcessor& _compiler, AssetDatabase & asset_database )
			: config( _config )
			, compiler(_compiler)
			, asset_database( asset_database )
			, hadErrors(false)
		{
			numCompiledAssets = 0;
		}

		// return true to exit immediately
		virtual bool Found_File( const SFindFileInfo& found ) override
		{
			BuildSettings	buildSettings;
			buildSettings.rebuildDependencies = false;

			const ERet result = compiler.CompileAsset(
				found.fullFileName.c_str()
				, config
				, asset_database
				, buildSettings
				);

			if(mxFAILED( result )) {
				hadErrors = true;
			} else {
				numCompiledAssets++;
			}
			return false;
		}
	};

	FileProcessor	callback( config, *this, asset_database );

	for( UINT i = 0; i < config.source_paths.num(); i++ )
	{
		Win32_ProcessFilesInDirectory( config.source_paths[i].c_str(), &callback );
	}

	//
	mxDO(Doom3_AssetBundles::Get().saveAndClose( asset_database ));

	//
	LogStream(LL_Info)
		<< "Compiled " << callback.numCompiledAssets << " assets in " << time_to_build_assets.ElapsedSeconds() << " seconds"
		;

	if( callback.hadErrors && config.beep_on_errors ) {
		mxBeep(500);
	}

	return ALL_OK;
}

IAssetBundler::~IAssetBundler() {}

	namespace Utils
	{
		void fillInAssetInfo(
			AssetInfo *asset_info_
			, const AssetID& asset_id
			, const TbMetaClass& asset_class
			, const CompiledAssetData& compiled_asset
			)
		{
			mxASSERT(AssetId_IsValid(asset_id));

			asset_info_->id = asset_id;
			Str::CopyS( asset_info_->class_name, asset_class.GetTypeName() );

			//
			asset_info_->source_file.length = 0
				+ compiled_asset.object_data.rawSize()
				+ compiled_asset.stream_data.rawSize()
				;

			asset_info_->source_file.timestamp = FileTimeT::CurrentTime();

			asset_info_->source_file.checksum = computeHashFromAssetData( compiled_asset );
		}

	}//namespace Utils

}//namespace AssetBaking
