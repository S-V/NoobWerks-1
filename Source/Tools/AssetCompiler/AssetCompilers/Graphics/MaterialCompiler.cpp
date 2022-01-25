// graphics material compiler
#include "stdafx.h"
#pragma hdrstop

#include <Base/Util/PathUtils.h>

#include <AssetCompiler/AssetCompilers/Graphics/MaterialCompiler.h>

#if WITH_MATERIAL_COMPILER

#include <Core/Text/Lexer.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Serialization/Serialization_Private.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include <Developer/RendererCompiler/Target_Common.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Graphics/Private/shader_effect_impl.h>
#include <Rendering/BuildConfig.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/Material.h>

#include <Scripting/Scripting.h>
#include <Scripting/Lua/FunctionBinding.h>

#include <AssetCompiler/ShaderCompiler/ShaderCompiler.h>
#include <AssetCompiler/AssetCompilers/Graphics/ShaderEffectCompiler.h>
#include <AssetCompiler/AssetCompilers/Graphics/RenderStateCompiler.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_MaterialShaders.h>
#include <AssetCompiler/AssetCompilers/Graphics/_MaterialShaderEffectsCompilation.h>


namespace AssetBaking
{

namespace
{
	AllocatorI& tempMemHeap() { return MemoryHeaps::temporary(); }
}



static const size_t MAX_PER_MATERIAL_COMMAND_BUFFER_SIZE = LLGL_MAX_SHADER_COMMAND_BUFFER_SIZE;

mxDEFINE_CLASS(StringNameValuePair);
mxBEGIN_REFLECTION(StringNameValuePair)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(value),
mxEND_REFLECTION;

mxDEFINE_CLASS( MaterialTextureBinding );
mxBEGIN_REFLECTION( MaterialTextureBinding )
	mxMEMBER_FIELD( name ),
	mxMEMBER_FIELD( asset ),
	//mxMEMBER_FIELD( sampler ),
mxEND_REFLECTION
MaterialTextureBinding::MaterialTextureBinding()
{
	//sampler = BuiltIns::PointSampler;
}

MaterialDef::MaterialDef()
{
}

ERet MaterialDef::parseFrom(
							AReader & file_stream
							, const char* file_name_for_debugging
							)
{
	DynamicArray< char >	source_file_data( MemoryHeaps::temporary() );

	mxDO(Str::loadFileToString(
		source_file_data
		, file_stream
		));

	//
	Lexer	lexer(
		source_file_data.raw()
		, source_file_data.rawSize()
		, file_name_for_debugging
		);

	//
	LexerOptions	lexerOptions;
	lexerOptions.m_flags.raw_value
		= LexerOptions::SkipComments
		| LexerOptions::IgnoreUnknownCharacterErrors
		| LexerOptions::SkipSemicolonComments
		| LexerOptions::SkipSingleVerticalLineComments
		;

	lexer.SetOptions( lexerOptions );

	TbToken	nextToken;
	while( lexer.PeekToken( nextToken ) )
	{
		if( lexer.EndOfFile() )
		{
			lexer.Error(
				"failed to parse material '%s' - end of file reached!"
				, file_name_for_debugging
				);
			return ERR_EOF_REACHED;
		}

		if( nextToken == "shader" )
		{
			SkipToken( lexer );

			mxDO(readOptionalChar( lexer, '=' ));
			mxDO(expectString( lexer, this->effect_filename ));
		}
		else if( nextToken == "features" )
		{
			mxDO(parseShaderFeatures( lexer ));
		}
		else if( nextToken == "parameters" )
		{
			SkipToken( lexer );

			mxDO(parseMaterialParameters( lexer ));
		}


		else if( nextToken == "textures" )
		{
			SkipToken( lexer );

			mxDO(expectTokenChar( lexer, '{' ));

			mxDO(parseTextureBindings( lexer ));

			mxDO(expectTokenChar( lexer, '}' ));
		}

		//deprecated
		else if( nextToken == "resources" )
		{
			SkipToken( lexer );

			mxDO(expectTokenChar( lexer, '=' ));
			mxDO(expectTokenChar( lexer, '[' ));

			mxDO(parseResourceBindings( lexer ));

			mxDO(expectTokenChar( lexer, ']' ));
		}

		else if( nextToken == '%' )
		{
			break;
		}
		else
		{
			return ERR_UNEXPECTED_TOKEN;
		}
	}

	return ALL_OK;
}

ERet MaterialDef::parseShaderFeatures(
	ATokenReader& lexer
	)
{
	mxDO(expectTokenString( lexer, "features" ));

	mxDO(expectTokenChar( lexer, '{' ));

	mxLOOP_FOREVER
	{
		TbToken	nextToken;
		if( !lexer.PeekToken( nextToken ) ) {
			return ERR_EOF_REACHED;
		}
		if( nextToken == '}' ) {
			break;
		}


		FeatureSwitch	feature_switch;
		mxDO(expectName( lexer, feature_switch.key ));

		mxDO(expectTokenChar( lexer, '=' ));

		mxDO(expectInt32( lexer, nextToken, feature_switch.value ));

		mxDO(feature_switches.add( feature_switch ));
	}

	mxDO(expectTokenChar( lexer, '}' ));

	return ALL_OK;
}

ERet MaterialDef::parseMaterialParameters(
	ATokenReader& lexer
	)
{
	mxDO(expectTokenChar( lexer, '{' ));

	//
	StringNameValuePair	uniform_parameter;
	mxDO(expectName( lexer, uniform_parameter.name ));

	mxDO(expectTokenChar( lexer, '=' ));

	String64	uniform_parameter_value;
	mxDO(expectString( lexer, uniform_parameter.value ));


	//
	mxDO(uniform_parameters.add( uniform_parameter ));


	//
	mxDO(expectTokenChar( lexer, '}' ));

	return ALL_OK;
}

ERet MaterialDef::parseTextureBindings(
	ATokenReader& lexer
	)
{
	TbToken	nextToken;
	while( lexer.PeekToken( nextToken ) )
	{
		if( nextToken == '}' )
		{
			return ALL_OK;
		}

		MaterialTextureBinding	resource_binding;
		{
			mxDO(expectName( lexer, resource_binding.name ));

			mxDO(expectTokenChar( lexer, '=' ));

			mxDO(expectString( lexer, resource_binding.asset ));
		}
		mxDO(textures.add( resource_binding ));
	}

	return ALL_OK;
}

ERet MaterialDef::parseResourceBindings(
	ATokenReader& lexer
	)
{
	TbToken	nextToken;
	while( lexer.PeekToken( nextToken ) )
	{
		if( nextToken == ']' )
		{
			return ALL_OK;
		}

		lexer.ReadToken( nextToken );

		if( nextToken == '{' )
		{
			MaterialTextureBinding	resource_binding;

			mxDO(expectTokenString( lexer, "name" ));
			mxDO(expectTokenChar( lexer, '=' ));
			mxDO(expectString( lexer, resource_binding.name ));

			mxDO(expectTokenString( lexer, "asset" ));
			mxDO(expectTokenChar( lexer, '=' ));
			mxDO(expectString( lexer, resource_binding.asset ));

			mxDO(textures.add( resource_binding ));

			mxDO(expectTokenChar( lexer, '}' ));
		}
	}

	return ALL_OK;
}

/*
----------------------------------------------------------
	MaterialCompiler
----------------------------------------------------------
*/
mxDEFINE_CLASS(MaterialCompiler);

namespace
{
	const ShaderCBuffer* findLocalUniformBuffer(
		const ShaderMetadata& shader_metadata
		)
	{
		const ShaderCBuffer* local_uniform_buffer = nil;

		nwFOR_EACH( const ShaderCBuffer& ubo, shader_metadata.cbuffers )
		{
			// global constant buffers are managed internally by the engine
			if( !ubo.IsGlobal() )
			{
				if( local_uniform_buffer ) {
					ptWARN("Material shaders can have only one local uniform buffer!");
					return local_uniform_buffer;
				}
				local_uniform_buffer = &ubo;
			}
		}

		return local_uniform_buffer;
	}

#if 0
	static const U32 MAX_UNIFORM_BUFFER_SIZE = 65536;

