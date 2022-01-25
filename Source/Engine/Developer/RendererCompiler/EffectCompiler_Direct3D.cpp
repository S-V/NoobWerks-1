/*
=============================================================================
	File:	PC_D3D11.cpp
	Desc:	
=============================================================================
*/
#include <Base/Base.h>
#pragma hdrstop
#include <Base/Template/Containers/Array/TInplaceArray.h>

#include <Core/Text/TextWriter.h>

#include <D3DX11.h>
#include <D3Dcompiler.h>

#include <Core/Text/Token.h>
#include <Core/Serialization/Serialization.h>
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_formats.h>
#include <Graphics/Private/shader_effect_impl.h>
#include <ShaderCompiler/ShaderCompiler.h>
#include <Core/Serialization/Text/TxTSerializers.h>

#include "EffectCompiler_Direct3D.h"

#if USE_D3D_SHADER_COMPILER

using namespace Shaders;

namespace
{
	const Chars CURRENT_PASS_MACRO("__PASS");
}

struct ShaderFeatureInfo
{
	String64		name;
	NameHash32		name_hash;
	NwShaderFeatureBitMask	mask;
};

struct DerivedShaderInfo
{
	DynamicArray< ShaderFeatureInfo >	features;
	U32									numPermutations;
	U32									defaultPermutationIndex;

public:
	DerivedShaderInfo( AllocatorI & allocator )
		: features( allocator )
		, numPermutations( 0 )
		, defaultPermutationIndex( 0 )
	{
	}
};

///
struct ProgramPermutation11: TbProgramPermutation
{
	ShaderMetadata	metadata;
	String64		name;	// for debugging only
};

struct IntermediateShader
{
	struct Pass
	{
		String32		name;
		NameHash32		name_hash;

		/// asset id of each program permutation in this pass
		DynamicArray< AssetID >		programPermutationIds;

	public:
		Pass( AllocatorI & allocator = MemoryHeaps::temporary() )
			: programPermutationIds( allocator )
		{}
	};

public:	// Data

	DynamicArray< Pass >	passes;

	/// merged from all shader combinations/permutations/variations
	ShaderMetadata			metadata;

public:
	IntermediateShader( AllocatorI & allocator )
		: passes( allocator )
	{
	}
};

static
ERet disassembleShaderBytecode(
							   void* compiledCodeData
							   , const size_t compiledCodeSize
							   , const String& file
							   , const NwShaderType8 shaderType
							   , const String& entry
							   , const FxOptions& options
							   , String &disassembledCode_
							   )
{
	CalendarTime	localTime( CalendarTime::GetCurrentLocalTime() );
	String64		timeOfDayString;
	GetTimeOfDayString( timeOfDayString, localTime.hour, localTime.minute, localTime.second, ':' );

	// string to place at the top of the disassembly listing
	String256		disasmComments;
	{
		StringWriter	stringStream( disasmComments );
		TextWriter		textWriter( stringStream );

		textWriter.Emitf("//=== %s shader ", NwShaderType8_Type().FindString(shaderType));
		enum { COLUMN_WIDTH = 80 };
		textWriter.Repeat('=', COLUMN_WIDTH - disasmComments.length());
		textWriter.NewLine();

		textWriter.Emitf("// File: '%s',\n", file.c_str());
		textWriter.Emitf("// Entry: '%s', Created: %s.\n", entry.c_str(), timeOfDayString.c_str());

		if( options.defines.num() )
		{
			textWriter.Emitf("// Defines:\n");
			for( U32 i = 0; i < options.defines.num(); i++ ) {
				textWriter.Emitf("// %s = %s\n",
					options.defines[i].name.c_str(), options.defines[i].value.c_str() );
			}
		}
	}

	U32 disassembleFlags = 0;

	Shaders::ByteCode disassembly = Shaders::DisassembleShaderD3D(
		compiledCodeData,
		compiledCodeSize,
		disassembleFlags,
		disasmComments.raw()
		);

	//NOTE: ignore the last nil ('\0') character.
	const size_t disassemblySize = Shaders::GetBufferSize(disassembly) - 1;

	mxDO(disassembledCode_.resize(disassemblySize));
	memcpy(disassembledCode_.raw(), Shaders::GetBufferData(disassembly), disassemblySize);

	Shaders::ReleaseBuffer( disassembly );

	return ALL_OK;
}

