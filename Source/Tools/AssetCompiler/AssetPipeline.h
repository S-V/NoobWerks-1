#pragma once

#include <string>

//not present in Visual C++ 2008
//#include <unordered_map>
#include <map>
#define unordered_map map


#include <AssetCompiler/BuildConfig.h>


class NwScriptingEngineI;


const ELogLevel ASSET_PIPELINE_LOG_LEVEL = ELogLevel::LL_Warn;


#include <GlobalEngineConfig.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Filesystem.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include <Developer/RendererCompiler/Effect_Compiler.h>
#include <AssetCompiler/AssetDatabase.h>
#include <AssetCompiler/AssetCompiler.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_AssetUtil.h>

///
struct FileAssociation: CStruct
{
	/// class name
	String64	asset_compiler;

	/// supported file extensions
	TInplaceArray< String32, 16 >	extensions;

public:
	mxDECLARE_CLASS( FileAssociation, CStruct );
	mxDECLARE_REFLECTION;
};

///
struct ScriptCompilerConfig: CStruct
{
	/// Remove debug information from compiled bytecode?
	bool	strip_debug_info;

public:
	mxDECLARE_CLASS( ScriptCompilerConfig, CStruct );
	mxDECLARE_REFLECTION;
	ScriptCompilerConfig();
};

///
struct TextureCompilerConfig: CStruct
{
	bool	use_proprietary_texture_formats_instead_of_DDS;

public:
	mxDECLARE_CLASS( TextureCompilerConfig, CStruct );
	mxDECLARE_REFLECTION;
	TextureCompilerConfig();
};

///
struct ShaderCompilerConfig: CStruct
{
	/// path to BGFX's shader compiler
	String128	path_to_shaderc;

	/// paths for opening included shader source files (e.g. "*.h", "*.hlsl")
	TInplaceArray< FilePathStringT, 16 >	include_directories;

	/// paths for opening effect files ("*.fx") (e.g. when compiling materials)
	TInplaceArray< FilePathStringT, 16 >	effect_directories;

	bool	optimize_shaders;
	bool	strip_debug_info;	//!< remove metadata and string names?

	/// folder for storing shader metadata (for debugging)
	String256	intermediate_path;

	/// folder where disassembled shaders will be stored
	String256	disassembly_path;

public:
	mxDECLARE_CLASS( ShaderCompilerConfig, CStruct );
	mxDECLARE_REFLECTION;
	ShaderCompilerConfig();
};

///
struct RenderPathCompilerConfig: CStruct
{
	bool	strip_debug_info;

public:
	mxDECLARE_CLASS( RenderPathCompilerConfig, CStruct );
	mxDECLARE_REFLECTION;
	RenderPathCompilerConfig();
};

namespace AssetBaking
{
	class AssetProcessor;

	enum Verbosity
	{
		Quiet,
		Minimal,
		Normal,
		Verbose,
		Diagnostic,
	};

	enum ChangeDetection
	{
		None		= 0,	//!< always recompile
		Length		= BIT(1),	//!< compare file length
		Timestamp	= BIT(2),
		Hash		= BIT(3),
		//Version		= BIT(4),
		All			= BITS_ALL
	};

	struct BuildSettings
	{
		ChangeDetection	inputChangeDetection;
		ChangeDetection	outputChangeDetection;
		bool			rebuildDependencies;
	public:
		BuildSettings()
		{
			inputChangeDetection = ChangeDetection::All;
			outputChangeDetection = ChangeDetection::All;
			rebuildDependencies = true;
		}
	};

	/////
	//struct AssetListener
	//{
	//	virtual void OnSourceAssetChanged( const AssetID& asset_id ) = 0;
	//};


	/*
	-----------------------------------------------------------------------------
	asset pipeline config
	contains asset database/processor settings
	-----------------------------------------------------------------------------
	*/
	/// NOTE: asset database is stored in the source assets folder
	struct AssetPipelineConfig: CStruct
	{
		// ignore source files whose filenames do not contain these substrings
		TInplaceArray< String128, 8 >	source_file_filters;

		/// paths to folders with source assets (usually in text form exported from DCC tools)
		TInplaceArray< String256, 4 >	source_paths;

		/// path to folder with binary assets (compiled, optimized, platform-specific blobs)
		String256	output_path;

		/// location of the asset database (file path)
		String256	asset_database;

		/// maps source file extension to asset processor
		TInplaceArray< FileAssociation, 16 >	file_associations;

		/// for updating dependencies
		TInplaceArray< String32, 16 >	tracked_file_extensions;




		/// for stealing assets from Doom 3
		Doom3AssetsConfig	doom3;



		/// asset compilers
		ScriptCompilerConfig	script_compiler;
		TextureCompilerConfig	texture_compiler;
		ShaderCompilerConfig	shader_compiler;

		RenderPathCompilerConfig	renderpath_compiler;



		//
		bool	dump_clumps_to_text;	// for debugging

		bool	beep_on_errors;

	public:
		mxDECLARE_CLASS( AssetPipelineConfig, CStruct );
		mxDECLARE_REFLECTION;
		AssetPipelineConfig();

