#include <bx/debug.h>
#include <Base/Base.h>
#pragma hdrstop
#include <Core/Memory.h>	// ProxyAllocator

#include <Sound/FMOD/SoundSystem_FMOD.h>




//#define nwSOUND_TEST_BACKGROUND_MUSIC_SOUND_NAME	"music_rmr_level4"
//#define nwSOUND_TEST_BACKGROUND_MUSIC_SOUND_NAME	"plunge"





/// Units per meter.  I.e feet would = 3.28.  centimeters would = 100.
#define DOPPLER_SCALE         1.0f
#define DISTANCE_FACTOR       1.0f

/// NOTE: this is very important!!!
/// if large values are used (or even 1.0f), the sound will fall off too quickly
/// when the listener is moving!!!
//#define ROLLOFF_SCALE         100//0.01
#define ROLLOFF_SCALE         0.5f





ERet FMOD_RESULT_TO_ERet( FMOD_RESULT result )
{
	switch( result )
	{
	case FMOD_OK:
		return ALL_OK;

	default:
		return ERR_UNKNOWN_ERROR;
	}
}

static mxFORCEINLINE
const FMOD_VECTOR FMOD_VECTOR_from_V3f( const V3f& v )
{
	// FMOD uses a left handed co-ordinate system by default.
    // We use a right handed co-ordinate system.
	FMOD_VECTOR	fmod_vector;
	fmod_vector.x = -v.x;
	fmod_vector.y = v.y;
	fmod_vector.z = v.z;
	return fmod_vector;
}




/*
=======================================================================
	NwSoundSystem_FMOD
=======================================================================
*/
NwSoundSystem_FMOD::NwSoundSystem_FMOD()
	: _audio_file_loader( MemoryHeaps::audio() )
	, _sound_shader_loader( MemoryHeaps::audio() )
{
	//
}

ERet NwSoundSystem_FMOD::initialize(
							   NwClump* audio_assets_storage
							   , int rng_seed
							   )
{
	// Create the main system object.
	nwFMOD_CALL(FMOD::System_Create(&fmod_system._ptr));

	//
	unsigned int      version;
	nwFMOD_CALL(fmod_system->getVersion(&version));

	if( version < FMOD_VERSION )
	{
		ptERROR("Error! You are using an old version of FMOD: %08x. This program requires %08x\n", version, FMOD_VERSION);
		return ERR_INCOMPATIBLE_VERSION;
	}

	//
	int	numdrivers;
    nwFMOD_CALL(fmod_system->getNumDrivers(&numdrivers));

	if (numdrivers == 0)
	{
		nwFMOD_CALL(fmod_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND));
	}


	// Initialize FMOD.

	// The maximum number of channels to be used in FMOD.
	// They are also called 'virtual channels' as you can play as many of these as you want,
	// even if you only have a small number of hardware or software voices.
	const int maxchannels = 512;	// includes virtual voices

	nwFMOD_CALL(fmod_system->init(
		maxchannels,

		//FMOD_INIT_3D_RIGHTHANDED	// FMOD will treat +X as right, +Y as up and +Z as backwards (towards you).
		FMOD_INIT_NORMAL,

		NULL	// void *extradriverdata
		));

	//
	int	numhardwarechannels;
	nwFMOD_CALL(fmod_system->getHardwareChannels(&numhardwarechannels));
	DBGOUT("[FMOD] Using %d hardware channels!", numhardwarechannels);

	//// Set the output to 5.1.
	//nwFMOD_CALL(fmod_system->setSpeakerMode(FMOD_SPEAKERMODE_5POINT1));


	// all 1.0's
	//float	dopplerscale, distancefactor, rolloffscale;
	//nwFMOD_CALL(fmod_system->get3DSettings(
	//	&dopplerscale, &distancefactor, &rolloffscale
	//	));

    /*
        Set the distance units. (meters/feet etc).
    */
#if 1
    nwFMOD_CALL(fmod_system->set3DSettings(
		DOPPLER_SCALE,	// dopplerscale
		DISTANCE_FACTOR,	// distancefactor
		ROLLOFF_SCALE	// rolloffscale
		));
#endif
	//
	nwFMOD_CALL(fmod_system->createChannelGroup(
		"main",
		&fmod_main_channelgroup._ptr
		));

	nwFMOD_CALL(fmod_system->createChannelGroup(
		"music",
		&fmod_music_channelgroup._ptr
		));

	nwFMOD_CALL(fmod_system->createChannelGroup(
		"fightmusic",
		&fmod_fight_music_channelgroup._ptr
		));

	//
	_audio_assets_storage = audio_assets_storage;

	_rng.SetSeed( rng_seed );

	_setupAssetLoaders();

	return ALL_OK;
}

void NwSoundSystem_FMOD::shutdown()
{
	_tearDownAssetLoaders();

	fmod_system->release();
}

void NwSoundSystem_FMOD::_setupAssetLoaders()
{
	NwAudioClip::metaClass().loader = &_audio_file_loader;
	NwSoundSource::metaClass().loader = &_sound_shader_loader;
}

