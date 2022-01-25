#include "stdafx.h"
#pragma hdrstop

#include <Base/Base.h>

#include <Core/Text/Lexer.h>
#include <Core/Assets/AssetBundle.h>
#include <Sound/SoundSystem.h>
#include <AssetCompiler/AssetCompilers/AudioAssetsCompiler.h>
#include <Core/Serialization/Text/TxTSerializers.h>


namespace AssetBaking
{

mxDEFINE_CLASS(AudioAssetsCompiler);

ERet AudioAssetsCompiler::Initialize()
{
	return ALL_OK;
}

void AudioAssetsCompiler::Shutdown()
{
}

const TbMetaClass* AudioAssetsCompiler::getOutputAssetClass() const
{
	return &NwAudioClip::metaClass();
}

//
mxTODO("remove composePathToAssetMetadata()");
template< class ASSET_DEF >
ERet ComposePathToAssetDef(
	String& path_to_asset_metadata,
	const char* absolute_path_to_source_file,
	const ASSET_DEF& asset_def
	)
{
	mxDO(Str::CopyS( path_to_asset_metadata, absolute_path_to_source_file ));
	Str::StripFileExtension( path_to_asset_metadata );
	mxDO(Str::setFileExtension( path_to_asset_metadata, ASSET_DEF::metaClass().m_name ));
	return ALL_OK;
}

ERet AudioAssetsCompiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	NwBlobWriter	output_writer(outputs.object_data);

	//
	NwSoundDef	sound_def;

	//
	FilePathStringT	path_to_sound_def;
	mxDO(ComposePathToAssetDef(
		path_to_sound_def
		, inputs.path.c_str()
		, sound_def
		));

	const ERet res = SON::LoadFromFile(
		path_to_sound_def.c_str()
		, sound_def
		, scratchpad
		);
	if(res == ALL_OK)
	{
		mxDO(output_writer.Put( (U32)NwSoundDef::FOUR_CC ));
		mxDO(output_writer.Put( sound_def.flags ));
	}

	//
	mxDO(NwBlob_::loadBlobFromStream( inputs.reader, outputs.object_data ));

	return ALL_OK;
}

}//namespace AssetBaking