static
ERet compileShaderD3D(
	const char* source
	,const UINT length
	,const String& file
	,const String& entry
	,NwFileIncludeI* include
	,const NwShaderType::Enum shaderType
	,const FxOptions& options
	,AShaderDatabase * shaderDatabase

	,AssetID *shaderAssetId_
	,ShaderMetadata &shader_metadata
	//const char* disassembly_comment,
	,String &disassembledCode
	)
{
	if( entry.IsEmpty() )
	{
		// Ok, bind a null shader to that stage
		return ALL_OK;
	}

	Shaders::Options	compilerOptions;
	compilerOptions.defines = (Macro*) _alloca(options.defines.num() * sizeof(compilerOptions.defines[0]));
	compilerOptions.numDefines = options.defines.num();
	for( U32 i = 0; i < options.defines.num(); i++ ) {
		compilerOptions.defines[i].name = options.defines[i].name.c_str();
		compilerOptions.defines[i].value = options.defines[i].value.c_str();
	}

	compilerOptions.include = include;
	if( options.optimize_shaders ) {
		compilerOptions.flags |= Shaders::Compile_Optimize;
	}
	if( options.strip_debug_info ) {
		compilerOptions.flags |= Shaders::Compile_StripDebugInfo;
	}

	Shaders::ByteCode	compiledCode;

	mxTRY(Shaders::CompileShaderD3D(
		compiledCode,
		source,
		length,
		entry.raw(),
		shaderType,
		&compilerOptions,
		file.raw()
	));

	void* compiledCodeData = Shaders::GetBufferData( compiledCode );
	size_t compiledCodeSize = Shaders::GetBufferSize( compiledCode );


	// Extract reflection metadata.

	Shaders::ShaderMetadata	reflectionMetadata;
	mxTRY(Shaders::ReflectShaderD3D( compiledCodeData, compiledCodeSize, reflectionMetadata ));
	//DBGOUT("%s '%s' uses %u CBs, %u SSs, %u SRs\numBoundResources",
	//	shaderTypeString, programPermutation_.name.raw(), shader_metadata.numUsedCBs, shader_metadata.numUsedSRs, shader_metadata.numUsedSSs);

	mxTRY(MergeMetadataD3D(shader_metadata, reflectionMetadata));


	// Disassemble.

	mxTRY(disassembleShaderBytecode(
		compiledCodeData, compiledCodeSize
		, file
		, shaderType
		, entry
		, options
		, disassembledCode
		));


	// Remove reflection metadata.

	Shaders::ByteCode strippedCode = nil;
	if( options.strip_debug_info )
	{
		strippedCode = Shaders::StripShaderD3D( compiledCodeData, compiledCodeSize );
		if( strippedCode != nil )
		{
			compiledCodeData = Shaders::GetBufferData( strippedCode );
			compiledCodeSize = Shaders::GetBufferSize( strippedCode );
		}
	}



	// Serialize shader bytecode.

	ShaderHeader_d header;
	mxZERO_OUT(header);
	{
		header.type = shaderType;
		header.usedCBs = reflectionMetadata.getUsedCBufferSlotsMask();
		header.usedSSs = reflectionMetadata.getUseSamplerSlotsMask();
		header.usedSRs = reflectionMetadata.getUsedTextureSlotsMask();
	}

	NwBlob		blob( MemoryHeaps::temporary() );

	NwBlobWriter	blobWriter( blob );
	mxDO(blobWriter.Put(header));
	mxDO(blobWriter.Write(compiledCodeData, compiledCodeSize));

	const AssetID shaderAssetId = shaderDatabase->addShaderBytecode( shaderType, blob.raw(), blob.rawSize() );
	*shaderAssetId_ = shaderAssetId;

	//const U32 uniqueBytecodeIndex = shaderCache.addUniqueShaderBytecode( shaderType, blob.m_data.raw(), blob.m_data.rawSize() );
	//mxASSERT2( uniqueBytecodeIndex <= MAX_UINT16, "Shader handles are 16-bit!" );
	//shaderByteCodeIndex_ = uniqueBytecodeIndex;

	//DBGOUT("Compiled '%s' in '%s' (%u bytes at [%u])\numBoundResources", entry.raw(), file.raw(), compiledCodeSize, shaderByteCodeIndex_);

	Shaders::ReleaseBuffer( compiledCode );
	Shaders::ReleaseBuffer( strippedCode );

	return ALL_OK;
}

static
ERet compileProgramPermutation(
							   ProgramPermutation11 &programPermutation_,
							   const U32 iPermutationIndex,
							   const FxOptions& options,
							   const FxShaderPassDesc& pass_decl,
							   AShaderDatabase* shaderDatabase
							   )
{
	NwFileIncludeI* include = options.include;

	char *	fileData;
	U32	fileSize;
	if( !include->OpenFile(options.source_file.c_str(), &fileData, &fileSize) )
	{
		ptERROR("Failed to open '%s'.\n", options.source_file.raw());
		return ERR_FAILED_TO_OPEN_FILE;
	}

	String sourceFile;
	sourceFile.setReference(Chars(include->CurrentFilePath(),_InitSlow));



	DynamicArray< char >	temp1( MemoryHeaps::temporary() );
	DynamicArray< char >	temp2( MemoryHeaps::temporary() );
	mxDO(temp1.setNum(mxKiB(32)));
	mxDO(temp2.setNum(mxKiB(32)));

	String		disassembledCode;
	String		tempDisassembledCode;

	disassembledCode.initializeWithExternalStorage( temp1.raw(), temp1.rawSize() );
	tempDisassembledCode.initializeWithExternalStorage( temp2.raw(), temp2.rawSize() );

	if( options.defines.num() )
	{
		StringStream	writer( disassembledCode );
		writer << "// Run-time defines:\n";
		for( UINT iDefine = 0; iDefine < options.defines.num(); iDefine++ )
		{
			const FxDefine& define = options.defines[ iDefine ];
			writer.PrintF("//\t%s = %s\n", define.name.c_str(), define.value.c_str());
		}
	}

	for( int shaderTypeIndex = 0; shaderTypeIndex < NwShaderType::MAX; shaderTypeIndex++ )
	{
		const NwShaderType::Enum shaderType = (NwShaderType::Enum) shaderTypeIndex;

		tempDisassembledCode.empty();

		mxTRY(compileShaderD3D(
			fileData,
			fileSize,
			sourceFile,
			pass_decl.entry.GetEntryFunction( shaderType ),
			include,
			shaderType,
			options,
			shaderDatabase,
			&programPermutation_.shaderAssetIds[shaderType],
			programPermutation_.metadata,
			tempDisassembledCode
			));
		mxTRY(Str::Append( disassembledCode, tempDisassembledCode ));
	}

	// Save shader disassembly.

	if( options.disassembly_path != nil && strlen(options.disassembly_path) > 0 )
	{
		String64	fileBase( options.source_file );
		Str::StripPath(fileBase);
		Str::StripFileExtension(fileBase);

		//const FxShaderEntry2& shader

		String32	uniquePart;
		Str::Format(uniquePart, "_%s#%u", pass_decl.name.c_str(), iPermutationIndex);

		Str::Append(fileBase, uniquePart);

		String256	dumpFileName;
		Str::ComposeFilePath(dumpFileName, options.disassembly_path, fileBase.c_str(), ".h");

		Util_SaveDataToFile(disassembledCode.raw(), disassembledCode.rawSize(), dumpFileName.raw());
	}

	include->CloseFile(fileData);

	return ALL_OK;
}

