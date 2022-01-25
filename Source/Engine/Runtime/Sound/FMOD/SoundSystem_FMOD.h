#pragma once

#include <FMOD/fmod.hpp>
#include <FMOD/fmod_errors.h>

#if mxARCH_TYPE == mxARCH_64BIT
#pragma comment( lib, "fmodex64_vc.lib" )
#else
#pragma comment( lib, "fmodex_vc.lib" )
#endif

//
#include <Sound/SoundSystem.h>




ERet FMOD_RESULT_TO_ERet( FMOD_RESULT result );


#if MX_DEBUG || MX_DEVELOPER

	#define nwFMOD_CALL( X )\
		mxMACRO_BEGIN\
			const FMOD_RESULT result = (X);\
			if( result != FMOD_OK ){\
				static int s_assert_behavior = AB_Break;\
				if( s_assert_behavior != AB_Ignore )\
					if( result != FMOD_OK ){\
						s_assert_behavior = ZZ_OnAssertionFailed( __FILE__, __LINE__, __FUNCTION__, #X, "FMOD_Wrapper error: '%s' (%d)\n", FMOD_ErrorString(result), result );\
						if( s_assert_behavior == AB_Break )\
								ptBREAK;\
					}\
				return FMOD_RESULT_TO_ERet(result);\
			}\
		mxMACRO_END

#else

	#define nwFMOD_CALL( X )\
		mxMACRO_BEGIN\
			const FMOD_RESULT result = (X);\
			if( result != FMOD_OK )\
			{\
				ptERROR("FMOD_Wrapper error: '%s' (%d)\n", FMOD_ErrorString(result), result);\
				return FMOD_RESULT_TO_ERet(result);\
			}\
		mxMACRO_END

#endif



/*
 * handles all the logic for loading, unloading, playing, stopping, and changing sounds
*/
class NwSoundSystem_FMOD: public NwSoundSystemI
{
public:
	TPtr< FMOD::System >	fmod_system;

	TPtr< FMOD::ChannelGroup >	fmod_main_channelgroup;
	TPtr< FMOD::ChannelGroup >	fmod_music_channelgroup;
	TPtr< FMOD::ChannelGroup >	fmod_fight_music_channelgroup;

	//
	NwRandom		_rng;

	TPtr< NwClump >	_audio_assets_storage;

	//
	NwAudioFileLoader	_audio_file_loader;
	NwSoundShaderLoader	_sound_shader_loader;

public:
	NwSoundSystem_FMOD();

	virtual ERet initialize(
		NwClump* audio_assets_storage
		, int rng_seed = 0
		) override;

	virtual void shutdown() override;

	virtual ERet UpdateOncePerFrame() override;

	virtual void UpdateListener(
		const V3f& position
		, const V3f& forward_direction
		, const V3f& up_direction
		, const V3f* velocity_meters_per_second = nil
		) override;

	virtual ERet PlaySound3D(
		const AssetID& sound_shader_id
		, const V3f& position
		, const V3f* velocity_meters_per_second = nil
		) override;

	/// aka Play 2D-Sound
	virtual ERet PlayAudioClip(
		const AssetID& audio_clip_id
		, const int play_flags = 0	// PlayAudioClipFlags
		, const float volume01 = 1
		, const E_SOUND_CHANNEL channel = SND_CHANNEL_MAIN
		) override;

	virtual ERet SetVolume(
		E_SOUND_CHANNEL channel
		, float01_t new_volume
		) override;

	virtual ERet SetMasterVolume(float01_t master_volume) override;


private:
	void _setupAssetLoaders();
	void _tearDownAssetLoaders();

	//
	FMOD::ChannelGroup* _GetChannelGroup(E_SOUND_CHANNEL channel) const;
};