void NwSoundSystem_FMOD::_tearDownAssetLoaders()
{
	NwAudioClip::metaClass().loader = nil;
	NwSoundSource::metaClass().loader = nil;
}

ERet NwSoundSystem_FMOD::UpdateOncePerFrame()
{
	nwFMOD_CALL(fmod_system->update());
	return ALL_OK;
}

void NwSoundSystem_FMOD::UpdateListener(
	const V3f& position
	, const V3f& forward_direction
	, const V3f& up_direction
	, const V3f* velocity_meters_per_second
	)
{
	const FMOD_VECTOR	fmod_position = FMOD_VECTOR_from_V3f(position);
	const FMOD_VECTOR	fmod_forward = FMOD_VECTOR_from_V3f(forward_direction);
	const FMOD_VECTOR	fmod_up = FMOD_VECTOR_from_V3f(up_direction);

	FMOD_VECTOR		fmod_velocity;
	FMOD_VECTOR *	fmod_velocity_ptr = nil;
	if( velocity_meters_per_second )
	{
		fmod_velocity = FMOD_VECTOR_from_V3f(*velocity_meters_per_second);
		fmod_velocity_ptr = &fmod_velocity;
	}

	//
	fmod_system->set3DListenerAttributes(
		0	// int listener
		, &fmod_position

		// Velocity is only required if you want doppler effects.
		// Otherwise you can pass 0 or NULL to both System::set3DListenerAttributes and Channel::set3DAttributes
		// for the velocity_meters_per_second parameter, and no doppler effect will be heard.
		, fmod_velocity_ptr

		, &fmod_forward
		, &fmod_up
		);
}


mxSTOLEN("Doom3 BFG, snd_local.h");
/// decibelToLinearScaleGain
static mxFORCEINLINE
float DBtoLinear( float db )
{
	//return mmPow( 2.0f, db * ( 1.0f / 6.0f ) );
	return mmPow( 10.0f, db * ( 1.0f / 20.0f ) );
}

/// linearScaleGainToDecibel
static mxFORCEINLINE float LinearToDB( float linear )
{
	// DB = 20*log10(volume)
	return 20.0f * log10(linear);
	//return ( linear > 0.0f ) ? ( logf( linear ) * ( 6.0f / 0.693147181f ) ) : -999.0f;
}


ERet NwSoundSystem_FMOD::PlaySound3D(
									 const AssetID& sound_shader_id
									 , const V3f& position
									 , const V3f* velocity_meters_per_second
									 )
{
	NwSoundSource *	sound_shader;
	mxDO(Resources::Load( sound_shader, sound_shader_id, _audio_assets_storage ));

	//
	NwAudioClip *	sound;
	mxTRY(sound_shader->getRandomSound(
		&sound
		, _rng
		));

	// NB: it's important to start the sound in paused state,
	// modify its 3D attributes, and then up-pause it -
	// - otherwise, we get clicking/ticking/crackling noise
	// (or, maybe, I'm doing something wrong).

	FMOD::Channel *	channel;
	nwFMOD_CALL(fmod_system->playSound(

		// It's generally recommended to always be FMOD_CHANNEL_FREE.
		// This will mean FMOD will choose a non-playing channel for you to play on.
		FMOD_CHANNEL_FREE

		, sound->fmod_sound
		, true	// FMOD_BOOL paused
		, &channel	// FMOD_CHANNEL **channel
		));

	nwFMOD_CALL(channel->setChannelGroup(fmod_main_channelgroup));

	//
	const bool is_3D_sound = !(sound_shader->flags & NwSoundFlags::Is2D);
	// Avoid FMOD error:
	// 'Tried to call a command on a 2d sound when the command was meant for 3d sound. ' (47)
	if(is_3D_sound)
	{
		const FMOD_VECTOR	fmod_position = FMOD_VECTOR_from_V3f(position);

		FMOD_VECTOR		fmod_velocity;
		FMOD_VECTOR *	fmod_velocity_ptr = nil;
		if( velocity_meters_per_second )
		{
			fmod_velocity = FMOD_VECTOR_from_V3f(*velocity_meters_per_second);
			fmod_velocity_ptr = &fmod_velocity;
		}

		// Sets the position and velocity of a 3D channel.
		nwFMOD_CALL(channel->set3DAttributes(
			// Position in 3D space of the channel. Specifying 0 / null will ignore this parameter.
			&fmod_position,
			// Velocity in 'distance units per second' in 3D space of the channel. Specifying 0 / null will ignore this parameter.
			fmod_velocity_ptr
			));

		//
		//	mxBUG("Sound loading");
		#if 1
			nwFMOD_CALL(channel->set3DMinMaxDistance(
				sound_shader->min_distance,
				sound_shader->max_distance
				));
		#else
		nwFMOD_CALL(channel->set3DMinMaxDistance(
			// While the player is within this distance in meters from the sound source the volume will remain a the defined Volume, 0 to inf.
			100,//NOTE: 0 - no sound is heard
			// When the player is this far away in meters from the sound source the volume will be 0, 0 to inf.
			10000
			));
		#endif

		//nwFMOD_CALL(channel->set3DDopplerLevel(
		//	0// no Doppler
		//	));
	}



	// Sets the volume for the channel linearly.

	float01_t linear_volume = DBtoLinear( sound_shader->volume );
	//mxASSERT(linear_volume >= 0 && linear_volume <= 1);
	linear_volume = clampf(linear_volume, 0, 1);

	//
	nwFMOD_CALL(channel->setVolume(
		// A linear volume level, from 0.0 to 1.0 inclusive. 0.0 = silent, 1.0 = full volume. Default = 1.0.
		linear_volume
		));


	//
	nwFMOD_CALL(channel->setPaused(false));


#if MX_DEBUG

	if(s_log_PlaySound3D) {
		DBGOUT("[Sound] Play '%s', volume = %f",
			sound_shader_id.c_str(), linear_volume
			);
	}

#endif // MX_DEBUG

	return ALL_OK;
}