static
ERet addStaticDefinesFromPass(
							  FxOptions *dstOptions_
							  , const FxShaderPassDesc& this_pass_decl
							  , const FxShaderEffectDecl& effect_decl
							  )
{
	mxDO(dstOptions_->defines.ReserveMore(
		effect_decl.passes.num() + 1 + this_pass_decl.defines.num()
		));


	//
	for( U32 iPass = 0; iPass < effect_decl.passes.num(); iPass++ )
	{
		const FxShaderPassDesc& effect_pass_decl = effect_decl.passes[ iPass ];

		FxDefine	pass_define;
		{
			Str::Copy( pass_define.name, effect_pass_decl.name );

			NameHash32 pass_name_hash = GetDynamicStringHash( effect_pass_decl.name.c_str() );
			Str::Format( pass_define.value, "%d", pass_name_hash );
		}
		mxDO(dstOptions_->defines.add( pass_define ));
	}

	//
	FxDefine	pass_define;
	{
		pass_define.name = CURRENT_PASS_MACRO;

		NameHash32 pass_name_hash = GetDynamicStringHash( this_pass_decl.name.c_str() );
		Str::Format( pass_define.value, "%d", pass_name_hash );
	}
	mxDO(dstOptions_->defines.add( pass_define ));




	//
	for( U32 iDefine = 0; iDefine < this_pass_decl.defines.num(); iDefine++ )
	{
		const String& define_string = this_pass_decl.defines[ iDefine ];
		FxDefine & newDefinition = dstOptions_->defines.AddNew();
		Str::Copy( newDefinition.name, define_string );
		newDefinition.value = Chars("1");
	}

	return ALL_OK;
}

static
ERet addDefinesForCompilingPermutation(FxOptions *dstOptions_
									   , const U32 programPermutationIndex
									   , const DerivedShaderInfo& shaderInfo)
{
	mxDO(dstOptions_->defines.ReserveMore(shaderInfo.features.num()));

	for( U32 iFeature = 0; iFeature < shaderInfo.features.num(); iFeature++ )
	{
		const ShaderFeatureInfo& shaderFeatureInfo = shaderInfo.features[ iFeature ];

		const U32 featureValue = programPermutationIndex & shaderFeatureInfo.mask;

		FxDefine & newMacro = dstOptions_->defines.AddNew();

		Str::Copy( newMacro.name, shaderFeatureInfo.name );
		Str::SetUInt( newMacro.value, featureValue );
	}

	return ALL_OK;
}

static
ERet getOptionsForCompilingProgramPermutation(
	FxOptions *dstCompileOptions_
	, const U32 programPermutationIndex
	, const FxShaderPassDesc& passDesc
	, const FxShaderEffectDecl& effect_decl
	, const DerivedShaderInfo& shaderInfo
	, const FxOptions& srcOptions
	)
{
	Str::Copy(dstCompileOptions_->source_file, srcOptions.source_file);

	dstCompileOptions_->include = srcOptions.include;

	dstCompileOptions_->optimize_shaders = srcOptions.optimize_shaders;
	dstCompileOptions_->strip_debug_info = srcOptions.strip_debug_info;

	dstCompileOptions_->disassembly_path = srcOptions.disassembly_path;

	dstCompileOptions_->defines = srcOptions.defines;

	// add static defines
	mxDO(addStaticDefinesFromPass(
		dstCompileOptions_
		, passDesc
		, effect_decl
		));

	// add run-time switches
	mxDO(addDefinesForCompilingPermutation( dstCompileOptions_, programPermutationIndex, shaderInfo ));

	return ALL_OK;
}

static
ERet CompilePass(
				 IntermediateShader::Pass &dstPass_
				 ,const FxShaderPassDesc& pass_decl
				 ,const FxShaderEffectDecl& effect_decl
				 ,IntermediateShader &technique_
				 ,const FxOptions& options
				 ,const DerivedShaderInfo& shaderInfo
				 ,AShaderDatabase* shaderDatabase
				 )
{
	dstPass_.name = pass_decl.name;

	dstPass_.name_hash = GetDynamicStringHash(pass_decl.name.c_str());

	//
	const UINT numProgramPermutations = shaderInfo.numPermutations;

	DBGOUT("'%s': Compiling %u shader permutations in pass_decl '%s' (%u defines)",
		options.source_file.c_str(), numProgramPermutations, pass_decl.name.c_str(), pass_decl.defines.num());

	mxDO(dstPass_.programPermutationIds.setNum(numProgramPermutations));

	for( U32 programPermutationIndex = 0;
		programPermutationIndex < numProgramPermutations;
		programPermutationIndex++ )
	{
		ProgramPermutation11	programPermutation;
		Str::Copy(programPermutation.name, options.source_file);

		FxOptions optionsForCompilingProgramPermutation;
		mxTRY(getOptionsForCompilingProgramPermutation(
			&optionsForCompilingProgramPermutation
			, programPermutationIndex
			, pass_decl
			, effect_decl
			, shaderInfo
			, options
			));

		mxTRY(compileProgramPermutation(
			programPermutation
			, programPermutationIndex
			, optionsForCompilingProgramPermutation
			, pass_decl
			, shaderDatabase
			));

		mxTRY(MergeMetadataD3D( technique_.metadata, programPermutation.metadata ));

		//
		dstPass_.programPermutationIds[ programPermutationIndex ] = shaderDatabase->addProgramPermutation( programPermutation );
	}

	return ALL_OK;
}

