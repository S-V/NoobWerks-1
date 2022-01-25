//
#include "stdafx.h"
#pragma hdrstop

#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Base/Template/Algorithm/Sorting/InsertionSort.h>	// InsertionSortKeysAndValues
#include <Base/Template/Templates.h>	// TCopyBase

#include <AssetCompiler/AssetCompilers/Graphics/RenderPathCompiler.h>

#if WITH_RENDERING_PIPELINE_COMPILER

#include <Base/Util/PathUtils.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Serialization/Serialization.h>
#include <Graphics/Public/graphics_formats.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Rendering/Public/Core/Material.h>
#include <Rendering/Public/Globals.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include <Developer/RendererCompiler/Effect_Compiler.h>
#include <Developer/RendererCompiler/Target_Common.h>
#include <Developer/UniqueNameHashChecker.h>

namespace AssetBaking
{

mxDEFINE_CLASS(RenderPipelineCompiler);

static ERet CreateRenderTargets(
								const FxResourceDescriptions& decl,
								NwClump &clump
								)
{
	const UINT numColorTargets = decl.color_targets.num();
	if( numColorTargets > 0 )
	{
		NwUniqueNameHashChecker	unique_render_target_name_hashes;

		for( UINT iColorTarget = 0; iColorTarget < numColorTargets; iColorTarget++ )
		{
			const FxColorTargetDescription& colorTargetDesc = decl.color_targets[ iColorTarget ];
			TbColorTarget* colorTarget;
			mxDO(clump.New( colorTarget ));

			*colorTarget = colorTargetDesc;

			colorTarget->name = colorTargetDesc.name;
			colorTarget->UpdateNameHash();
			mxTRY(unique_render_target_name_hashes.InsertUniqueName(
				colorTarget->name.c_str(),
				colorTarget->hash
				));

//if( colorTarget->view_flags & GrTextureCreationFlags::GENERATE_MIPS ) {
//	DBGOUT("GenMips: %s", colorTarget->name.c_str());
//}
//if( colorTarget->view_flags & GrTextureCreationFlags::RANDOM_WRITES ) {
//	DBGOUT("UAV: %s", colorTarget->name.c_str());
//}
		}
	}

	const UINT numDepthTargets = decl.depth_targets.num();
	if( numDepthTargets > 0 )
	{
		NwUniqueNameHashChecker	unique_depth_target_name_hashes;

		for( UINT iDepthTarget = 0; iDepthTarget < numDepthTargets; iDepthTarget++ )
		{
			const FxDepthTargetDescription& depthTargetDesc = decl.depth_targets[ iDepthTarget ];
			TbDepthTarget* depth_target;
			mxDO(clump.New< TbDepthTarget >(depth_target));

			*depth_target = depthTargetDesc;

			depth_target->name = depthTargetDesc.name;
			depth_target->UpdateNameHash();
			mxTRY(unique_depth_target_name_hashes.InsertUniqueName( depth_target->name.c_str(), depth_target->hash ));
		}
	}

	const UINT numTextures = decl.textures.num();
	if( numTextures > 0 )
	{
		NwUniqueNameHashChecker	unique_texture_name_hashes;

		for( UINT iTexture = 0; iTexture < numTextures; iTexture++ )
		{
			const TbTexture2D& source = decl.textures[ iTexture ];
			TbTexture2D* destination;
			mxDO(clump.New< TbTexture2D >(destination));

			*destination = source;
			destination->UpdateNameHash();

			mxTRY(unique_texture_name_hashes.InsertUniqueName( destination->name.c_str(), destination->hash ));
		}
	}

	return ALL_OK;
}

static
ERet CreateRenderStates(
						const FxResourceDescriptions& descriptions,
						NwClump &clump
						)
{
	// render states

	const int numDepthStencilStates = descriptions.depth_stencil_states.num();
	if( numDepthStencilStates > 0 )
	{
		// we store relative indices within the handles
		const UINT maxAllowedDepthStencilStates = (1UL << BYTES_TO_BITS(sizeof(HDepthStencilState)));
		mxENSURE( numDepthStencilStates < maxAllowedDepthStencilStates, ERR_INVALID_PARAMETER,
			"Too many depth-stencil states '%d' (max. allowed: %d)", numDepthStencilStates, maxAllowedDepthStencilStates );

		NwUniqueNameHashChecker	unique_depth_stencil_name_hashes;

		for( int iDepthStencilState = 0; iDepthStencilState < numDepthStencilStates; iDepthStencilState++ )
		{
			const NwDepthStencilDescription& description = descriptions.depth_stencil_states[ iDepthStencilState ];

			FxDepthStencilState* depthStencilState;
			mxDO(clump.New< FxDepthStencilState >(depthStencilState));

			TCopyBase( *depthStencilState, description );

			//mxTRY(unique_depth_stencil_name_hashes.InsertUniqueName( depthStencilState->name.c_str(), depthStencilState->hash ));
		}
	}

	const int numRasterizerStates = descriptions.rasterizer_states.num();
	if( numRasterizerStates > 0 )
	{
		mxASSERT(numRasterizerStates < 256);

		NwUniqueNameHashChecker	unique_rasterizer_name_hashes;

		for( int iRasterizerState = 0; iRasterizerState < numRasterizerStates; iRasterizerState++ )
		{
			const NwRasterizerDescription& description = descriptions.rasterizer_states[ iRasterizerState ];
			FxRasterizerState* rasterizerState;
			mxDO(clump.New< FxRasterizerState >(rasterizerState));

			TCopyBase( *rasterizerState, description );

			//mxTRY(unique_rasterizer_name_hashes.InsertUniqueName( rasterizerState->name.c_str(), rasterizerState->hash ));
		}
	}
#if 0
	const int numSamplerStates = descriptions.sampler_states.num();
	if( numSamplerStates > 0 )
	{
		std::map< UINT, FxSamplerState* > samplerStatesMap;

		mxDO(resources->sampler_states.ReserveMore( numSamplerStates ));
		
		for( int iSamplerState = 0; iSamplerState < numSamplerStates; iSamplerState++ )
		{
			const SamplerDescription& description = descriptions.sampler_states[ iSamplerState ];
			FxSamplerState* samplerState;
			clump.New< FxSamplerState >(samplerState);
			TCopyBase( *samplerState, description );
			samplerState->name = description.name;
			samplerState->UpdateNameHash();

			mxDO(EnsureUniqueHash(samplerState, samplerStatesMap));

			resources->sampler_states.add(samplerState);
		}
	}
#endif

	const UINT numBlendStates = descriptions.blend_states.num();
	if( numBlendStates > 0 )
	{
		// we store relative indices within the handles
		const UINT maxAllowedBlendStates = (1UL << BYTES_TO_BITS(sizeof(HBlendState)));
		mxENSURE( numBlendStates < maxAllowedBlendStates, ERR_INVALID_PARAMETER,
			"Too many blend states '%d' (max. allowed: %d)", numBlendStates, maxAllowedBlendStates );

		NwUniqueNameHashChecker	unique_blend_state_name_hashes;

		for( int iBlendState = 0; iBlendState < descriptions.blend_states.num(); iBlendState++ )
		{
			const NwBlendDescription& description = descriptions.blend_states[ iBlendState ];

			FxBlendState* blendState;
			mxDO(clump.New< FxBlendState >(blendState));

			TCopyBase( *blendState, description );

			//mxTRY(unique_blend_state_name_hashes.InsertUniqueName( blendState->name.c_str(), blendState->hash ));
		}
	}

#if 0
	const UINT numStateBlocks = descriptions.state_blocks.num();
	if( numStateBlocks > 0 )
	{
		NwUniqueNameHashChecker	unique_state_block_name_hashes;

		for( int iStateBlock = 0; iStateBlock < numStateBlocks; iStateBlock++ )
		{
			const FxStateBlockDescription& description = descriptions.state_blocks[ iStateBlock ];

			NwRenderState* stateBlock;
			mxDO(clump.New< NwRenderState >(stateBlock));

			const NameHash32 nameHash = GetDynamicStringHash( description.name.c_str() );
			mxTRY(unique_state_block_name_hashes.InsertUniqueName( description.name.c_str(), nameHash ));

			stateBlock->name = description.name;
			stateBlock->hash = nameHash;

			const int blendStateIndex = FindIndexByName(descriptions.blend_states, description.blendState);
			if( blendStateIndex == -1 ) {
				ptERROR("State block '%s' couldn't find blend state '%s'\n", description.name.c_str(), description.blendState.raw());
				return ERR_OBJECT_NOT_FOUND;
			}
			stateBlock->blend_state.id = blendStateIndex;

			const int rasterizerStateIndex = FindIndexByName(descriptions.rasterizer_states, description.rasterizerState);
			if( rasterizerStateIndex == -1 ) {
				ptERROR("State block '%s' couldn't find rasterizer state '%s'\n", description.name.c_str(), description.rasterizerState.raw());
				return ERR_OBJECT_NOT_FOUND;
			}
			stateBlock->rasterizer_state.id = rasterizerStateIndex;

			const int depthStencilStateIndex = FindIndexByName(descriptions.depth_stencil_states, description.depthStencilState);
			if( depthStencilStateIndex == -1 ) {
				ptERROR("State block '%s' couldn't find depth-stencil state '%s'\n", description.name.c_str(), description.depthStencilState.raw());
				return ERR_OBJECT_NOT_FOUND;
			}
			stateBlock->depth_stencil_state.id = depthStencilStateIndex;
			stateBlock->stencil_reference_value = description.stencilRef;

			resources->state_blocks.add(stateBlock);
		}
	}
#endif

	return ALL_OK;
}

const TbMetaClass* RenderPipelineCompiler::getOutputAssetClass() const
{
	return &NwClump::metaClass();
}

ERet RenderPipelineCompiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &_outputs
	)
{
	RenderPipelineDef	decl;
	mxDO(SON::Load(
		inputs.reader,
		decl,
		MemoryHeaps::temporary(),
		inputs.path.c_str()
		));

	ptPRINT("Compiling rendering pipeline: %s", inputs.path.c_str());

	const UINT numRenderPasses = decl.layers.num();

	mxENSURE(numRenderPasses < Rendering::tbMAX_PASSES_IN_RENDER_PIPELINE,
		ERR_NOT_IMPLEMENTED,
		"The number of passes must fit into UInt8!"
		);

	{
		NwClump	outputClump;


		// add references to external resources.
		for( UINT i = 0; i < decl.includes.num(); i++ )
		{
			AssetImport* clumpImport;
			mxDO(outputClump.New( clumpImport ));

			clumpImport->id = AssetID_from_FilePath( decl.includes[i].c_str() );
			clumpImport->type = NwClump::metaClass().GetTypeGUID();
		}



		// create render resources
		{
			mxDO(CreateRenderTargets( decl, outputClump ));
			mxDO(CreateRenderStates( decl, outputClump ));
		}


		{
			Rendering::RenderPath *	renderPath;
			mxDO(outputClump.New( renderPath ));

			// maps from sorted name hashes to original pass order
			DynamicArray< U32 >	permutedIndices( MemoryHeaps::temporary() );

			mxDO(renderPath->sortedPassNameHashes.setNum( numRenderPasses ));
			mxDO(renderPath->passInfo.setNum( numRenderPasses ));
			mxDO(renderPath->passData.setNum( numRenderPasses ));
			mxDO(permutedIndices.setNum( numRenderPasses ));

			//
			for( UINT iPass = 0; iPass < numRenderPasses; iPass++ )
			{
				const FxSceneLayerDef& layerDef = decl.layers[ iPass ];

				const NameHash32 nameHash = GetDynamicStringHash( layerDef.id.c_str() );
				renderPath->sortedPassNameHashes[ iPass ] = nameHash;
				permutedIndices[ iPass ] = iPass;
			}

			//
			if( numRenderPasses )
			{
				Sorting::InsertionSortKeysAndValues(
					renderPath->sortedPassNameHashes.raw()
					, numRenderPasses
					, permutedIndices.raw()
					, Sorting::ByIncreasingOrder<typename NameHash32>()
					);
			}

			for( UINT iSortedIndex = 0; iSortedIndex < numRenderPasses; iSortedIndex++ )
			{
				UINT originalIndex = permutedIndices[ iSortedIndex ];

				Rendering::ScenePassData &scenePassData = renderPath->passData[ originalIndex ];
				scenePassData.sortedNameHashIndex = iSortedIndex;
			}

			//
			for( UINT iPass = 0; iPass < numRenderPasses; iPass++ )
			{
				const FxSceneLayerDef& layerDef = decl.layers[ iPass ];

				//
				Rendering::ScenePassData & dst = renderPath->passData[ iPass ];

				//
				Rendering::ScenePassInfo & dstPassInfo = renderPath->passInfo[dst.sortedNameHashIndex];
				dstPassInfo.draw_order = iPass;
				dstPassInfo.num_sublayers = 1;
				dstPassInfo.filter_index = iPass;
				dstPassInfo.passDataIndex = iPass;



				//
				dst.view_flags = 0;

				const UINT numRenderTargets = layerDef.render_targets.num();
				mxDO(dst.render_targets.setNum(numRenderTargets));

				ptPRINT("[%d]Pass '%s': id=0x%p, %d RTs, DS='%s'"
					, iPass, layerDef.id.c_str(), renderPath->sortedPassNameHashes[dst.sortedNameHashIndex], numRenderTargets, layerDef.depth_stencil.name.c_str()
				);

				for( UINT iRT = 0; iRT < numRenderTargets; iRT++ )
				{
					const FxRenderTargetBinding& srcRT = layerDef.render_targets[ iRT ];

					if( Str::Equal( srcRT.name, DEFAULT_RENDER_TARGET ) )
					{
						if( numRenderTargets > 1 ) {
							ptWARN("Invalid render targets");mxASSERT(0);
							return ERR_INVALID_PARAMETER;
						}
						dst.render_targets[ iRT ] = 0;
					}
					else
					{
						dst.render_targets[ iRT ] = GetDynamicStringHash( srcRT.name.c_str() );
					}

					if( srcRT.clear ) {
						dst.view_flags |= (1UL << iRT);
					}
				}

				//
				if( layerDef.depth_stencil.is_cleared ) {
					dst.view_flags |= NwViewStateFlags::ClearDepth;
					dst.view_flags |= NwViewStateFlags::ClearStencil;	//TODO: expose separate clear stencil to user?
				}

				if( layerDef.depth_stencil.name.IsEmpty() )
				{
					dst.depth_stencil_name_hash = 0;
					if( TEST_BIT( dst.view_flags, NwViewStateFlags::ClearDepth ) ) {
						ptWARN("ClearDepth specified, but no depth-stencil set");mxASSERT(0);
						return ERR_INVALID_PARAMETER;
					}
				}
				else
				{
					dst.depth_stencil_name_hash = GetDynamicStringHash( layerDef.depth_stencil.name.c_str() );

					if( layerDef.depth_stencil.is_read_only ) {
						if( layerDef.depth_stencil.is_cleared ) {
							ptWARN("Cannot specify read-only depth-stencil AND the Clear flag!");
							return ERR_INVALID_PARAMETER;
						}

						dst.view_flags |= NwViewStateFlags::ReadOnlyDepthStencil;
					}

					dst.depth_clear_value = layerDef.depth_stencil.depth_clear_value;
				}

				//
				if( inputs.cfg.renderpath_compiler.strip_debug_info ) {
					//
				} else {
					if( layerDef.profiling_scope.IsEmpty() ) {
						Str::Copy( dst.profiling_scope, layerDef.id );	// by default
					} else {
						Str::Copy( dst.profiling_scope, layerDef.profiling_scope );
					}
				}
			}
		}

//UNDONE;
#if 0
		mxDO(CreateResourceObjects( rendererDef, clump ));

		//TbRenderResourcesLUT* resources = FindSingleInstance< TbRenderResourcesLUT >( clump );

		// compile shaders

		ShaderCompiler* shaderCompiler =
			checked_cast< ShaderCompiler* >( inputs.ac->FindCompiler<ShaderCompiler>() );
		mxASSERT_PTR(shaderCompiler);

		for( UINT iShader = 0; iShader < rendererDef.shaders.num(); iShader++ )
		{
			//const FxShaderDescription& shaderDef = rendererDef.shader_programs[ iShader ];
			const char* shaderName = rendererDef.shaders[ iShader ].c_str();

			String128 shaderFileName;
			Str::CopyS(shaderFileName, shaderName);
			UNDONE;
			//Str::SetFileExtension(shaderFileName, Resources::gs_metaTypes[AssetTypes::SHADER].extension);

			String256 shaderFilePath;
			mxDO(Win32_GetAbsoluteFilePath( inputs.cfg.source_path.c_str(), shaderFileName.c_str(), shaderFilePath ));

			TbShader* shader;
			mxDO(clump.New( shader ));

			//mxDO(shaderCompiler->CompileShaderFromFile( shaderFile.c_str(), *shader, shaderCache ));

			//library->shaders.add(shader);


			AssetExport* shaderEntry;
			mxDO(clump.New( shaderEntry ));

			shaderEntry->id = AssetID_From_FileName( shaderName );
			UNDONE;
			//		shaderEntry->type = AssetTypes::SHADER;

			shaderEntry->o = shader;
		}
#endif

		mxDO(Serialization::SaveClumpImage(outputClump, NwBlobWriter(_outputs.object_data)));
		//mxDO(NwBlobWriter(_outputs.stream_data).Put(UnusedStreamingData()));

		//DEBUGGING
		if( inputs.cfg.dump_clumps_to_text )
		{
			String256	dump_file_path;
			Str::ComposeFilePath( dump_file_path,
				inputs.cfg.output_path.c_str(), inputs.path.c_str(), "son" );
			SON::SaveClumpToFile( outputClump, dump_file_path.c_str() );
		}
	}

	return ALL_OK;
}