	static ERet CopyUniformShaderParameters(
		const ShaderMetadata& shader_metadata,
		TbParameterBuffer &uniforms
		, const ShaderCBuffer **pLocalCBuffer_
		)
	{
		*pLocalCBuffer_ = nil;

		//
		UINT sizeOfUniforms = 0;
		for( UINT iCB = 0; iCB < shader_metadata.cbuffers.num(); iCB++ )
		{
			const ShaderCBuffer& rCBuffer = shader_metadata.cbuffers[ iCB ];
			// global constant buffers are managed internally by the engine
			if( !rCBuffer.IsGlobal() )
			{
				mxASSERT(rCBuffer.size <= MAX_UNIFORM_BUFFER_SIZE);
				sizeOfUniforms += rCBuffer.size;
			}
		}

		const UINT numVectors = sizeOfUniforms / sizeof(V4f);
		uniforms.setNum(numVectors);



		// fill material constant buffers with default data

		UINT bufferOffset = 0;

		UINT	num_local_constant_buffers = 0;

		for( UINT iCB = 0; iCB < shader_metadata.cbuffers.num(); iCB++ )
		{
			const ShaderCBuffer& rCBuffer = shader_metadata.cbuffers[ iCB ];
			if( !rCBuffer.IsGlobal() )
			{
				++num_local_constant_buffers;
				if( num_local_constant_buffers > 1 ) {
					ptWARN("Material shaders can have only one local uniform buffer!");
					return ERR_INVALID_PARAMETER;
				}

				*pLocalCBuffer_ = &rCBuffer;

				void* defaultValue = mxAddByteOffset(uniforms.raw(), bufferOffset);
				if( rCBuffer.defaults.num() ) {					
					memcpy(defaultValue, rCBuffer.defaults.raw(), rCBuffer.size);
				} else {
					MemZero(defaultValue, rCBuffer.size);
				}
				bufferOffset += rCBuffer.size;
			}
		}
		return ALL_OK;
	}
#endif
	static
	const MaterialTextureBinding* findTextureBindingInMaterial(
		const String& resource_name_in_shader
		, const MaterialDef& material_def
		)
	{
		for( UINT iSR = 0; iSR < material_def.textures.num(); iSR++ )
		{
			const MaterialTextureBinding& resource_binding = material_def.textures[iSR];
			if( resource_binding.name == resource_name_in_shader ) {
				return &resource_binding;
			}
		}
		return nil;
	}