static
ERet ExtractShaderFeatures( DynamicArray< ShaderFeatureInfo > &features_
						   , U32 *totalNumProgramPermutations_
						   , U32 *defaultPermutationIndex_
						   , const FxShaderEffectDecl& description )
{
	NwUniqueNameHashChecker	unique_feature_name_hash_checker;

	U32	totalFeatureBitsUsed = 0;
	U32	defaultFeatureMask = 0;

	mxDO(features_.setNum(description.features.num()));

	for( UINT iFeature = 0; iFeature < description.features.num(); iFeature++ )
	{
		const FxShaderFeatureDecl& featureDecl = description.features[ iFeature ];

		// ensure that name hashes are unique
		const char* featureName = featureDecl.name.c_str();
		const NameHash32 featureNameHash = GetDynamicStringHash( featureName );
		mxTRY(unique_feature_name_hash_checker.InsertUniqueName( featureName, featureNameHash ));

		//
		const UINT numBitsUsedByFeature = Max<UINT>( featureDecl.bitWidth, 1 );
		const NwShaderFeatureBitMask featureBitMask = ( ( 1 << numBitsUsedByFeature ) - 1 );
		const NwShaderFeatureBitMask featureMaskWithShift = featureBitMask << totalFeatureBitsUsed;

		ShaderFeatureInfo &dstFeature = features_[ iFeature ];
		dstFeature.name = featureDecl.name;
		dstFeature.name_hash = featureNameHash;
		dstFeature.mask = featureMaskWithShift;

		DEVOUT( "Feature '%s': %d bits, %d shift (mask: 0x%X)",
			featureName, numBitsUsedByFeature, totalFeatureBitsUsed, featureMaskWithShift );

		defaultFeatureMask |= (featureDecl.defaultValue & featureBitMask) << totalFeatureBitsUsed;
		totalFeatureBitsUsed += numBitsUsedByFeature;
	}

	*totalNumProgramPermutations_ = 1 << totalFeatureBitsUsed;
	*defaultPermutationIndex_ = defaultFeatureMask;

	return ALL_OK;
}

namespace
{
struct ShaderResources
{
	TInplaceArray< const Shaders::ShaderCBuffer*, 8 >	global_cbuffers;
	TPtr<const Shaders::ShaderCBuffer>	local_cbuffer;

	//
	TInplaceArray< const Shaders::ShaderResource*, 16 >	global_resources;
	TInplaceArray< const Shaders::ShaderResource*, 16 >	local_resources;

	//
	TInplaceArray< const Shaders::ShaderSampler*, 8 >	global_samplers;
	TInplaceArray< BuiltIns::ESampler, 8 >				global_samplers_ids;

	TInplaceArray< const Shaders::ShaderSampler*, 8 >	local_samplers;
};

ERet GatherShaderResources(
						   ShaderResources &shader_resources_
						   , const Shaders::ShaderMetadata& shader_metadata
						   , const FxShaderEffectDecl& effect_decl
							  )
{
	for( U32 iCB = 0; iCB < shader_metadata.cbuffers.num(); iCB++ )
	{
		const Shaders::ShaderCBuffer& buffer = shader_metadata.cbuffers[ iCB ];

		if( buffer.IsGlobal() ) {
			mxDO(shader_resources_.global_cbuffers.add( &buffer ));
		} else {
			shader_resources_.local_cbuffer = &buffer;
		}
	}

	const UINT num_local_cbuffers
		= shader_metadata.cbuffers.num() - shader_resources_.global_cbuffers.num()
		;
	if(num_local_cbuffers > 0)
	{
		mxENSURE(num_local_cbuffers == 1
			, ERR_INVALID_PARAMETER
			, "Only one local constant buffer is supported!"
			);
	}

	//
	for( U32 iSR = 0; iSR < shader_metadata.resources.num(); iSR++ )
	{
		const Shaders::ShaderResource& resource = shader_metadata.resources[ iSR ];

		if( resource.is_global ) {
			mxDO(shader_resources_.global_resources.add( &resource ));
		} else {
			mxDO(shader_resources_.local_resources.add( &resource ));
		}
	}

	//
	for( U32 iSS = 0; iSS < shader_metadata.samplers.num(); iSS++ )
	{
		const Shaders::ShaderSampler& sampler = shader_metadata.samplers[ iSS ];

		if( sampler.is_built_in ) {
			mxDO(shader_resources_.global_samplers.add( &sampler ));
			shader_resources_.global_samplers_ids.add(sampler.builtin_sampler_id);
		} else {
			const FxSamplerInitializer* sampler_initializer = effect_decl.findSamplerInitializerByName( sampler.name.c_str() );
			if( sampler_initializer )
			{
				const BuiltIns::ESampler builtin_sampler_id = (BuiltIns::ESampler) sampler_initializer->sampler_state_index;
				mxASSERT(builtin_sampler_id < mxCOUNT_OF(BuiltIns::g_samplers));

				mxDO(shader_resources_.global_samplers.add( &sampler ));
				shader_resources_.global_samplers_ids.add(builtin_sampler_id);
			}
			else
			{
				// No binding for the sampler specified in the shader code.
				// The sampler state handle will be default-initialized.
				shader_resources_.global_samplers_ids.add(BuiltIns::DefaultSampler);
				mxDO(shader_resources_.local_samplers.add( &sampler ));
			}
		}
	}

	return ALL_OK;
}

}//namespace