mxDEFINE_CLASS(FxRenderTargetBinding);
mxBEGIN_REFLECTION(FxRenderTargetBinding)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(clear),	
mxEND_REFLECTION;
FxRenderTargetBinding::FxRenderTargetBinding()
{
	name.setReference(DEFAULT_RENDER_TARGET);
	clear = false;
}

mxDEFINE_CLASS(FxDepthStencilBinding);
mxBEGIN_REFLECTION(FxDepthStencilBinding)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(depth_clear_value),
	mxMEMBER_FIELD(is_cleared),
	mxMEMBER_FIELD(is_read_only),
mxEND_REFLECTION;
FxDepthStencilBinding::FxDepthStencilBinding()
{
	depth_clear_value = 1.0f;
	is_cleared = false;
	is_read_only = false;
}

mxDEFINE_CLASS(FxSceneLayerDef);
mxBEGIN_REFLECTION(FxSceneLayerDef)
	mxMEMBER_FIELD(id),
	mxMEMBER_FIELD(render_targets),
	mxMEMBER_FIELD(depth_stencil),
	mxMEMBER_FIELD(sort),
	mxMEMBER_FIELD(profiling_scope),	
mxEND_REFLECTION;
FxSceneLayerDef::FxSceneLayerDef()
{
}

mxDEFINE_CLASS(RenderPipelineDef);
mxBEGIN_REFLECTION(RenderPipelineDef)
	mxMEMBER_FIELD(includes),
	mxMEMBER_FIELD(layers),
	mxMEMBER_FIELD(shaders),
mxEND_REFLECTION;
RenderPipelineDef::RenderPipelineDef()
{
}

}//namespace AssetBaking

#endif // WITH_RENDERING_PIPELINE_COMPILER
