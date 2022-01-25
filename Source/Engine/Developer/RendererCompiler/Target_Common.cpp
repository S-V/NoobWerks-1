#include <Base/Base.h>
#pragma hdrstop

//not present in Visual C++ 2008
//#include <unordered_map>
#include <map>

// TCopyBase()
#include <Base/Template/Templates.h>
#include <Core/Core.h>
#include <Core/ObjectModel/Clump.h>
#include <ShaderCompiler/ShaderCompiler.h>
#include "Target_Common.h"

mxDEFINE_CLASS(FxStateBlockDescription);
mxBEGIN_REFLECTION(FxStateBlockDescription)
	mxMEMBER_FIELD(blendFactorRGBA),
	mxMEMBER_FIELD(sampleMask),
	mxMEMBER_FIELD(blendState),
	mxMEMBER_FIELD(depthStencilState),
	mxMEMBER_FIELD(stencilRef),
	mxMEMBER_FIELD(rasterizerState),
mxEND_REFLECTION;
FxStateBlockDescription::FxStateBlockDescription()
{
	stencilRef = 0;
	blendFactorRGBA = RGBAf::BLACK;
	sampleMask = ~0;
}

mxDEFINE_CLASS(FxColorTargetDescription);
mxBEGIN_REFLECTION(FxColorTargetDescription)
mxEND_REFLECTION;
FxColorTargetDescription::FxColorTargetDescription()
{
}

mxDEFINE_CLASS(FxDepthTargetDescription);
mxBEGIN_REFLECTION(FxDepthTargetDescription)
mxEND_REFLECTION;
FxDepthTargetDescription::FxDepthTargetDescription()
{
}

mxDEFINE_CLASS(FxStaticSwitch);
mxBEGIN_REFLECTION(FxStaticSwitch)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(tooltip),
	mxMEMBER_FIELD(UIName),
	mxMEMBER_FIELD(UIWidget),
	mxMEMBER_FIELD(defaultValue),
mxEND_REFLECTION;
FxStaticSwitch::FxStaticSwitch()
{
	UIWidget.setReference("CheckBox");
}

mxDEFINE_CLASS(FxShaderFeature);
mxBEGIN_REFLECTION(FxShaderFeature)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(tooltip),
	//mxMEMBER_FIELD(UIName),
	//mxMEMBER_FIELD(UIWidget),
	mxMEMBER_FIELD(defaultValue),
mxEND_REFLECTION;
FxShaderFeature::FxShaderFeature()
{
	defaultValue = 0;
}


mxDEFINE_CLASS(FxShaderEntry);
mxBEGIN_REFLECTION(FxShaderEntry)
	mxMEMBER_FIELD(file),
	mxMEMBER_FIELD(vertex),
	mxMEMBER_FIELD(pixel),
	mxMEMBER_FIELD(hull),
	mxMEMBER_FIELD(domain),
	mxMEMBER_FIELD(geometry),
	mxMEMBER_FIELD(compute),
mxEND_REFLECTION;
const String& FxShaderEntry::GetEntryFunction( NwShaderType::Enum _type ) const
{
	switch( _type )
	{
	case NwShaderType::Vertex :	return vertex;
	case NwShaderType::Pixel :	return pixel;
	case NwShaderType::Hull :		return hull;
	case NwShaderType::Domain :	return domain;
	case NwShaderType::Geometry :	return geometry;	
	case NwShaderType::Compute :	return compute;
	mxDEFAULT_UNREACHABLE(return String());
	}
	return String();
}

mxDEFINE_CLASS(FxTextureInitializer);
mxBEGIN_REFLECTION(FxTextureInitializer)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(texture_asset_name),
mxEND_REFLECTION;

mxDEFINE_CLASS(FxSamplerInitializer);
mxBEGIN_REFLECTION(FxSamplerInitializer)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(sampler_state_index),
	mxMEMBER_FIELD(is_built_in_sampler_state),
mxEND_REFLECTION;

mxDEFINE_CLASS(FxScenePass);
mxBEGIN_REFLECTION(FxScenePass)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(state),
	mxMEMBER_FIELD(defines),
	mxMEMBER_FIELD(features),
mxEND_REFLECTION;

