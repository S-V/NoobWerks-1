// graphics material compiler
#pragma once

#include <AssetCompiler/AssetPipeline.h>


#if WITH_MATERIAL_COMPILER

#include <Graphics/Public/graphics_formats.h>
#include <GPU/Private/Frontend/GraphicsResourceLoaders.h>	// NwRenderStateDesc
#include <Rendering/Public/Core/Material.h>
#include <Developer/RendererCompiler/Effect_Compiler.h>
#include <Developer/RendererCompiler/Target_Common.h>


namespace AssetBaking
{


ERet CompileMaterialEffectIfNeeded(
						   const char* effect_filename	// e.g. "skinned.fx" (as written in material description file)
						   , const AssetID& effect_asset_id	// e.g. "skinned"
						   , const TSpan< const FxDefine > shader_defines
						   , const ShaderCompilerConfig& shader_compiler_settings
						   , AssetDatabase & asset_database

						   , ShaderMetadata &shader_metadata_
						   , NwShaderEffectData &shader_effect_data_
						   , const DebugParam& dbgparam
						   );

ERet GetDefaultShaderFeatureMask(
								 NwShaderFeatureBitMask &shader_feature_mask_
								 , const TSpan< const FeatureSwitch > feature_switches
								 , const NwShaderEffectData& shader_effect_data
								 );

}//namespace AssetBaking

#endif // WITH_MATERIAL_COMPILER
