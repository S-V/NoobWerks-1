#include "stdafx.h"
#pragma hdrstop

#include <Base/Base.h>

#include <Core/Text/Lexer.h>
#include <Core/Assets/AssetBundle.h>

#include <AssetCompiler/AssetCompilers/Doom3/Doom3_SoundShader_AssetCompiler.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_AssetUtil.h>
#include <AssetCompiler/AssetBundleBuilder.h>

#include <Sound/SoundSystem.h>
#include <Sound/SoundResources.h>


namespace Doom3
{
using namespace AssetBaking;

mxTODO("duplicate")
template< class STRING >
mxFORCEINLINE bool streqi( const STRING& token, const Chars& s )
{
	return 0 == stricmp( token.c_str(), s.c_str() );
}

	struct SoundShaderDef: NwSoundSourceData
	{
		String64	name;

		SoundFlagsT	flags;

		DynamicArray< String64 >	audiofiles;

	public:
		SoundShaderDef( AllocatorI & allocator )
			: audiofiles( allocator )
		{
			flags = NwSoundFlags::Defaults;
		}
	};

	/// parses *.sndshd file (based on code in "snd_shader.cpp")
	ERet parseSoundShaderDefs(
		const char* filename
		, DynamicArray< SoundShaderDef > &sound_shader_defs_
		)
	{
		sound_shader_defs_.RemoveAll();

		AllocatorI &	scratchpad = MemoryHeaps::temporary();
		DynamicArray< char >	source_file_data( scratchpad );

		mxDO(Str::loadFileToString(
			source_file_data
			, filename
			));

		//
		Lexer	lexer(
			source_file_data.raw()
			, source_file_data.rawSize()
			, filename
			);

		//
		LexerOptions	lexerOptions;
		lexerOptions.m_flags.raw_value
			= LexerOptions::SkipComments
			| LexerOptions::IgnoreUnknownCharacterErrors
			| LexerOptions::AllowPathNames
			;

		lexer.SetOptions( lexerOptions );

		//
		mxLOOP_FOREVER
		{
			TbToken	nextToken;

			SoundShaderDef	new_sound_shader_def( scratchpad );

			mxDO(expectName( lexer, new_sound_shader_def.name ));

			mxDO(expectTokenChar( lexer, '{' ));

			// the wave files

			// allow empty sound files
			mxLOOP_FOREVER
			{
				if( !lexer.PeekToken( nextToken ) ) {
					break;
				}
				if( nextToken == '}' ) {
					break;
				}

				//
				//
				if( streqi( nextToken, "description" ) ) {
					lexer.SkipRestOfLine();
					continue;
				}

				//
				if( streqi( nextToken, "minDistance" ) )
				{
					lexer.ReadToken( nextToken );
					mxDO(ExpectFloat( lexer, nextToken
						, new_sound_shader_def.min_distance
						, 0
						, 10000
						));
					continue;
				}
				//
				if( streqi( nextToken, "maxDistance" ) )
				{
					lexer.ReadToken( nextToken );
					mxDO(ExpectFloat( lexer, nextToken
						, new_sound_shader_def.max_distance
						, 0
						, 10000
						));
					continue;
				}
				//
				if( streqi( nextToken, "volume" ) )
				{
					lexer.ReadToken( nextToken );
					mxDO(ExpectFloat(
						lexer, nextToken
						, new_sound_shader_def.volume
						, -100
						, 100
						));
					continue;
				}

				//
				if( streqi( nextToken, "unclamped" ) )
				{
					lexer.ReadToken( nextToken );
					continue;
				}
				if( streqi( nextToken, "no_dups" ) )
				{
					lexer.ReadToken( nextToken );
					continue;
				}
				if( streqi( nextToken, "omnidirectional" ) )
				{
					lexer.ReadToken( nextToken );
					continue;
				}
		
				if( streqi( nextToken, "private" ) )
				{
					lexer.ReadToken( nextToken );
					continue;
				}

				if( streqi( nextToken, "looping" ) )
				{
					lexer.ReadToken( nextToken );
					new_sound_shader_def.flags |= NwSoundFlags::Looping;
					continue;
				}


				if( streqi( nextToken, "plain" ) )
				{
					// no longer supported
					lexer.ReadToken( nextToken );
					continue;
				}

				//
				if( streqi( nextToken, "shakes" ) )
				{
					lexer.ReadToken( nextToken );

					float	shake;
					mxDO(expectFloat2( &shake, lexer ));
					continue;
				}
				//
				if( streqi( nextToken, "soundClass" ) )
				{
					lexer.ReadToken( nextToken );

					int	sound_class;
					mxDO(expectInt32( lexer, nextToken, sound_class ));
					continue;
				}

				//
				if( streqi( nextToken, "force_2D_because_of_Fmod_falloff_bug" ) )
				{
					new_sound_shader_def.flags |= NwSoundFlags::Is2D;

					lexer.ReadToken( nextToken );
					continue;
				}


				//
				String64	soundfilename;
				mxDO(expectName( lexer, soundfilename ));

				mxDO(new_sound_shader_def.audiofiles.add( soundfilename ));
				continue;
			}

			mxDO(expectTokenChar( lexer, '}' ));

			//
			const UINT num_sound_entries = new_sound_shader_def.audiofiles.num();
			if( num_sound_entries > NwSoundSource::PREALLOCED_SOUNDS )
			{
				lexer.Warning( "too many (%d) sounds will cause memory allocation at runtime"
					, num_sound_entries );
			}

			mxDO(sound_shader_defs_.add( new_sound_shader_def ));

			//
			if( !lexer.PeekToken( nextToken ) ) {
				break;
			}
			if( nextToken == '}' ) {
				break;
			}
		}

		if( AssetBaking::LogLevelEnabled( LL_Debug ) )
		{
			DBGOUT("Parsed sound shader file: '%s', %d sound shaders"
				, filename, sound_shader_defs_.num()
				);
		}

		return ALL_OK;
	}

}//namespace Doom3