mxDEFINE_CLASS(FxShaderDescription);
mxBEGIN_REFLECTION(FxShaderDescription)
	mxMEMBER_FIELD(entry),
	mxMEMBER_FIELD(scene_passes),
	mxMEMBER_FIELD(bind_samplers),
	mxMEMBER_FIELD(bind_textures),
	//mxMEMBER_FIELD(static_permutations),
	//mxMEMBER_FIELD(supportedVertexTypes),
	mxMEMBER_FIELD(not_reflected_uniform_buffers),
	//mxMEMBER_FIELD(mirrored_uniform_buffers),
mxEND_REFLECTION;
FxShaderDescription::FxShaderDescription()
{
	//// By default, create a mirror copy in RAM for the global constant buffer.
	//String globalCBufferID;
	//globalCBufferID.setReference( Shaders::DEFAULT_GLOBAL_UBO );
	//mirrored_uniform_buffers.add( globalCBufferID );
}







mxDEFINE_CLASS(FxShaderEntry2);
mxBEGIN_REFLECTION(FxShaderEntry2)
	mxMEMBER_FIELD(vertex),
	mxMEMBER_FIELD(pixel),
	mxMEMBER_FIELD(hull),
	mxMEMBER_FIELD(domain),
	mxMEMBER_FIELD(geometry),
	mxMEMBER_FIELD(compute),
mxEND_REFLECTION;
const String& FxShaderEntry2::GetEntryFunction( NwShaderType::Enum _type ) const
{
	switch( _type )
	{
	case NwShaderType::Vertex :	return vertex;
	case NwShaderType::Pixel :	return pixel;
	case NwShaderType::Hull :		return hull;
	case NwShaderType::Domain :	return domain;
	case NwShaderType::Geometry :	return geometry;	
	case NwShaderType::Compute :	return compute;
	mxDEFAULT_UNREACHABLE(return String());
	}
	return String();
}

mxDEFINE_CLASS(FxShaderPassDesc);
mxBEGIN_REFLECTION(FxShaderPassDesc)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(entry),
	mxMEMBER_FIELD(state),
	mxMEMBER_FIELD(defines),
mxEND_REFLECTION;

mxDEFINE_CLASS(FxShaderFeatureDecl);
mxBEGIN_REFLECTION(FxShaderFeatureDecl)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(tooltip),
	//mxMEMBER_FIELD(UIName),
	//mxMEMBER_FIELD(UIWidget),
	mxMEMBER_FIELD(bitWidth),
	mxMEMBER_FIELD(defaultValue),
mxEND_REFLECTION;
FxShaderFeatureDecl::FxShaderFeatureDecl()
{
	bitWidth = 1;
	defaultValue = 0;
}

mxDEFINE_CLASS(FxSamplerStateDecl);
mxBEGIN_REFLECTION(FxSamplerStateDecl)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(desc),
	mxMEMBER_FIELD(is_used),
mxEND_REFLECTION;
FxSamplerStateDecl::FxSamplerStateDecl()
{
	is_used = false;
}

mxDEFINE_CLASS(FxShaderEffectDecl);
mxBEGIN_REFLECTION(FxShaderEffectDecl)
	mxMEMBER_FIELD(passes),

	mxMEMBER_FIELD(features),

	mxMEMBER_FIELD(sampler_descriptions),

	mxMEMBER_FIELD(sampler_bindings),
	mxMEMBER_FIELD(texture_bindings),
	//mxMEMBER_FIELD(static_permutations),
	//mxMEMBER_FIELD(supportedVertexTypes),
mxEND_REFLECTION;
FxShaderEffectDecl::FxShaderEffectDecl()
{
}


mxDEFINE_CLASS(FxResourceDescriptions);
mxBEGIN_REFLECTION(FxResourceDescriptions)
	mxMEMBER_FIELD(color_targets),
	mxMEMBER_FIELD(depth_targets),
	mxMEMBER_FIELD(textures),
	mxMEMBER_FIELD(depth_stencil_states),
	mxMEMBER_FIELD(rasterizer_states),
	mxMEMBER_FIELD(sampler_states),
	mxMEMBER_FIELD(blend_states),
	mxMEMBER_FIELD(state_blocks),
mxEND_REFLECTION;
FxResourceDescriptions::FxResourceDescriptions()
{
}


mxDEFINE_CLASS(FxLibraryDescription);
mxBEGIN_REFLECTION(FxLibraryDescription)
	mxMEMBER_FIELD(includes),
	mxMEMBER_FIELD(shader_programs),
mxEND_REFLECTION;
FxLibraryDescription::FxLibraryDescription()
{
}

