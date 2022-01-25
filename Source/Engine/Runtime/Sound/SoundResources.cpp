#include <bx/debug.h>
#include <Base/Base.h>
#pragma hdrstop
#include <Core/Memory.h>	// ProxyAllocator

#include <Sound/SoundResources.h>
#include <Sound/SoundSystem.h>
#include <Sound/FMOD/SoundSystem_FMOD.h>



mxBEGIN_FLAGS( SoundFlagsT )
mxREFLECT_BIT( Is2D, NwSoundFlags::Is2D ),
mxREFLECT_BIT( Looping, NwSoundFlags::Looping ),
mxEND_FLAGS

/*
=======================================================================
	NwSoundDef
=======================================================================
*/
mxDEFINE_CLASS(NwSoundDef);
mxBEGIN_REFLECTION(NwSoundDef)
	mxMEMBER_FIELD(flags),
mxEND_REFLECTION

NwSoundDef::NwSoundDef()
{
	flags = NwSoundFlags::Defaults;
}

/*
=======================================================================
	NwAudioClip
=======================================================================
*/
mxDEFINE_CLASS(NwAudioClip);
mxBEGIN_REFLECTION(NwAudioClip)
mxEND_REFLECTION

NwAudioClip::NwAudioClip()
{
}

/*
=======================================================================
	NwAudioFileLoader
=======================================================================
*/
NwAudioFileLoader::NwAudioFileLoader(
							 ProxyAllocator & parent_allocator
							 )
	: TbAssetLoader_Null( NwAudioClip::metaClass(), parent_allocator )
{
}

ERet NwAudioFileLoader::create( NwResource **new_instance_, const TbAssetLoadContext& context )
{
	*new_instance_ = context.object_allocator.new_<NwAudioClip>();
	return ALL_OK;
}

ERet NwAudioFileLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	NwAudioClip *sound = static_cast< NwAudioClip* >( instance );

//	sound->dbg_id = context.key.id;
//if(strstr(context.key.id.c_str(), "loop_flames_04")) {
//	mxASSERT(0);
//}

	//
	const UINT mandatory_flags = FMOD_OPENMEMORY|FMOD_HARDWARE|FMOD_LOWMEM;

	unsigned create_sound_flags = mandatory_flags;

	//
	AssetReader	stream;
	mxDO(Resources::OpenFile( context.key, &stream ));

	//
	NwBlob	sound_data( context.scratch_allocator );
	mxDO(NwBlob_::loadBlobFromStream( stream, sound_data ));

	//
	char* stream_data = sound_data.raw();
	U32	stream_len = stream.Length();

	//
	const U32* header = (U32*) sound_data.raw();
	if( *header == NwSoundDef::FOUR_CC )
	{
		const SoundFlagsT flags = *(SoundFlagsT*) (header + 1);

		if( flags & NwSoundFlags::Is2D ) {
			create_sound_flags |= FMOD_2D;
		}
		if( flags & NwSoundFlags::Looping ) {
			create_sound_flags |= FMOD_LOOP_NORMAL;
		}

		stream_data += 8;
		stream_len -= 8;
	}

	//
	create_sound_flags |= context.load_flags | mandatory_flags;

	//
	FMOD_CREATESOUNDEXINFO	exinfo;
	mxZERO_OUT(exinfo);
	exinfo.cbsize = sizeof(exinfo);
	exinfo.length = stream_len;

	//
	NwSoundSystem_FMOD& sound_system_fmod = static_cast<NwSoundSystem_FMOD&>(NwSoundSystemI::Get());
	nwFMOD_CALL(sound_system_fmod.fmod_system->createSound(
		stream_data
		, create_sound_flags
		, &exinfo
		, &sound->fmod_sound._ptr
		));

	//nwFMOD_CALL(sound->fmod_sound->set3DMinMaxDistance(
	//	10.0f,
	//	10000.0f
	//));

	return ALL_OK;
}

void NwAudioFileLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
{
	//FMOD_Sound_Release(sound);
	UNDONE;
}

ERet NwAudioFileLoader::reload( NwResource * instance, const TbAssetLoadContext& context )
{
	mxDO(Super::reload( instance, context ));
	return ALL_OK;
}

/*
=======================================================================
	NwSoundSource
=======================================================================
*/
mxDEFINE_CLASS(NwSoundSource);
mxBEGIN_REFLECTION(NwSoundSource)
	mxMEMBER_FIELD(min_distance),
	mxMEMBER_FIELD(max_distance),
	mxMEMBER_FIELD(volume),
	mxMEMBER_FIELD(sounds),
mxEND_REFLECTION

NwSoundSource::NwSoundSource()
{
	sounds.initializeWithExternalStorageAndCount(
		sounds_storage.asTypedArray(),
		sounds_storage.capacity()
		);
}

ERet NwSoundSource::ensureLoaded(LoadFlagsT fmod_sound_flags)
{
	mxASSERT(sounds.num());
	for( UINT i = 0; i < sounds.num(); i++ )
	{
		mxDO(sounds[i].Load(
			nil,
			fmod_sound_flags
			));
	}
	return ALL_OK;
}

ERet NwSoundSource::getRandomSound(
								   NwAudioClip **sound_
								   , NwRandom & rng
								   ) const
{
	const int sound_index = rng.RandomInt( sounds.num() );
	NwAudioClip* random_sound = sounds[ sound_index ]._ptr;
	*sound_ = random_sound;
	return random_sound ? ALL_OK : ERR_FAILED_TO_LOAD_DATA;
}

/*
=======================================================================
	NwSoundShaderLoader
=======================================================================
*/
NwSoundShaderLoader::NwSoundShaderLoader( ProxyAllocator & parent_allocator )
	: TbAssetLoader_Null( NwSoundSource::metaClass(), parent_allocator )
{
}

ERet NwSoundShaderLoader::create( NwResource **new_instance_, const TbAssetLoadContext& context )
{
	*new_instance_ = context.object_allocator.new_< NwSoundSource >();
	return ALL_OK;
}

ERet NwSoundShaderLoader::load( NwResource * instance, const TbAssetLoadContext& context )
{
	NwSoundSource *sound_shader = static_cast< NwSoundSource* >( instance );

	//
	AssetReader	stream;
	mxDO(Resources::OpenFile( context.key, &stream ));

	//
	NwSoundSourceData	sound_source_data;
	mxDO(stream.Get( sound_source_data ));

	TCopyBase(*sound_shader, sound_source_data);

	//
	LoadFlagsT	fmod_sound_flags = 0;

	if(sound_source_data.flags & NwSoundFlags::Looping)
	{
		fmod_sound_flags |= FMOD_LOOP_NORMAL;
	}

	if(sound_source_data.flags & NwSoundFlags::Is2D)
	{
		fmod_sound_flags |= FMOD_2D;
	}
	else
	{
		fmod_sound_flags |= FMOD_3D;
	}

	//
	U32	num_sounds;
	mxDO(stream.Get( num_sounds ));

	mxDO(sound_shader->sounds.setNum( num_sounds ));
	for( UINT i = 0; i < num_sounds; i++ )
	{
		mxDO(readAssetID(
			sound_shader->sounds[i]._id,
			stream
			));
	}

	//
	sound_shader->ensureLoaded(fmod_sound_flags);

	return ALL_OK;
}

void NwSoundShaderLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
{
	UNDONE;
}

ERet NwSoundShaderLoader::reload( NwResource * instance, const TbAssetLoadContext& context )
{
	mxDO(Super::reload( instance, context ));
	return ALL_OK;
}