static
ERet WriteShaderObjectBlob( NwBlob &shaderObjectBlob_
						   , const FxShaderEffectDecl& effect_decl
						   , const IntermediateShader& intermediate_shader
						   , const DerivedShaderInfo& shaderInfo
						   , const FxOptions& options
						   )
{
	using namespace Testbed::Graphics;

	struct ShaderEffectDataWriterUtil
	{
		static ERet WriteShaderTechniqueData(
			NwShaderEffectData &shader_effect_data_
			, const FxShaderEffectDecl& effect_decl
			, const IntermediateShader& intermediate_shader
			, const DerivedShaderInfo& shaderInfo
			, const FxOptions& options
			)
		{
			const ShaderMetadata& shader_metadata = intermediate_shader.metadata;
			//
			mxDO(EnsureItemsHaveUniqueNameHashes( shader_metadata.cbuffers ));
			mxDO(EnsureItemsHaveUniqueNameHashes( shader_metadata.resources ));
			mxDO(EnsureItemsHaveUniqueNameHashes( shader_metadata.samplers ));
			mxDO(EnsureItemsHaveUniqueNameHashes( shader_metadata.UAVs ));

			//
			ShaderResources	shader_resources;
			mxDO(GatherShaderResources(
				shader_resources
				, intermediate_shader.metadata
				, effect_decl
				));

			//
			mxASSERT(effect_decl.sampler_descriptions.num() == 0);
#if 0
			mxDO(shader_effect_data_.created_sampler_states.setNum(
				effect_decl.sampler_descriptions.num()
				));

			HSamplerState default_sampler_state_handle( 0 );
			Arrays::setAll(
				shader_effect_data_.created_sampler_states,
				default_sampler_state_handle
				);
#endif
			//
			mxDO(SetupBindingsAndBuildCommandBuffer(
				shader_effect_data_
				, effect_decl
				, intermediate_shader

				, shader_resources
				, shader_resources.local_cbuffer._ptr

				, options
				));

			mxDO(SetupLocalUniformBuffer(
				shader_effect_data_
				, intermediate_shader
				, shader_resources.local_cbuffer._ptr
				));


			//
			U32 totalNumProgramPermutations = 0;

			mxDO(SetupEffectPasses(
				shader_effect_data_
				, effect_decl
				, intermediate_shader
				, shaderInfo
				, &totalNumProgramPermutations
				));

			mxDO(SetupShaderFeaturesList(
				shader_effect_data_.features
				, shader_effect_data_.default_feature_mask
				, shaderInfo
				));

			//
			mxDO(shader_effect_data_.programs.setNum( totalNumProgramPermutations ));

			HProgram dummy_program_handle( 0 );
			Arrays::setAll(shader_effect_data_.programs, dummy_program_handle);

			return ALL_OK;
		}

	private:
		static ERet SetupBindingsAndBuildCommandBuffer(
			NwShaderEffectData &shader_effect_data_
			, const FxShaderEffectDecl& effect_decl
			, const IntermediateShader& intermediate_shader

			, const ShaderResources& shader_resources
			, const Shaders::ShaderCBuffer* local_cbuffer

			, const FxOptions& options
			)
		{
			const ShaderMetadata& shader_metadata = intermediate_shader.metadata;

			//
			NGpu::CommandBuffer	cmd_buf( MemoryHeaps::temporary() );
			mxDO(cmd_buf.reserve( LLGL_MAX_SHADER_COMMAND_BUFFER_SIZE ));

			//
			NGpu::CommandBufferWriter	cmd_buf_writer( cmd_buf );


			//
			NwShaderBindings &dst_bindings = shader_effect_data_.bindings;

			//
			{
				mxDO(dst_bindings._CBs.setNum(
					shader_resources.global_cbuffers.num()
					));
				mxDO(dst_bindings._dbg.CB_names.setNum(
					shader_resources.global_cbuffers.num()
					));
				NwShaderBindings::Binding *dst_CB_bindings = dst_bindings._CBs.raw();

				nwFOR_EACH_INDEXED( const ShaderCBuffer* global_cbuffer, shader_resources.global_cbuffers, i )
				{
					DEVOUT("Set Global CB[%d]: '%s' -> slot[%d]...",
						i, global_cbuffer->name.c_str(), global_cbuffer->slot);

					NwShaderBindings::Binding &dst_binding = dst_CB_bindings[i];

					dst_binding.name_hash = GetDynamicStringHash( global_cbuffer->name.c_str() );
					dst_binding.command_offset = cmd_buf.currentOffset();
					dst_binding.sampler_index = 0;

					mxDO(cmd_buf_writer.SetCBuffer(
						global_cbuffer->slot,
						HBuffer::MakeNilHandle()	// will be patched upon loading the effect
						));

					//
					if(!options.strip_debug_info) {
						dst_bindings._dbg.CB_names._data[i] = global_cbuffer->name;
					}
				}
				dst_bindings._num_global_CBs = shader_resources.global_cbuffers.num();
			}




			//
			{
				mxDO(dst_bindings._SRs.setNum(
					shader_resources.global_resources.num()
					+
					shader_resources.local_resources.num()
					));
				mxDO(dst_bindings._dbg.SR_names.setNum(
					dst_bindings._SRs.num()
					));
				NwShaderBindings::Binding *dst_SR_bindings = dst_bindings._SRs.raw();


				//
				nwFOR_EACH_INDEXED( const ShaderResource* global_resource,
					shader_resources.global_resources,
					i )
				{
					NwShaderBindings::Binding &dst_binding = dst_SR_bindings[i];

					dst_binding.name_hash = GetDynamicStringHash( global_resource->name.c_str() );
					dst_binding.command_offset = cmd_buf.currentOffset();
					dst_binding.sampler_index = 0;

					mxDO(cmd_buf_writer.setResource(
						global_resource->slot,
						HShaderInput::MakeNilHandle()	// will be patched upon loading the effect
						));

					//
					if(!options.strip_debug_info) {
						dst_bindings._dbg.SR_names._data[i] = global_resource->name;
					}
				}
				dst_bindings._num_global_SRs = shader_resources.global_resources.num();


				//
				nwFOR_EACH_INDEXED( const ShaderResource* local_resource,
					shader_resources.local_resources,
					i )
				{
					const UINT abs_idx = shader_resources.global_resources.num() + i;

					NwShaderBindings::Binding &dst_binding = dst_SR_bindings[ abs_idx ];

					dst_binding.name_hash = GetDynamicStringHash( local_resource->name.c_str() );
					dst_binding.command_offset = cmd_buf.currentOffset();
					dst_binding.sampler_index = 0;

					mxDO(cmd_buf_writer.setResource(
						local_resource->slot,
						HShaderInput::MakeNilHandle()	// will be patched upon loading the effect
						));

					//
					if(!options.strip_debug_info) {
						dst_bindings._dbg.SR_names._data[ abs_idx ] = local_resource->name;
					}
				}
			}




			//
			{
				mxDO(dst_bindings._SSs.setNum(
					shader_resources.global_samplers.num()
					+
					shader_resources.local_samplers.num()
					));

				mxDO(dst_bindings._dbg.SS_names.setNum(
					dst_bindings._SSs.num()
					));

				NwShaderBindings::Binding *dst_SS_bindings = dst_bindings._SSs.raw();

				//
				nwFOR_EACH_INDEXED( const ShaderSampler* global_sampler,
					shader_resources.global_samplers,
					i )
				{
					NwShaderBindings::Binding &dst_binding = dst_SS_bindings[i];

					dst_binding.name_hash = GetDynamicStringHash( global_sampler->name.c_str() );
					dst_binding.command_offset = cmd_buf.currentOffset();

					const BuiltIns::ESampler builtin_sampler_id = shader_resources.global_samplers_ids[i];
					dst_binding.sampler_index = builtin_sampler_id;

					DEVOUT("Set Global Sampler[%d]: '%s' -> slot[%d]...",
						i, global_sampler->name.c_str(), global_sampler->slot);

					mxDO(cmd_buf_writer.setSampler(
						global_sampler->slot,
						HSamplerState::MakeNilHandle()	// will be patched upon loading the effect
						));

					//
					if(!options.strip_debug_info) {
						dst_bindings._dbg.SS_names._data[ i ] = global_sampler->name;
					}
				}
				dst_bindings._num_builtin_SSs = shader_resources.global_samplers.num();


				//
				nwFOR_EACH_INDEXED( const ShaderSampler* local_sampler,
					shader_resources.local_samplers,
					i )
				{
					const UINT abs_idx = shader_resources.global_samplers.num() + i;

					NwShaderBindings::Binding &dst_binding = dst_SS_bindings[ abs_idx ];

					dst_binding.name_hash = GetDynamicStringHash( local_sampler->name.c_str() );

					// No binding for the sampler specified in the shader code.
					// The sampler state handle will be default-initialized.
					dst_binding.sampler_index = BuiltIns::DefaultSampler;

					//
					dst_binding.command_offset = cmd_buf.currentOffset();

					mxDO(cmd_buf_writer.setSampler(
						local_sampler->slot,
						HSamplerState::MakeNilHandle()	// will be patched upon loading the effect
						));

					//
					if(!options.strip_debug_info) {
						dst_bindings._dbg.SS_names._data[ abs_idx ] = local_sampler->name;
					}
				}
			}

			//
			{
				mxDO(dst_bindings._UAVs.setNum(
					shader_metadata.UAVs.num()
					));

				mxDO(dst_bindings._dbg.UAV_names.setNum(
					dst_bindings._UAVs.num()
					));

				NwShaderBindings::Binding *dst_UAV_bindings = dst_bindings._UAVs.raw();

				nwFOR_EACH_INDEXED( const ShaderUAV& uav, shader_metadata.UAVs, i )
				{
					DEVOUT("Set UAV[%d]: '%s' -> slot[%d]...", i, uav.name.c_str(), uav.slot);

					NwShaderBindings::Binding &dst_binding = dst_UAV_bindings[ i ];

					dst_binding.name_hash = GetDynamicStringHash( uav.name.c_str() );
					dst_binding.command_offset = cmd_buf.currentOffset();
					dst_binding.sampler_index = 0;

					mxDO(cmd_buf_writer.setUav(
						uav.slot,
						HShaderOutput::MakeNilHandle()
						));
					//
					if(!options.strip_debug_info) {
						dst_bindings._dbg.UAV_names._data[ i ] = uav.name;
					}
				}
			}

			//
			if(!cmd_buf.IsEmpty())
			{
				mxDO(shader_effect_data_.command_buffer.CopyFromMemory(
					cmd_buf.getStart(),
					cmd_buf.currentOffset()
					));

				shader_effect_data_.command_buffer.DbgPrint();
			}

			return ALL_OK;
		}

		static ERet SetupEffectPasses(
			NwShaderEffectData &shader_effect_data_
			, const FxShaderEffectDecl& effect_decl
			, const IntermediateShader& intermediate_shader
			, const DerivedShaderInfo& shaderInfo
			, U32 *totalNumProgramPermutationsInAllPasses
			)
		{
			mxDO(shader_effect_data_.passes.setNum( intermediate_shader.passes.num() ));

			UINT currentProgramOffset = 0;

			for( UINT iPass = 0; iPass < intermediate_shader.passes.num(); iPass++ )
			{
				const FxShaderPassDesc& pass_decl = effect_decl.passes[iPass];
				const IntermediateShader::Pass& srcPass = intermediate_shader.passes[iPass];

				NwShaderEffect::Pass &dstPass = shader_effect_data_.passes[iPass];

				dstPass.name_hash = srcPass.name_hash;

				if(pass_decl.has_custom_render_state)
				{
					dstPass.render_state.u = ~0;
				}
				else
				{
					mxASSERT( !pass_decl.state.IsEmpty() );
					dstPass.render_state.u = 0;
				}

				dstPass.default_program_index = shaderInfo.defaultPermutationIndex;
				dstPass.default_program_handle.id = ~0;
				dstPass.program_handles = nil;

				currentProgramOffset += srcPass.programPermutationIds.num();
			}

			*totalNumProgramPermutationsInAllPasses = currentProgramOffset;

			return ALL_OK;
		}

		static
		ERet SetupLocalUniformBuffer(
			NwShaderEffectData &shader_effect_data_
			, const IntermediateShader& intermediate_shader
			, const Shaders::ShaderCBuffer* local_cbuffer
			)
		{
			NwUniformBufferLayout &dstUniformBuffer = shader_effect_data_.uniform_buffer_layout;

			if( !local_cbuffer ) {
				dstUniformBuffer.size = 0;
				dstUniformBuffer.fields.clear();
				shader_effect_data_.uniform_buffer_slot = -1;
				return ALL_OK;
			}

			mxDO(EnsureItemsHaveUniqueNameHashes( local_cbuffer->fields ));

			dstUniformBuffer.name = local_cbuffer->name;

			dstUniformBuffer.size = local_cbuffer->size;
		
			const UINT numUniforms = local_cbuffer->fields.num();
			mxDO(dstUniformBuffer.fields.setNum( numUniforms ));

			for( UINT iUniform = 0; iUniform < numUniforms; iUniform++ )
			{
				const Shaders::Field& srcUniform = local_cbuffer->fields[ iUniform ];

				NwUniform &dstUniform = dstUniformBuffer.fields[ iUniform ];

				const NameHash32 name_hash = GetDynamicStringHash( srcUniform.name.c_str() );
				dstUniform.name_hash = name_hash;

				dstUniform.offset	= srcUniform.offset;
				dstUniform.size		= srcUniform.size;
			}

			//
			shader_effect_data_.uniform_buffer_slot = local_cbuffer->slot;


			// Copy default uniform parameters.

			//
			const U32 aligned_cmd_buf_size = AlignUp(
				local_cbuffer->size,
				sizeof(shader_effect_data_.local_uniforms[0])
				);
			mxASSERT(aligned_cmd_buf_size % sizeof(shader_effect_data_.local_uniforms[0]) == 0);

			mxDO(shader_effect_data_.local_uniforms.setNum(
				aligned_cmd_buf_size / sizeof(shader_effect_data_.local_uniforms[0])
				));

			// set default values

			const Shaders::ShaderCBuffer& ubo = *local_cbuffer;

			for( UINT iUniform = 0; iUniform < ubo.fields.num(); iUniform++ )
			{
				const Shaders::Field& uniform_desc = ubo.fields[ iUniform ];

				void *uniform_data = mxAddByteOffset(
					shader_effect_data_.local_uniforms.raw(),
					uniform_desc.offset
					);

				memcpy(
					uniform_data,
					uniform_desc.initial_value.raw(),
					uniform_desc.size
					);
			}

			return ALL_OK;
		}

		static ERet SetupShaderFeaturesList(
			TBuffer< NwShaderEffectData::ShaderFeature > &features_
			, NwShaderFeatureBitMask &default_feature_mask_
			, const DerivedShaderInfo& shaderInfo
			)
		{
			mxDO(features_.setNum( shaderInfo.features.num() ));

			for( UINT iFeature = 0; iFeature < shaderInfo.features.num(); iFeature++ )
			{
				const ShaderFeatureInfo& srcFeatureInfo = shaderInfo.features[iFeature];
				NwShaderEffectData::ShaderFeature &dstFeature = features_[iFeature];

				dstFeature.name_hash = srcFeatureInfo.name_hash;
				dstFeature.bit_mask = srcFeatureInfo.mask;
			}

			//
			default_feature_mask_ = shaderInfo.defaultPermutationIndex;

			return ALL_OK;
		}
	};

	//
	NwBlobWriter	blobWriter( shaderObjectBlob_ );

	NwShaderEffectData	shader_effect_data;

	mxDO(ShaderEffectDataWriterUtil::WriteShaderTechniqueData(
		shader_effect_data
		, effect_decl
		, intermediate_shader
		, shaderInfo
		, options
		));

	mxDO(Serialization::SaveMemoryImage(
		shader_effect_data,
		blobWriter
		));

#if 0
	GL::Dbg::printCommands(
		shader_effect_data.command_buffer.raw(),
		shader_effect_data.command_buffer_size
		);
#endif

	return ALL_OK;
}