mxREMOVE_THIS
template< class VALUE >	// where VALUE: NamedObject
ERet EnsureUniqueHash( VALUE* _value, std::map< U32, VALUE* > & _hashMap )
{
	std::map< U32, VALUE* >::const_iterator existingIt = _hashMap.find(_value->hash);
	if( existingIt != _hashMap.end() ) {
		ptERROR("'%s' has the same hash as '%s' (0x%X)", _value->name.c_str(), existingIt->second->name.c_str(), _value->hash);
		return ERR_DUPLICATE_OBJECT;
	}
	_hashMap.insert(std::make_pair(_value->hash, _value));
	return ALL_OK;
}

ERet CreateResourceObjects(
						   const FxResourceDescriptions& descriptions,
						   NwClump &clump
						   )
{
	UNDONE;
#if 0
	TbRenderResourcesLUT* resources;
	mxDO(GoC_SingleInstance(clump, resources));

	// render targets

	const U32 numColorTargets = descriptions.color_targets.num();
	if( numColorTargets > 0 )
	{
		std::map< U32, TbColorTarget* > colorTargetsMap;

		mxDO(resources->color_targets.ReserveMore( numColorTargets ));

		for( U32 iColorTarget = 0; iColorTarget < numColorTargets; iColorTarget++ )
		{
			const FxColorTargetDescription& colorTargetDesc = descriptions.color_targets[ iColorTarget ];
			TbColorTarget* colorTarget;
			mxTRY(clump.New( colorTarget ));
			colorTarget->name = colorTargetDesc.name;
			colorTarget->UpdateNameHash();
			colorTarget->sizeX = colorTargetDesc.sizeX;
			colorTarget->sizeY = colorTargetDesc.sizeY;
			colorTarget->format = colorTargetDesc.format;

			mxDO(EnsureUniqueHash(colorTarget, colorTargetsMap));

			resources->color_targets.add( colorTarget );
		}
	}

	const U32 numDepthTargets = descriptions.depth_targets.num();
	if( numDepthTargets > 0 )
	{
		std::map< U32, TbDepthTarget* > depthTargetsMap;

		mxDO(resources->depth_targets.ReserveMore( numDepthTargets ));

		for( U32 iDepthTarget = 0; iDepthTarget < numDepthTargets; iDepthTarget++ )
		{
			const FxDepthTargetDescription& depthTargetDesc = descriptions.depth_targets[ iDepthTarget ];
			TbDepthTarget* depth_target;
			clump.New< TbDepthTarget >(depth_target);
			depth_target->name = depthTargetDesc.name;
			depth_target->UpdateNameHash();
			depth_target->sizeX = depthTargetDesc.sizeX;
			depth_target->sizeY = depthTargetDesc.sizeY;
			depth_target->format = depthTargetDesc.format;
			depth_target->sample = depthTargetDesc.sample;

			mxDO(EnsureUniqueHash(depth_target, depthTargetsMap));

			resources->depth_targets.add( depth_target );
		}
	}



	const U32 numTextures = descriptions.textures.num();
	if( numTextures > 0 )
	{
		std::map< U32, TbTexture2D* > texturesMap;

		for( U32 iTexture = 0; iTexture < numTextures; iTexture++ )
		{
			const TbTexture2D& source = descriptions.textures[ iTexture ];
			TbTexture2D* destination;
			clump.New< TbTexture2D >(destination);
			*destination = source;
			destination->UpdateNameHash();

			mxDO(EnsureUniqueHash(destination, texturesMap));
		}
	}



	// render states

	const int numDepthStencilStates = descriptions.depth_stencil_states.num();
	if( numDepthStencilStates > 0 )
	{
		std::map< U32, FxDepthStencilState* > depthStencilStatesMap;

		mxDO(resources->depth_stencil_states.ReserveMore( numDepthStencilStates ));

		for( int iDepthStencilState = 0; iDepthStencilState < numDepthStencilStates; iDepthStencilState++ )
		{
			const DepthStencilDescription& description = descriptions.depth_stencil_states[ iDepthStencilState ];
			FxDepthStencilState* depthStencilState;
			clump.New< FxDepthStencilState >(depthStencilState);
			TCopyBase( *depthStencilState, description );
			depthStencilState->name = description.name;
			depthStencilState->UpdateNameHash();

			mxDO(EnsureUniqueHash(depthStencilState, depthStencilStatesMap));

			resources->depth_stencil_states.add(depthStencilState);
		}
	}

	const int numRasterizerStates = descriptions.rasterizer_states.num();
	if( numRasterizerStates > 0 )
	{
		std::map< U32, FxRasterizerState* > rasterizerStatesMap;

		mxDO(resources->rasterizer_states.ReserveMore( numRasterizerStates ));

		for( int iRasterizerState = 0; iRasterizerState < numRasterizerStates; iRasterizerState++ )
		{
			const RasterizerDescription& description = descriptions.rasterizer_states[ iRasterizerState ];
			FxRasterizerState* rasterizerState;
			clump.New< FxRasterizerState >(rasterizerState);
			TCopyBase( *rasterizerState, description );
			rasterizerState->name = description.name;
			rasterizerState->UpdateNameHash();

			mxDO(EnsureUniqueHash(rasterizerState, rasterizerStatesMap));

			resources->rasterizer_states.add(rasterizerState);
		}
	}

	const int numSamplerStates = descriptions.sampler_states.num();
	if( numSamplerStates > 0 )
	{
		std::map< U32, FxSamplerState* > samplerStatesMap;

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

	const int numBlendStates = descriptions.blend_states.num();
	if( numBlendStates > 0 )
	{
		std::map< U32, FxBlendState* > blendStatesMap;

		mxDO(resources->blend_states.ReserveMore( numBlendStates ));
		
		for( int iBlendState = 0; iBlendState < descriptions.blend_states.num(); iBlendState++ )
		{
			const BlendDescription& description = descriptions.blend_states[ iBlendState ];
			FxBlendState* blendState;
			clump.New< FxBlendState >(blendState);
			TCopyBase( *blendState, description );
			blendState->name = description.name;
			blendState->UpdateNameHash();

			mxDO(EnsureUniqueHash(blendState, blendStatesMap));

			resources->blend_states.add(blendState);
		}
	}

	const int numStateBlocks = descriptions.state_blocks.num();
	if( numStateBlocks > 0 )
	{
		std::map< U32, NwRenderState* > stateBlocksMap;

		mxDO(resources->state_blocks.ReserveMore( numStateBlocks ));

		for( int iStateBlock = 0; iStateBlock < numStateBlocks; iStateBlock++ )
		{
			const FxStateBlockDescription& description = descriptions.state_blocks[ iStateBlock ];

			NwRenderState* stateBlock;
			mxDO(clump.New< NwRenderState >(stateBlock));

			stateBlock->name = description.name;
			stateBlock->UpdateNameHash();

			const int blendStateIndex = FindIndexByName(descriptions.blend_states, description.blendState);
			if( blendStateIndex == -1 ) {
				ptERROR("State block '%s' couldn't find blend state '%s'\n", description.name.c_str(), description.blendState.raw());
				return ERR_OBJECT_NOT_FOUND;
			}
//stateBlock->blendStateHandle.id = blendStateIndex;
			stateBlock->blendState = resources->blend_states[blendStateIndex];
			//stateBlock->blendFactor = description.blendFactorRGBA;
			//stateBlock->sampleMask = description.sampleMask;

			const int rasterizerStateIndex = FindIndexByName(descriptions.rasterizer_states, description.rasterizerState);
			if( rasterizerStateIndex == -1 ) {
				ptERROR("State block '%s' couldn't find rasterizer state '%s'\n", description.name.c_str(), description.rasterizerState.raw());
				return ERR_OBJECT_NOT_FOUND;
			}
//stateBlock->rasterizerStateHandle.id = rasterizerStateIndex;
			stateBlock->rasterizerState = resources->rasterizer_states[rasterizerStateIndex];

			const int depthStencilStateIndex = FindIndexByName(descriptions.depth_stencil_states, description.depthStencilState);
			if( depthStencilStateIndex == -1 ) {
				ptERROR("State block '%s' couldn't find depth-stencil state '%s'\n", description.name.c_str(), description.depthStencilState.raw());
				return ERR_OBJECT_NOT_FOUND;
			}
//stateBlock->depthStencilStateHandle.id = depthStencilStateIndex;
			stateBlock->depthStencilState = resources->depth_stencil_states[depthStencilStateIndex];
			stateBlock->stencilRef = description.stencilRef;

			mxDO(EnsureUniqueHash(stateBlock, stateBlocksMap));

			resources->state_blocks.add(stateBlock);
		}
	}
#endif

	return ALL_OK;
}