		void fixPathsAfterLoading();
	};


	//Exporter
	//Importer
	//Converter
	//Compiler
	//Packer


	/// Represents the result of compiling a Doom 3 declaration file (e.g. "*.mtr", "*.sndshd")
	/// which is used for change detection and doesn't actually contain any assets.
	/// The individual assets (e.g. material/shader decls, textures, audio files, etc.) will be packed
	/// into the corresponding "doom3_*.TbDevAssetBundle" files
	/// (e.g. doom3_materials, doom3_sound_shaders, doom3_textures, doom3_audio_files, etc.).
	///
	struct MetaFile: CStruct
	{
		U32	dummy_unused_var_just_to_make_size_nonempty;

	public:
		mxDECLARE_CLASS( MetaFile, CStruct );
		mxDECLARE_REFLECTION;
		MetaFile();
	};

	//
	typedef std::unordered_map< String32, AssetCompilerI* > CompilersByExtension;

	///
	class AssetProcessor: public TGlobal< AssetProcessor >
	{
	public:
		AssetProcessor();
		~AssetProcessor();

		ERet Initialize( const AssetPipelineConfig& _settings );
		void Shutdown();

		/// Determines asset processor by file extension.
		AssetCompilerI* findCompilerByFileName( const char* _filename );

		AssetCompilerI* findCompilerByClass( const TbMetaClass& asset_compiler_class );

		template< class COMPILER >
		ERet findAssetCompiler( COMPILER *&compiler_ )
		{
			AssetCompilerI* o = this->findCompilerByClass( COMPILER::metaClass() );
			mxENSURE(o, ERR_OBJECT_NOT_FOUND, "No compiler of type '%s'", COMPILER::metaClass().GetTypeName());
			compiler_ = UpCast< COMPILER >( o );
			return ALL_OK;
		}

		/// Performs a non incremental build.
		ERet BuildAssets(
			const AssetPipelineConfig& config
			, AssetDatabase & asset_database
			);

		///
		ERet CompileAsset(
			const char* source_file_path,
			const AssetPipelineConfig& config,
			AssetDatabase & asset_database,
			const BuildSettings& settings = BuildSettings()
		);

		ERet CompileAsset2(
			const char* source_file_path,
			const AssetPipelineConfig& config,
			AssetDatabase & asset_database
		);

		///
		ERet ProcessChangedFile(
			const char* source_file_path,
			const AssetPipelineConfig& config,
			AssetDatabase & asset_database,
			const BuildSettings& settings = BuildSettings()
		);

	public:
		NwScriptingEngineI* GetScriptEngine()
		{
			return _scripting;
		}

	private:
		void _RegisterAssetCompilersFromFileExtensions( const AssetPipelineConfig& _settings );
		void _RegisterCustomAssetCompilers();

	private:
		TInplaceArray< AssetCompilerI*, 32 >	m_registered_compilers;
		CompilersByExtension	m_compilers_by_file_extension;
		TPtr< NwScriptingEngineI >	_scripting;
	};


	/*
	----------------------------------------------------------
		IAssetBundler
	----------------------------------------------------------
	*/
	class IAssetBundler: NonCopyable
	{
	public:
		virtual ~IAssetBundler() = 0;

		//
		virtual ERet addAsset(
			const AssetID& asset_id
			, const TbMetaClass& asset_class
			, const CompiledAssetData& asset_data
			, AssetDatabase & asset_database
			) = 0;
	};




	AssetDataHash computeHashFromAssetData( const CompiledAssetData& compiled_asset_data );
	AssetDataHash computeHashFromData( const void* data, const size_t size );
	AssetDataHash computeFileHash( FileReader & file );

	namespace Utils
	{
		void fillInAssetInfo(
			AssetInfo *asset_info_
			, const AssetID& asset_id
			, const TbMetaClass& asset_class
			, const CompiledAssetData& compiled_asset
			);
	}//namespace Utils

	inline
	bool LogLevelEnabled( const ELogLevel this_level )
	{
		return this_level >= ASSET_PIPELINE_LOG_LEVEL;
	}


	template< class RESOURCE >	// where RESOURCE: NwResource
	ERet SaveObjectData(
		const RESOURCE& o
		, NwBlob &object_data
		)
	{
		switch(RESOURCE::SERIALIZATION_METHOD)
		{
		case RESOURCE::Serialization_Unspecified:	// fallthrough
		case RESOURCE::Serialize_as_POD:
			mxDO(NwBlobWriter(object_data).Put(o));
			break;

		case RESOURCE::Serialize_as_MemoryImage:
			mxDO(Serialization::SaveMemoryImage(
				o
				, NwBlobWriter(object_data)
				));
			break;

		case RESOURCE::Serialize_as_BinaryFile:
			mxDO(Serialization::SaveBinary(
				o
				, NwBlobWriter(object_data)
				));
			break;

		case RESOURCE::Serialize_as_Text:
			mxDO(SON::Save(
				o
				, NwBlobWriter(object_data)
				));
			break;
		}
		return ALL_OK;
	}

}//namespace AssetBaking