ERet NwSoundSystem_FMOD::PlayAudioClip(
	const AssetID& audio_clip_id
	, const int play_flags /*= 0*/	// PlayAudioClipFlags
	, const float volume01 /*= 1*/
	, const E_SOUND_CHANNEL snd_channel /*= SND_CHANNEL_MAIN*/
	)
{
	LoadFlagsT	load_flags = FMOD_2D;
	if( play_flags & PlayAudioClip_Looped ) {
		load_flags |= FMOD_LOOP_NORMAL;
	}

	//
	NwAudioClip *	audio_clip;
	mxDO(Resources::Load(
		audio_clip
		, audio_clip_id
		, _audio_assets_storage
		, load_flags
		));

	//
	FMOD::Channel *	channel;
	//
	nwFMOD_CALL(fmod_system->playSound(
		FMOD_CHANNEL_FREE	// FMOD_CHANNELINDEX
		, audio_clip->fmod_sound
		, true	// FMOD_BOOL paused
		, &channel	// FMOD_CHANNEL **channel
		));

	//// this is a HACK!!!!
	//if(play_flags & PlayAudioClip_Reset)
	//{
	//	channel->stop();
	//	nwFMOD_CALL(fmod_system->playSound(
	//		FMOD_CHANNEL_REUSE	// FMOD_CHANNELINDEX
	//		, audio_clip->fmod_sound
	//		, true	// FMOD_BOOL paused
	//		, &channel	// FMOD_CHANNEL **channel
	//		));
	//}

	nwFMOD_CALL(channel->setChannelGroup(
		_GetChannelGroup(snd_channel)
		));

	// Sets the volume for the channel linearly.
	nwFMOD_CALL(channel->setVolume(
		// A linear volume level, from 0.0 to 1.0 inclusive. 0.0 = silent, 1.0 = full volume. Default = 1.0.
		 volume01
		));

	nwFMOD_CALL(channel->setPaused(false));

	return ALL_OK;
}

FMOD::ChannelGroup* NwSoundSystem_FMOD::_GetChannelGroup(E_SOUND_CHANNEL channel) const
{
	switch(channel)
	{
	case SND_CHANNEL_MAIN:
		return fmod_main_channelgroup;

	case SND_CHANNEL_MUSIC:
		return fmod_music_channelgroup;

	case SND_CHANNEL_MUSIC_FIGHT:
		return fmod_fight_music_channelgroup;

	default:
		// avoid crashes - return the main channel group
		mxENSURE(false, fmod_main_channelgroup, "");
	}
}

ERet NwSoundSystem_FMOD::SetVolume(
	E_SOUND_CHANNEL channel
	, float01_t new_volume
	)
{
	TPtr< FMOD::ChannelGroup >	fmod_channel_group;

	switch(channel)
	{
	case SND_CHANNEL_MAIN:
		fmod_channel_group = fmod_main_channelgroup;
		break;

	case SND_CHANNEL_MUSIC:
		fmod_channel_group = fmod_music_channelgroup;
		break;

	case SND_CHANNEL_MUSIC_FIGHT:
		fmod_channel_group = fmod_fight_music_channelgroup;
		break;

	default:
		mxENSURE(false, ERR_INVALID_PARAMETER, "");
	}

	fmod_channel_group->setVolume(new_volume);

	return ALL_OK;
}

ERet NwSoundSystem_FMOD::SetMasterVolume(float01_t master_volume)
{
	FMOD::ChannelGroup *	master_channelgroup;
	nwFMOD_CALL(fmod_system->getMasterChannelGroup(
		&master_channelgroup
		));

	master_channelgroup->setVolume(master_volume);
	return ALL_OK;
}

#if 0

ERet NwSoundSystem_FMOD::SetAmbientReverb()
{
	FMOD_REVERB_PROPERTIES	reverb_props;
	UNDONE;

	nwFMOD_CALL(fmod_system->setReverbAmbientProperties(
		&reverb_props
		));
	return ALL_OK;
}

#endif
