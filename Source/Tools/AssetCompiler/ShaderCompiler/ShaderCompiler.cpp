#include "stdafx.h"
#pragma hdrstop
//
#include <Core/Text/Lexer.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Serialization/Serialization.h>
//
#include <Core/Serialization/Text/TxTSerializers.h>
//
#include <AssetCompiler/ShaderCompiler/ShaderCompiler.h>
#include <AssetCompiler/AssetCompilers/Graphics/ShaderEffectCompiler.h>	// MyFileInclude



namespace AssetBaking
{
namespace ShaderCompilation
{

static
ERet ParseTechniquePass( Lexer & lexer, FxShaderPassDesc &newPass_ )
{
	mxDO(expectName( lexer, newPass_.name ));

	mxDO(expectTokenChar( lexer, '{' ));

	//
	if( PeekTokenString( lexer, "compute" ) )
	{
		// this is a compute shader
		SkipToken( lexer );
		mxDO(expectTokenChar( lexer, '=' ));
		mxDO(expectName( lexer, newPass_.entry.compute ));
	}
	else
	{
		// this is a normal, "rendering" shader

		mxDO(expectTokenString( lexer, "vertex" ));
		mxDO(expectTokenChar( lexer, '=' ));
		mxDO(expectName( lexer, newPass_.entry.vertex ));

		if( PeekTokenString( lexer, "geometry" ) ) {
			SkipToken( lexer );
			mxDO(expectTokenChar( lexer, '=' ));
			mxDO(expectName( lexer, newPass_.entry.geometry ));
		}

		if( PeekTokenString( lexer, "pixel" ) ) {
			SkipToken( lexer );
			mxDO(expectTokenChar( lexer, '=' ));
			mxDO(expectName( lexer, newPass_.entry.pixel ));
		}


		// Parse render state.

#if USE_NEW_EFFECT_FORMAT_WHERE_RENDER_STATES_ARE_INLINED

		if( PeekTokenString( lexer, "render_state" ) ) {
			SkipToken( lexer );
			mxDO(expectTokenChar( lexer, '=' ));
			mxDO(expectTokenChar( lexer, '{' ));

			const char* render_state_desc_start = lexer.CurrentPtr();
			mxDO(SkipUntilClosingBrace(lexer));

			mxDO(SON::LoadFromBuffer(
				(char*) render_state_desc_start,
				lexer.CurrentPtr() - render_state_desc_start,
				&newPass_.custom_render_state_desc, newPass_.custom_render_state_desc.metaClass(),
				MemoryHeaps::temporary()
				));

			newPass_.has_custom_render_state = true;
		}

#endif
		if( PeekTokenString( lexer, "state" ) ) {
			SkipToken( lexer );
			mxDO(expectTokenChar( lexer, '=' ));
			mxDO(expectName( lexer, newPass_.state ));
		}


		// Parse shader macros.

		if( PeekTokenString( lexer, "defines" ) )
		{
			SkipToken( lexer );
			mxDO(expectTokenChar( lexer, '=' ));

			mxDO(expectTokenChar( lexer, '[' ));

			TbToken	nextToken;
			while( lexer.PeekToken( nextToken ) )
			{
				if( nextToken == ']' )
				{
					break;
				}

				String32	define_name;
				mxDO(expectName( lexer, define_name ));

				mxDO(newPass_.defines.add( define_name ));
			}

			mxDO(expectTokenChar( lexer, ']' ));
		}

	}

	mxDO(expectTokenChar( lexer, '}' ));

	//
	if(newPass_.state.IsEmpty())
	{
		newPass_.state = "default";
	}

	return ALL_OK;
}

static
ERet parseSampleStateDeclaration( Lexer & lexer, FxShaderEffectDecl &techniqueDecl_ )
{
	FxSamplerStateDecl	sampler_state_decl;

	mxDO(expectName( lexer, sampler_state_decl.name ));

	//
	NwSamplerDescription &sampler_description = sampler_state_decl.desc;

	mxDO(expectTokenChar( lexer, '{' ));

	TbToken	nextToken;
	while( lexer.PeekToken( nextToken ) )
	{
		if( nextToken == '}' )
		{
			break;
		}

		if( nextToken.type != TT_Name )
		{
			return ERR_UNEXPECTED_TOKEN;
		}

		String64	token;
		mxDO(expectName( lexer, token ));
		mxDO(expectTokenChar( lexer, '=' ));

		if( token == "Filter" )
		{
			mxDO(parseEnum( lexer, &sampler_description.filter ));
		}
		//
		else if( token == "AddressU" )
		{
			mxDO(parseEnum( lexer, &sampler_description.address_U ));
		}
		else if( token == "AddressV" )
		{
			mxDO(parseEnum( lexer, &sampler_description.address_V ));
		}
		else if( token == "AddressW" )
		{
			mxDO(parseEnum( lexer, &sampler_description.address_W ));
		}
		//
		else
		{
			tbLEXER_ERROR_BRK( lexer, "unexpected token: '%s'", token.c_str() );
			return ERR_UNEXPECTED_TOKEN;
		}

		mxDO(readOptionalSemicolon( lexer ));
	}

	mxDO(expectTokenChar( lexer, '}' ));
	mxDO(readOptionalSemicolon( lexer ));

	mxDO(techniqueDecl_.sampler_descriptions.add( sampler_state_decl ));

	return ALL_OK;
}

static
ERet parseSamplerBindings( Lexer & lexer, FxShaderEffectDecl &techniqueDecl_ )
{
	mxDO(expectTokenChar( lexer, '{' ));

	TbToken	nextToken;
	while( lexer.PeekToken( nextToken ) )
	{
		if( nextToken == '}' )
		{
			break;
		}

		if( nextToken.type != TT_Name )
		{
			return ERR_UNEXPECTED_TOKEN;
		}

		FxSamplerInitializer	new_sampler_initializer;
		{
			// Read the sampler's name in the HLSL code.
			mxDO(expectName( lexer, new_sampler_initializer.name ));

			mxDO(expectTokenChar( lexer, '=' ));

			//
			String64	sampler_name;
			mxDO(expectName( lexer, sampler_name ));

			//
			const int sampler_state_index = techniqueDecl_.findSamplerStateDeclarationIndexByName( sampler_name.c_str() );
			if( sampler_state_index != INDEX_NONE )
			{
				techniqueDecl_.sampler_descriptions[ sampler_state_index ].is_used = true;

				new_sampler_initializer.sampler_state_index = sampler_state_index;
				new_sampler_initializer.is_built_in_sampler_state = false;
			}
			else
			{
				SamplerID	built_in_sampler_state_id;
				mxDO(GetEnumFromString( &built_in_sampler_state_id, sampler_name.c_str() ));

				new_sampler_initializer.sampler_state_index = built_in_sampler_state_id;
				new_sampler_initializer.is_built_in_sampler_state = true;
			}
		}
		mxDO(techniqueDecl_.sampler_bindings.add( new_sampler_initializer ));
	}

	mxDO(expectTokenChar( lexer, '}' ));

	return ALL_OK;
}

static
ERet ParseTextureBindings( Lexer & lexer, FxShaderEffectDecl &techniqueDecl_ )
{
	mxDO(expectTokenChar( lexer, '{' ));

	TbToken	nextToken;
	while( lexer.PeekToken( nextToken ) )
	{
		if( nextToken == '}' )
		{
			break;
		}

		if( nextToken.type != TT_Name )
		{
			return ERR_UNEXPECTED_TOKEN;
		}

		FxTextureInitializer	new_texture_initializer;
		{
			// Read the texture's name in the HLSL code.
			mxDO(expectName( lexer, new_texture_initializer.name ));

			mxDO(expectTokenChar( lexer, '=' ));

			//
			String64	texture_asset_name;
			mxDO(expectString( lexer, texture_asset_name ));

			new_texture_initializer.texture_asset_name = MakeAssetID(texture_asset_name.c_str());
		}
		mxDO(techniqueDecl_.texture_bindings.add( new_texture_initializer ));
	}

	mxDO(expectTokenChar( lexer, '}' ));

	return ALL_OK;
}

static
ERet parseTechniqueFeature( Lexer & lexer, FxShaderFeatureDecl &newFeature_ )
{
	mxDO(expectName( lexer, newFeature_.name ));

	mxDO(expectTokenChar( lexer, '{' ));

	TbToken	token;

	if( PeekTokenString( lexer, "bits" ) )
	{
		SkipToken(lexer);
		mxDO(expectTokenChar( lexer, '=' ));
		mxDO(ExpectUInt32( lexer, token, newFeature_.bitWidth ));
	}

	if( PeekTokenString( lexer, "default" ) )
	{
		SkipToken(lexer);
		mxDO(expectTokenChar( lexer, '=' ));
		mxDO(ExpectUInt32( lexer, token, newFeature_.defaultValue ));
	}

	mxDO(expectTokenChar( lexer, '}' ));

	return ALL_OK;
}

static
ERet ParseTechniqueDescription( Lexer & lexer, const char* filename, FxShaderEffectDecl &shaderDef_ )
{
	LexerOptions	lexerOptions;
	lexerOptions.m_flags.raw_value = LexerOptions::SkipComments | LexerOptions::IgnoreUnknownCharacterErrors;
	lexer.SetOptions( lexerOptions );

	TbToken	nextToken;
	while( lexer.PeekToken( nextToken ) )
	{
		if( nextToken == "pass" )
		{
			SkipToken( lexer );

			FxShaderPassDesc& newPass = shaderDef_.passes.AddNew();
			mxDO(ParseTechniquePass( lexer, newPass ));
		}
		else if( nextToken == "SamplerState" )
		{
			SkipToken( lexer );
UNDONE;
			mxDO(parseSampleStateDeclaration( lexer, shaderDef_ ));
		}
		else if( nextToken == "samplers" )
		{
			SkipToken( lexer );

			mxDO(parseSamplerBindings( lexer, shaderDef_ ));
		}
		else if( nextToken == "textures" )
		{
			SkipToken( lexer );

			mxDO(ParseTextureBindings( lexer, shaderDef_ ));
		}
		else if( nextToken == "feature" )
		{
			SkipToken( lexer );

			FxShaderFeatureDecl &	newFeature = shaderDef_.features.AddNew();
			mxDO(parseTechniqueFeature( lexer, newFeature ));
		}
		else if( nextToken == "%%" )
		{
			// end of effect declaration is reached
			return ALL_OK;
		}
		else
		{
			tbLEXER_ERROR_BRK( lexer, "unexpected token: '%s'", nextToken.c_str() );
			return ERR_UNEXPECTED_TOKEN;
		}
	}

	return ALL_OK;
}

ERet parseTechniqueDescriptionFromSourceFile( const NwBlob& fileData, const char* filename, FxShaderEffectDecl &shaderDef_ )
{
	Lexer	lexer( fileData.raw(), fileData.rawSize(), filename );

	LexerOptions	lexerOptions;
	lexerOptions.m_flags.raw_value

		// NOTE: don't skip C - style multi-line comments: /* ... */,
		// because they have effect descriptions in them (inside %% ... %% blocks).
		= LexerOptions::SkipSingleLineComments

		| LexerOptions::IgnoreUnknownCharacterErrors
		;

	lexer.SetOptions( lexerOptions );


	while( !lexer.EndOfFile() )
	{
		TbToken	token;
		if( !lexer.ReadToken( token ) ) {
			break;
		}
		//ptPRINT("READ(%d): %s", token.type, token.text.c_str());
		//
		if( token == "%%" )
		{
			mxDO(ParseTechniqueDescription( lexer, filename, shaderDef_ ));

			mxDO(expectTokenString( lexer, "%%" ));
			return ALL_OK;
		}
	}

	tbLEXER_ERROR_BRK( lexer, "failed to parse technique - need a closing '%%'" );

	return ERR_FAILED_TO_PARSE_DATA;
}

ERet validateTechniqueDescription(
								  const FxShaderEffectDecl& effect_decl
								  , const char* source_file_path
								  )
{
	if( effect_decl.passes.IsEmpty() ) {
		ptWARN( "No passes in shader '%s'", source_file_path );
		return ERR_OBJECT_NOT_FOUND;
	}

	//
	nwFOR_EACH( const FxSamplerStateDecl& sampler_state_decl, effect_decl.sampler_descriptions )
	{
		if( !sampler_state_decl.is_used ) {
			ptWARN( "Sampler state '%s' is declared in effect but not used in shader code!",
				sampler_state_decl.name.c_str()
				);
			return ERR_INVALID_PARAMETER;
		}
	}

	return ALL_OK;
}

///
ERet compileEffectFromFile(
							  const char* source_file_path,
							  const ShaderCompilerConfig& settings,
							  const FxShaderEffectDecl& effect_decl,
							  const TArray< FxDefine >& defines,
							  const char* _dependentFilePath,
							  AssetDatabase & assetDatabase

							  ,NwBlob *shaderObjectBlob	// shader object
							  ,NwBlob *shaderBytecodeBlob	// compiled shader bytecode
							  ,ShaderMetadata &shader_metadata_
							  )
{
	mxASSERT(strlen(source_file_path) > 0);

	//
	MyFileInclude	include(_dependentFilePath, &assetDatabase);
	for( U32 i = 0; i < settings.include_directories.num(); i++ ) {
		mxDO(include.AddSearchPath( settings.include_directories[i].c_str() ));
	}

	//
	{
		LogStream log(LL_Info);
		log << GetCurrentTimeString<String32>(':')
			<< " Compiling '" << source_file_path << "'";
		if( defines.num() )
		{
			log << " with";
			log.Flush();
			for( int i = 0; i < defines.num(); i++ )
			{
				ptPRINT("\t%s = %s\n", defines[i].name.c_str(), defines[i].value.c_str());
			}
		}
	}

	//
	FxOptions	compile_settings;
	{
		mxDO(compile_settings.defines.AddMany( defines.raw(), defines.num() ));

		compile_settings.include = &include;

		Str::CopyS( compile_settings.source_file, source_file_path );

		compile_settings.optimize_shaders = settings.optimize_shaders;
		compile_settings.strip_debug_info = settings.strip_debug_info;
		compile_settings.disassembly_path = settings.disassembly_path.c_str();
	}

	///
	class ShaderDatabase: public AShaderDatabase
	{
		AssetDatabase &	_assetDatabase;

	public:
		ShaderDatabase( AssetDatabase & assetDatabase )
			: _assetDatabase( assetDatabase )
		{
		}

		virtual const AssetID addShaderBytecode(
			const NwShaderType::Enum shaderType
			, const void* compiledBytecodeData
			, const size_t compiledBytecodeSize
			) override
		{
			const TbMetaClass& shaderAssetClass = Testbed::Graphics::shaderAssetClassForShaderTypeEnum(
				shaderType
				);

			CompiledAssetData	compiled_shader;
			compiled_shader.object_data.initializeWithExternalStorageAndCount(
				(char*)compiledBytecodeData
				, compiledBytecodeSize
				, compiledBytecodeSize
				);

			AssetID newAssetId;
			_assetDatabase.addOrUpdateGeneratedAsset(
				AssetID()
				, shaderAssetClass
				, compiled_shader
				, &newAssetId
				);

			return newAssetId;
		}

		///
		virtual const AssetID addProgramPermutation(
			const TbProgramPermutation& programPermutation
			) override
		{
			CompiledAssetData	compiled_program;
			Serialization::SaveMemoryImage(
				programPermutation,
				NwBlobWriter(compiled_program.object_data)
				);

			AssetID	newAssetId;
			_assetDatabase.addOrUpdateGeneratedAsset(
				AssetID()
				, NwShaderProgram::metaClass()
				, compiled_program
				, &newAssetId
				);

			return newAssetId;
		}

	private:
		AssetDataHash computeProgramHash( const TbProgramPermutation& programPermutation )
		{
			AssetDataHash combinedHash = 0;
			for( int i = 0; i < mxCOUNT_OF(programPermutation.shaderAssetIds); i++ )
			{
				const AssetID& shaderAssetId = programPermutation.shaderAssetIds[i];
				const U64 shaderAssetNameHash = computeHashFromData( shaderAssetId.d.c_str(), shaderAssetId.d.Length() );
				combinedHash ^= shaderAssetNameHash;
			}

			return combinedHash;
		}
	};

	ShaderDatabase	shaderDatabase( assetDatabase );

	//
	mxTRY(CompileEffect_D3D11(
		compile_settings,
		effect_decl,
		&shaderDatabase,

		shaderObjectBlob,
		shaderBytecodeBlob,
		shader_metadata_
	));

	return ALL_OK;
}

ERet compileEffectFromBlob(
						   const NwBlob& fileData
						   , const char* source_file_path
						   , const ShaderCompilerConfig& shader_compiler_settings
						   , AssetDatabase & asset_database
						   , CompiledAssetData &compiled_effect_data_
							  )
{
	FxShaderEffectDecl	effect_decl;
	mxDO(ShaderCompilation::parseTechniqueDescriptionFromSourceFile(
		fileData
		, source_file_path
		, effect_decl
		));

	//
	mxDO(ShaderCompilation::validateTechniqueDescription(
		effect_decl
		, source_file_path
		));

	//
	ShaderMetadata	shader_metadata;

	mxTRY(ShaderCompilation::compileEffectFromFile(
		source_file_path,
		shader_compiler_settings,
		effect_decl,
		TArray< FxDefine >(),
		source_file_path,
		asset_database

		, &compiled_effect_data_.object_data	// shader object
		, &compiled_effect_data_.stream_data	// compiled shader bytecode
		, shader_metadata
		));

	// TEMP
	mxDO(SON::Save(
		shader_metadata
		, NwBlobWriter(compiled_effect_data_.metadata)
		));

	return ALL_OK;
}

static
ERet loadEffectSourceCodeToBlob(
								const char* source_file_name
								, const ShaderCompilerConfig& shader_compiler_settings
								, NwBlob & fileDataBlob_
								, String256 & path_to_shader_source_file
								, const char* material_file_path = nil
								)
{
	if( material_file_path )
	{
		mxDO(Str::ComposeFilePath(
			path_to_shader_source_file
			, material_file_path
			, source_file_name
			));

		if(mxSUCCEDED(NwBlob_::loadBlobFromFile(
			fileDataBlob_
			, path_to_shader_source_file.c_str()
			)))
		{
			return ALL_OK;
		}
	}

	for( UINT i = 0; i < shader_compiler_settings.effect_directories.num(); i++ )
	{
		const String& search_path = shader_compiler_settings.effect_directories[i];

		mxDO(Str::ComposeFilePath(
			path_to_shader_source_file
			, search_path.c_str()
			, source_file_name
			) );

		if(mxSUCCEDED(NwBlob_::loadBlobFromFile(
			fileDataBlob_
			, path_to_shader_source_file.c_str()
			)))
		{
			return ALL_OK;
		}
	}

	ptWARN("Failed to open shader source file '%s'.", source_file_name);

	return ERR_OBJECT_NOT_FOUND;
}

ERet CompileEffect(
	const char* source_file_name
	, const ShaderCompilerConfig& shader_compiler_settings	// for searching shader source files
	, AssetDatabase & asset_database
	, CompiledAssetData &compiled_effect_data_
	, const char* material_file_path /*= nil*/
	)
{
	String256	path_to_shader_source_file;
	NwBlob		fileDataBlob( MemoryHeaps::temporary() );

	mxDO(loadEffectSourceCodeToBlob(
		source_file_name
		, shader_compiler_settings
		, fileDataBlob
		, path_to_shader_source_file
		, material_file_path
		));

	mxDO(compileEffectFromBlob(
		fileDataBlob
		, path_to_shader_source_file.c_str()
		, shader_compiler_settings
		, asset_database
		, compiled_effect_data_
	));

	return ALL_OK;
}

}//namespace ShaderCompilation
}//namespace AssetBaking