namespace AssetBaking
{

mxDEFINE_CLASS(Doom3_SoundShader_AssetCompiler);

ERet Doom3_SoundShader_AssetCompiler::Initialize()
{
	return ALL_OK;
}

void Doom3_SoundShader_AssetCompiler::Shutdown()
{
}

const TbMetaClass* Doom3_SoundShader_AssetCompiler::getOutputAssetClass() const
{
	// *.sndshd files are converted into asset bundles with audio and sound shader assets
	return &MetaFile::metaClass();
}

static
ERet compileAudioFileIfNeeded(
							  const String& audio_filename
							  , const AssetPipelineConfig& asset_pipeline_config
							  , AssetDatabase & asset_database
							  , AllocatorI &	scratchpad
						   )
{
	const AssetID audio_file_id = Doom3::PathToAssetID( audio_filename.c_str() );

	if( asset_database.containsAssetWithId( audio_file_id ) )
	{
		// the WAV file already exists
		LogStream(LL_Warn),"Audio file ",audio_file_id," already exists! Skipping...";
		return ALL_OK;
	}

	//
	String256	audio_file_fullpath;
	Doom3::composeFilePath(
		audio_file_fullpath
		, audio_filename.c_str()
		, asset_pipeline_config.doom3
		);

	//
	CompiledAssetData	compiled_sound( scratchpad );

	if(mxFAILED(NwBlob_::loadBlobFromFile(
		compiled_sound.object_data
		, audio_file_fullpath.c_str()
		)))
	{
		mxDO(Str::setFileExtension(
			audio_file_fullpath,
			"ogg"
			));

		const ERet ret = NwBlob_::loadBlobFromFile(
			compiled_sound.object_data
			, audio_file_fullpath.c_str()
			);
		if(mxFAILED(ret))
		{
			LogStream(LL_Warn),"Could not load Audio file '",audio_filename,"' !";
			return ret;
		}
	}


	mxDO(asset_database.addOrUpdateGeneratedAsset(
		audio_file_id
		, NwAudioClip::metaClass()
		, compiled_sound
		));

	return ALL_OK;
}

static
ERet compileSoundShaderDef(
						   const Doom3::SoundShaderDef& sound_shader_def
						   , const AssetPipelineConfig& asset_pipeline_config
						   , AssetDatabase & asset_database
						   , AllocatorI &	scratchpad
						   )
{
	const AssetID sound_shader_id = Doom3::PathToAssetID( sound_shader_def.name.c_str() );

	//
	NwSoundSource	new_sound_shader;
	{
		new_sound_shader.min_distance	= sound_shader_def.min_distance;
		new_sound_shader.max_distance	= sound_shader_def.max_distance;
		new_sound_shader.volume			= sound_shader_def.volume;		new_sound_shader.flags			= sound_shader_def.flags;
		mxDO(new_sound_shader.sounds.setNum( sound_shader_def.audiofiles.num() ));
		for( UINT i = 0; i < sound_shader_def.audiofiles.num(); i++ )
		{
			const String& audio_filename = sound_shader_def.audiofiles[i];

			const AssetID audio_file_id = Doom3::PathToAssetID( audio_filename.c_str() );

			new_sound_shader.sounds[i]._setId( audio_file_id );

			// ignore errors - only sounds that are in Doom 3 folder can be loaded
			compileAudioFileIfNeeded(
				audio_filename
				, asset_pipeline_config
				, asset_database
				, scratchpad
				);
		}
	}

	//
	CompiledAssetData	compiled_sound_shader( scratchpad );
	NwBlobWriter			blob_writer( compiled_sound_shader.object_data );

	{
		{
			const NwSoundSourceData& sound_source_data = new_sound_shader;
			mxDO(blob_writer.Put( sound_source_data ));
		}

		const U32	num_sounds = new_sound_shader.sounds.num();
		mxDO(blob_writer.Put( num_sounds ));
		for( UINT i = 0; i < num_sounds; i++ )
		{
			mxDO(writeAssetID(
				new_sound_shader.sounds[i]._id
				, blob_writer
				));
		}
	}
	mxDO(asset_database.addOrUpdateGeneratedAsset(
		sound_shader_id
		, NwSoundSource::metaClass()
		, compiled_sound_shader
		));

	return ALL_OK;
}

ERet Doom3_SoundShader_AssetCompiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	//
	DynamicArray< Doom3::SoundShaderDef >	sound_shader_defs( scratchpad );

	mxDO(Doom3::parseSoundShaderDefs(
		inputs.path.c_str()
		, sound_shader_defs
		));

	//
	for( UINT i = 0; i < sound_shader_defs.num(); i++ )
	{
		const Doom3::SoundShaderDef& sound_shader_def = sound_shader_defs[ i ];

		if(mxFAILED(compileSoundShaderDef(
			sound_shader_def
			, inputs.cfg
			, inputs.asset_database
			, scratchpad
			)))
		{
			LogStream(LL_Warn),"Couldn't compile sound shader: ",sound_shader_def.name;
		}
	}

	//
	NwBlobWriter(outputs.object_data).Put(MetaFile());

	return ALL_OK;
}

}//namespace AssetBaking
