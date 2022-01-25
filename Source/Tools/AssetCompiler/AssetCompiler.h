#pragma once

#include <Core/Assets/AssetID.h>
//
#include <AssetCompiler/AssetDatabase.h>	// CompiledAssetData


namespace SON
{
	struct Node;
}


namespace AssetBaking
{
	struct AssetPipelineConfig;


	///
	struct AssetCompilerInputs
	{
		AssetID			asset_id;// id of this asset
		FilePathStringT	path;	// absolute path to the source file
		AReader &		reader;	// source file reader stream

		const AssetPipelineConfig& cfg;
		AssetDatabase & asset_database;

		const SON::Node* const asset_build_rules_parse_node;

	public:
		AssetCompilerInputs(
			const char* source_file_abs_path
			, AReader & source_file_reader
			, const AssetPipelineConfig& config
			, AssetDatabase & asset_database
			, SON::Node* asset_build_rules_parse_node
			)
			: reader( source_file_reader )
			, cfg( config )
			, asset_database( asset_database )
			, asset_build_rules_parse_node(asset_build_rules_parse_node)
		{
			asset_id = AssetID_from_FilePath( source_file_abs_path );
			Str::CopyS( path, source_file_abs_path );
		}
	};

	///
	struct AssetCompilerOutputs: CompiledAssetData
	{
		/// valid if the user wants to change the name of the output asset file
		AssetID	override_asset_id;

		// valid if the asset should be saved on disk and added to the database
		const TbMetaClass *	asset_type;

	public:
		AssetCompilerOutputs( AllocatorI & blob_storage = MemoryHeaps::temporary() )
			: CompiledAssetData( blob_storage )
			, asset_type( nil )
		{}
	};


	/// Asset/Resource compilers register themselves automatically upon initialization.
	/// AssetCompilerI derives from AObject to provide factory functionality (create-by-name).
	struct AssetCompilerI: AObject, NonCopyable
	{
		mxDECLARE_ABSTRACT_CLASS( AssetCompilerI, AObject );
		mxDECLARE_REFLECTION;

		virtual ~AssetCompilerI()
		{
		}

		// e.g. a shader compiler might create an OpenGL context for compiling shaders,
		// a material compiler might register Lua functions for parsing expressions in uniform parameters
		virtual ERet Initialize()
		{
			return ALL_OK;
		}

		virtual void Shutdown()
		{
		}

		/// returns the type of compiled assets,
		/// or NULL, if this compiler does not produce a compiled asset.
		virtual const TbMetaClass* getOutputAssetClass() const
		{
			return nil;
		}

		virtual ERet CompileAsset(
			const AssetCompilerInputs& inputs,
			AssetCompilerOutputs &outputs
		) = 0;
	};

	template< class RESOURCE >
	struct CompilerFor: AssetCompilerI
	{
		virtual const TbMetaClass* getOutputAssetClass() const override
		{
			return &RESOURCE::metaClass();
		}
	};

	template< class RESOURCE >
	struct MemoryImageWriterFor: CompilerFor< RESOURCE >
	{
		virtual ERet CompileAsset(
			const AssetCompilerInputs& inputs,
			AssetCompilerOutputs &_outputs
			) override
		{
			RESOURCE	declarations;
			mxDO(SON::Load(
				inputs.reader
				, declarations
				, MemoryHeaps::temporary()
				, inputs.asset_id.d.c_str()
				));
			mxDO(Serialization::SaveMemoryImage( declarations, NwBlobWriter(_outputs.object_data) ));
			return ALL_OK;
		}
	};



	struct AssetBuildRules: CStruct
	{
		String128	compiler;
	public:
		mxDECLARE_CLASS( AssetBuildRules, CStruct );
		mxDECLARE_REFLECTION;
	};

}//namespace AssetBaking