static
ERet WriteShaderStreamingDataBlob( NwBlob &shaderStreamingDataBlob_
								  , const IntermediateShader& intermediate_shader
								  , const FxShaderEffectDecl& effect_declaration
								  )
{
	NwBlobWriter	blobWriter( shaderStreamingDataBlob_ );

	// Write sampler state descriptions (assume they are all used).
	{
		nwFOR_EACH( const FxSamplerStateDecl& sampler_description, effect_declaration.sampler_descriptions )
		{
			mxDO(blobWriter.Put( sampler_description.desc ));
		}
	}

	// Write texture references (assume they are all used).
	{
		nwFOR_EACH( const FxTextureInitializer& texture_initializer, effect_declaration.texture_bindings )
		{
			DBGOUT("TODO: bind texture '%s' <- '%s'",
				texture_initializer.name.c_str(),
				texture_initializer.texture_asset_name.c_str()
				);
			//mxDO(blobWriter.Put( sampler_description.desc ));
		}
	}

	//
	nwFOR_EACH( const FxShaderPassDesc& pass_def, effect_declaration.passes )
	{
		mxDO(blobWriter.alignBy(TbShaderEffectLoader::ALIGN));

		if( pass_def.has_custom_render_state )
		{
			mxDO(blobWriter.Put(pass_def.custom_render_state_desc));
		}
		else
		{
			AssetID	render_state_id;
			if( !pass_def.state.IsEmpty() )
			{
				render_state_id = MakeAssetID(pass_def.state.c_str());
			}
			else
			{
				render_state_id = MakeAssetID("default");
			}

			mxDO(writeAssetID( render_state_id, blobWriter ));
		}
	}


	//
	mxDO(blobWriter.alignBy(TbShaderEffectLoader::ALIGN));

	//
	{
		U32	maxProgramPermutationCountInPasses = 0;

		for( UINT iPass = 0; iPass < intermediate_shader.passes.num(); iPass++ )
		{
			const IntermediateShader::Pass& pass = intermediate_shader.passes[ iPass ];
			maxProgramPermutationCountInPasses = Max( maxProgramPermutationCountInPasses, pass.programPermutationIds.num() );
		}

		mxDO(blobWriter.Put( maxProgramPermutationCountInPasses ));
	}

	//
	for( UINT iPass = 0; iPass < intermediate_shader.passes.num(); iPass++ )
	{
		const IntermediateShader::Pass& pass = intermediate_shader.passes[ iPass ];
		blobWriter << pass.programPermutationIds;
	}

	return ALL_OK;
}