	//struct MaterialTextureBinding
	//{
	//	String32	texture_name_in_shader;
	//	String32	texture_asset_name;	// default resource to bind (e.g. "test_diffuse")
	//	U32			bind_slot;
	//};

	static ERet setupMaterialTextureBindings(
		TBuffer< NwShaderTextureRef > &referenced_textures_
		, const NwShaderEffectData& shader_effect_data
		, const ShaderMetadata& shader_metadata
		, const MaterialDef& material_def
		//, TArray< MaterialTextureBinding > &material_texture_bindings
		)
	{
		//TODO: Copy default textures from the shader effect.
		//mxDO(Arrays::Copy( referenced_textures_, shader_effect_data.referenced_textures ));

		DynamicArray< NwShaderTextureRef >	referenced_textures( MemoryHeaps::temporary() );
		mxDO(referenced_textures.reserve( material_def.textures.num() ));

		// Override with the textures from the material description.
		for( UINT i = 0; i < material_def.textures.num(); i++ )
		{
			const MaterialTextureBinding& texture_binding_decl_in_material_decl = material_def.textures[i];

			const String& texture_name = texture_binding_decl_in_material_decl.name;

			const NameHash32 texture_name_hash = GetDynamicStringHash( texture_name.c_str() );

			//
			const int texture_bind_point_index = shader_effect_data.bindings.findTextureBindingIndex( texture_name_hash );

			if( texture_bind_point_index != -1 )
			{
				NwShaderTextureRef texture_ref;
				{
					const AssetID texture_asset_id = Doom3::PathToAssetID( texture_binding_decl_in_material_decl.asset.c_str() );

					texture_ref.texture._setId( texture_asset_id );
					//texture_ref.input_slot = shader_effect_data.bindings.SRs[ texture_bind_point_index ].;
					texture_ref.binding_index = texture_bind_point_index;
				}
				referenced_textures.add( texture_ref );
			}
			else
			{
				ptWARN("The shader '%s' doesn't have a texture named '%s'!"
					, material_def.effect_filename.c_str(), texture_name.c_str()
					);
			}
		}

		mxDO(Arrays::Copy(
			referenced_textures_,
			referenced_textures
			));

		return ALL_OK;
	}
}

static int findPassIndexByNameHash(
	const NameHash32 name_hash,
	const NwShaderEffect::Pass* passes, const UINT num_passes
	)
{
	for( UINT i = 0; i < num_passes; i++ ) {
		if( passes[i].name_hash == name_hash ) {
			return i;
		}
	}
	return INDEX_NONE;
}


static 
void SetupMaterialPassFromEffectPass(
									 Rendering::MaterialPass &material_pass_
									 , const NwShaderEffect::Pass& effect_pass
									 , const int pass_index_in_effect
									 )
{
	material_pass_.render_state.u = 0;
	material_pass_.filter_index = pass_index_in_effect;
	material_pass_.draw_order = 0;
	material_pass_.program.id = effect_pass.default_program_index;
}

static ERet SetupMaterialPasses(
								TBuffer< Rendering::MaterialPass > &material_passes_
								, const NwShaderEffectData& shader_effect_data
								, const ShaderMetadata& shader_metadata
								, const MaterialDef& material_def
								, const NwShaderFeatureBitMask shader_feature_mask
								)
{
	const UINT num_passes_in_effect = shader_effect_data.passes.num();

	const UINT num_filtered_passes = material_def.filtered_passes.num();

	if( num_filtered_passes > 0 )
	{
		mxASSERT(num_filtered_passes <= num_passes_in_effect);

		mxDO(material_passes_.setNum( num_filtered_passes ));

		for( UINT iPass = 0; iPass < num_filtered_passes; iPass++ )
		{
			const MaterialDef::SelectedPass& included_pass = material_def.filtered_passes[ iPass ];

			const int pass_index_in_effect = findPassIndexByNameHash(
				included_pass.shader_pass_id
				, shader_effect_data.passes.raw(), num_passes_in_effect
				);
			mxENSURE( pass_index_in_effect != INDEX_NONE, ERR_OBJECT_NOT_FOUND, "" );

			const NwShaderEffect::Pass& effect_pass = shader_effect_data.passes[ pass_index_in_effect ];

			//
			Rendering::MaterialPass &material_pass = material_passes_[ iPass ];

			SetupMaterialPassFromEffectPass(
				material_pass
				, effect_pass
				, pass_index_in_effect
				);
			material_pass.program.id = shader_feature_mask;

		}//For each pass in material.
	}
	else
	{
		mxDO(material_passes_.setNum( num_passes_in_effect ));

		for( UINT iPass = 0; iPass < num_passes_in_effect; iPass++ )
		{
			const NwShaderEffect::Pass& effect_pass = shader_effect_data.passes[ iPass ];
			//
			Rendering::MaterialPass &material_pass = material_passes_[ iPass ];

			SetupMaterialPassFromEffectPass(
				material_pass
				, effect_pass
				, iPass
				);
			material_pass.program.id = shader_feature_mask;

		}//For each pass in effect.
	}

	return ALL_OK;
}

static
ERet loadShaderDefinition( FxShaderDescription &shaderDef, const char* shader_name, const AssetCompilerInputs& inputs )
{
UNDONE;
const char* SHADER_SOURCE_FILE_EXTENSION = "shader";

	String256	pathToShader;
	mxDO(Str::ComposeFilePath(pathToShader, inputs.path.c_str(), shader_name, SHADER_SOURCE_FILE_EXTENSION));

	if(mxFAILED(SON::LoadFromFile( pathToShader.c_str(), shaderDef, MemoryHeaps::temporary() )))
	{
		bool	loaded_shader_definition = false;

		for( UINT i = 0; i < inputs.cfg.shader_compiler.include_directories.num(); i++ )
		{
			const String& search_path = inputs.cfg.shader_compiler.include_directories[i];

			mxDO(Str::ComposeFilePath(pathToShader, search_path.c_str(), shader_name, SHADER_SOURCE_FILE_EXTENSION));

			if(mxSUCCEDED(SON::LoadFromFile( pathToShader.c_str(), shaderDef, MemoryHeaps::temporary() )))
			{
				loaded_shader_definition = true;
				break;
			}
		}

		if( !loaded_shader_definition )
		{
			return ERR_OBJECT_NOT_FOUND;
		}
	}

	return ALL_OK;
}

int get_uniforms_from_Lua(
						   lua_State *L
						   , F32 *dst_buf
						   )
{
	int a = lua_gettop( L );
	int t0 = lua_type(L, -1);
	int t1 = lua_type(L, 0);
	int t2 = lua_type(L, 1);
	mxASSERT(0);
	return 0;
	//// universal helper function to get Vec3 function argument from Lua to C++ function
	//luaL_checktype(L, idx, LUA_TTABLE);
	//lua_rawgeti(L, idx, 1); vec.x = lua_tonumber(L, -1); lua_pop(L, 1);
	//lua_rawgeti(L, idx, 2); vec.y = lua_tonumber(L, -1); lua_pop(L, 1);
	//lua_rawgeti(L, idx, 3); vec.z = lua_tonumber(L, -1); lua_pop(L, 1);
}

#if 0
namespace
{
	ERet createNewMaterialEffect_IfNeeded(
		AssetID &materialEffectId_
		, const CompiledAssetData& compiled_shader_effect
		, AssetDatabase & asset_database
		)
	{
		materialEffectId_ = AssetID();

		AssetInfo	existingShaderAssetInfo;
		const TbMetaClass *	existing_shader_asset_class = nil;

		mxDO(asset_database.findExistingAssetWithSameContents(
			existingShaderAssetInfo
			,existing_shader_asset_class
			,compiled_shader_effect
			));

		if( existingShaderAssetInfo.IsValid() && existing_shader_asset_class != nil )
		{
			materialEffectId_ = existingShaderAssetInfo.id;
		}
		else
		{
			const AssetID materialEffectId = asset_database.generateNextAssetId();

			mxDO(asset_database.addOrUpdateGeneratedAsset(
				materialEffectId
				,NwShaderEffect::metaClass()
				,compiled_shader_effect
				));

			materialEffectId_ = materialEffectId;
		}

		return ALL_OK;
	}
}//namespace
#endif




///
ERet BuildMaterialCommandBuffer(
								CommandBufferChunk &cmd_buf_
								,const MaterialDef& material_def
								,const NwShaderEffectData& shader_effect_data
								, const DebugParam& dbgparam
								)
{
	//TEMP TODO
	cmd_buf_.d.empty();

	//mxDO(cmd_buf_.Copy( shader_effect_data.command_buffer ));

if(dbgparam.flag)
{
	cmd_buf_.DbgPrint();
}

#if 0
	for( UINT iSR = 0; iSR < header.numResources; iSR++ )
	{
		const MaterialTextureBinding& material_texture_binding = material_texture_bindings[ iSR ];

		const String& texture_asset_name = material_texture_binding.texture_asset_name;

		// if this is a valid texture name
		if( texture_asset_name.length() > 0 )
		{
			const AssetID assetID = AssetID_from_FilePath( texture_asset_name.c_str() );
			mxDO(writeAssetID( assetID, output_writer ));
		}
		else
		{
			// not a valid texture asset Id - the texture will be left uninitialized (e.g. if it's a shadow map)
			LogStream(LL_Warn) << "Material " << inputs.asset_id.d.c_str() << " : no initializer for texture " << material_texture_binding.texture_name_in_shader;
			mxDO(writeAssetID( AssetID_from_FilePath(MISSING_TEXTURE_ASSET_ID), output_writer ));
		}

		mxDO(output_writer.Put( material_texture_binding.bind_slot ));
	}
#endif

	//if( cmd_buf_.d.IsEmpty() || (cmd_buf_.size > 0) )
	//{
	//	ptWARN("Material '%s' command buffer is empty!",
	//		material_def.effect_filename.c_str()
	//		);
	//}

	return ALL_OK;
}

///
ERet MaterialCompiler::evaluateUniforms(
										void * uniform_buffer_data_start_
										, const ShaderCBuffer& uniform_buffer
										, const MaterialDef& material_def
										) const
{
	for( UINT i = 0; i < material_def.uniform_parameters.num(); i++ )
	{
		const StringNameValuePair& uniform_parameter = material_def.uniform_parameters[i];

		const Shaders::Field* uniform = uniform_buffer.findUniformByName( uniform_parameter.name.c_str() );

		_evalulated_uniforms_count = 0;

		if( uniform )
		{
			lua_State *	L = AssetProcessor::Get().GetScriptEngine()->GetLuaState();
			LUA_DBG_CHECK_STACK(L);

			// Load script
			LUA_TRY(luaL_loadbuffer(
				L,
				uniform_parameter.value.c_str(),
				uniform_parameter.value.length(),
				NULL
				)
				, L
				);

			// LUA_MULTRET = -1 means push all return parameters onto the stack
			LUA_TRY(
				lua_pcall( L, 0, LUA_MULTRET, 0 )
				, L
				);

			if( _evalulated_uniforms_count )
			{
				const int evalulated_uniforms_size = _evalulated_uniforms_count * sizeof(_evalulated_uniforms[0]);

				if( uniform->size == evalulated_uniforms_size )
				{
					memcpy(
						mxAddByteOffset( uniform_buffer_data_start_, uniform->offset )
						, _evalulated_uniforms
						, uniform->size
						);
				}
				else
				{
					ptWARN( "Incorrect size '%s'!", uniform_parameter.name.c_str() );
					return ERR_OBJECT_NOT_FOUND;
				}
			}
			else
			{
				ptWARN( "Couldn't evaluate uniform parameter expression '%s': '%s'!"
					, uniform_parameter.name.c_str(), uniform_parameter.value.c_str() );
				return ERR_OBJECT_NOT_FOUND;
			}
		}
		else
		{
			ptWARN( "Uniform '%s' not found!", uniform_parameter.name.c_str() );
			return ERR_OBJECT_NOT_FOUND;
		}
	}

	return ALL_OK;
}


ERet MaterialCompiler::compileMaterialEx(
	CompiledAssetData &outputs
	, const MaterialDef& material_def
	, const AssetID& material_asset_id
	, AssetDatabase & asset_database
	, const ShaderCompilerConfig& shader_compiler_settings
	, AllocatorI& temp_alloc
	, const DebugParam& dbgparam
) const
{
	const AssetID effect_asset_id = MakeAssetID( material_def.effect_filename.c_str() );

	//
	ShaderMetadata			shader_metadata;
	NwShaderEffectData		shader_effect_data;

	mxDO(CompileMaterialEffectIfNeeded(
		material_def.effect_filename.c_str()
		, effect_asset_id
		, Arrays::GetSpan( material_def.defines )
		, shader_compiler_settings
		, asset_database

		, shader_metadata
		, shader_effect_data
		, dbgparam
		));

	//asset_database.addDependency(
	//	);


	// Select shader permutation.

	NwShaderFeatureBitMask	shader_feature_mask = 0;
	mxDO(GetDefaultShaderFeatureMask(
		shader_feature_mask
		, material_def.feature_switches
		, shader_effect_data
		));

	//
	Rendering::Material::VariableLengthData	material_data;
	{
		mxDO(BuildMaterialCommandBuffer(
			material_data.command_buffer
			,material_def
			,shader_effect_data
			,dbgparam
			));

		// allocate space for shader resources (e.g. textures)
		mxDO(setupMaterialTextureBindings(
			material_data.referenced_textures
			, shader_effect_data
			, shader_metadata
			, material_def
			));
	}


	//
	mxDO(SetupMaterialPasses(
		material_data.passes
		, shader_effect_data
		, shader_metadata
		, material_def
		, shader_feature_mask
		));
	mxENSURE(material_data.passes.num() > 0,
		ERR_UNKNOWN_ERROR,
		""
		);


	// use shader metadata to populate material parameters

	const ShaderCBuffer *	ubo = findLocalUniformBuffer( shader_metadata );

	// Parse values of uniform parameters.

	if( ubo )
	{
		const UINT num_vec4s = tbALIGN16(ubo->size) / sizeof(Vector4);
		mxDO(material_data.uniforms.setNum(num_vec4s));

		mxDO(evaluateUniforms(
			material_data.uniforms.raw()
			, *ubo
			, material_def
			));
	}


	// save compiled material

	NwBlobWriter	material_object_data_writer( outputs.object_data );
	{
		Rendering::MaterialLoader::MaterialHeader_d	header;
		mxZERO_OUT(header);
		{
			header.fourCC = Rendering::MaterialLoader::SIGNATURE;
		}
		mxDO(material_object_data_writer.Put( header ));

		//
		mxDO(writeAssetID( effect_asset_id, material_object_data_writer ));

		//
		mxDO(material_object_data_writer.alignBy( Serialization::NwImageSaveFileHeader::ALIGNMENT ));
		mxDO(Serialization::SaveMemoryImage(
			material_data
			, material_object_data_writer
			));


		////
		//mxDO(material_object_data_writer.alignBy( Rendering::MaterialLoader::PASSDATA_ALIGNMENT ));

		//for( UINT iPass = 0; iPass < material_passes.num(); iPass++ )
		//{
		//	mxDO(material_object_data_writer.Put(
		//		material_passes._data[ iPass ]
		//	));
		//}


//{
//	AllocatorI &	scratchpad = MemoryHeaps::temporary();
//
//	Blob	saved_data( scratchpad );
//
//	mxDO(Serialization::SaveMemoryImage(
//		id_material_data
//		, NwBlobWriter( saved_data )
//		));
//
//
//	Rendering::Material::LoadedData *	test;
//	mxDO(Serialization::LoadMemoryImage(
//		NwBlobReader( saved_data )
//		, test
//		, scratchpad
//		));
//}

//		LogStream(LL_Info) << "Saved material: " << header.numResources << " textures, " << header.numUniforms << " uniforms";
	}

	return ALL_OK;
}

///
ERet MaterialCompiler::compileMaterial(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	) const
{
	MaterialDef material_def;
	mxDO(material_def.parseFrom(
		inputs.reader
		, inputs.path.c_str()
		));

	mxDO(this->compileMaterialEx(
		outputs
		, material_def
		, inputs.asset_id
		, inputs.asset_database
		, inputs.cfg.shader_compiler
		, MemoryHeaps::temporary()
	));

	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////

#define CHECK_PARAM_COUNT( num_arguments )\
	mxASSERT2( num_arguments == lua_gettop( L ),\
			"incorrect number of arguments: expected %d, but got %d", num_arguments, lua_gettop( L ) );

#define CHECK_PARAM_TYPE( param_index, TYPE )\
	if( !LuaWrapper::checkParameterType<TYPE>( L, param_index ) ) return 0;


int l_make_vec2(
				lua_State* L
				)
{
	CHECK_PARAM_COUNT(2);
	CHECK_PARAM_TYPE(1, F32);
	CHECK_PARAM_TYPE(2, F32);

	F32 p1 = LuaWrapper::toType< F32 >( L, 1 );
	F32 p2 = LuaWrapper::toType< F32 >( L, 2 );

	LuaWrapper::pushLua( L, p1 );	// first result
	LuaWrapper::pushLua( L, p2 );	// second result

	//
	MaterialCompiler& mc = MaterialCompiler::Get();
	mc._evalulated_uniforms[0] = p1;
	mc._evalulated_uniforms[1] = p2;
	mc._evalulated_uniforms_count = 2;

	return 2;	// number of results
} 

static
void _bindFunctionsToLua()
{
	lua_State *	L = AssetProcessor::Get().GetScriptEngine()->GetLuaState();

	//lua_register( L, "vec2", TO_LUA_CALLBACK(lua_push_vec2) );
	lua_register( L, "vec2", l_make_vec2 );
}

MaterialCompiler::MaterialCompiler()
{
	_evalulated_uniforms_count = 0;
}

ERet MaterialCompiler::Initialize()
{
	_bindFunctionsToLua();

	return ALL_OK;
}

void MaterialCompiler::Shutdown()
{
}

ERet MaterialCompiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	mxTRY(this->compileMaterial( inputs, outputs ));
	return ALL_OK;
}

}//namespace AssetBaking

#endif // WITH_MATERIAL_COMPILER