ERet CompileEffect_D3D11(
						 // inputs:
						 const FxOptions& options
						 ,const FxShaderEffectDecl& effect_decl

						 //[in-out]
						 ,AShaderDatabase* shaderDatabase

						 // outputs:
						 ,NwBlob *shaderObjectDataBlob_
						 ,NwBlob *shaderStreamingDataBlob_
						 ,ShaderMetadata &metadata_	// merged from all shader combinations/permutations/variations

						 //, const DebugParam& dbgparm
							)
{
	DerivedShaderInfo	shaderInfo( MemoryHeaps::temporary() );
	mxDO(ExtractShaderFeatures( shaderInfo.features, &shaderInfo.numPermutations, &shaderInfo.defaultPermutationIndex, effect_decl ));

	//
	const U32 numPasses = effect_decl.passes.num();
	mxENSURE( numPasses > 0, ERR_INVALID_PARAMETER, "No passes in shader '%s'", options.source_file.c_str() );

	//
	IntermediateShader	intermediate_shader( MemoryHeaps::temporary() );

	mxTRY(intermediate_shader.passes.setNum(numPasses));

	//
	NwUniqueNameHashChecker	unique_pass_name_hash_checker;

	for( U32 iPass = 0; iPass < numPasses; iPass++ )
	{
		const FxShaderPassDesc& srcPassDesc = effect_decl.passes[ iPass ];

		//
		const NameHash32 passNameHash = GetDynamicStringHash( srcPassDesc.name.c_str() );
		mxTRY(unique_pass_name_hash_checker.InsertUniqueName( srcPassDesc.name.c_str(), passNameHash ));

		//
		IntermediateShader::Pass &destPass = intermediate_shader.passes[ iPass ];

		//
		mxTRY(CompilePass(
			destPass
			,srcPassDesc
			,effect_decl
			,intermediate_shader
			,options
			,shaderInfo
			,shaderDatabase
		));
	}//

	//
	metadata_ = intermediate_shader.metadata;

	//
	mxDO(WriteShaderObjectBlob( *shaderObjectDataBlob_
		, effect_decl
		, intermediate_shader
		, shaderInfo
		, options
		));

	mxDO(WriteShaderStreamingDataBlob( *shaderStreamingDataBlob_
		, intermediate_shader
		, effect_decl
		));

	DEVOUT("=== Finished compiling technique %s ===", options.source_file.c_str());

	return ALL_OK;
}

#endif // USE_D3D_SHADER_COMPILER
