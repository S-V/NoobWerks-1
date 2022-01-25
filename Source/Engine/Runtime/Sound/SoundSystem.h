#pragma once

#include <FMOD/fmod.h>
#include <FMOD/fmod_errors.h>

#if mxARCH_TYPE == mxARCH_64BIT
#pragma comment( lib, "fmodex64_vc.lib" )
#else
#pragma comment( lib, "fmodex_vc.lib" )
#endif

#include <Base/Base.h>
#include <Base/Math/Random.h>
#include <Base/Template/Containers/Blob.h>

#include <Core/ObjectModel/Clump.h>
#include <Core/Assets/Asset.h>	// NwResource
#include <Core/Assets/AssetLoader.h>	// Resource loaders
#include <Core/Assets/AssetReference.h>	// TResPtr<>

//
#include <Sound/SoundResources.h>

enum E_SOUND_CHANNEL
{
	SND_CHANNEL_MAIN,
	SND_CHANNEL_MUSIC,
	SND_CHANNEL_MUSIC_FIGHT,
};


enum PlayAudioClipFlags
{
	PlayAudioClip_Looped = BIT(0),
	PlayAudioClip_Reset = BIT(1),
};

/*
 * handles all the logic for loading, unloading, playing, stopping, and changing sounds
*/
class NwSoundSystemI: public TGlobal< NwSoundSystemI >
{
public:
	virtual ERet initialize(
		NwClump* audio_assets_storage
		, int rng_seed = 0
		) = 0;

	virtual void shutdown() = 0;

	/// needed to update 3d engine, once per frame.
	virtual ERet UpdateOncePerFrame() = 0;

	/// update 'ears'
	virtual void UpdateListener(
		const V3f& position
		, const V3f& forward_direction
		, const V3f& up_direction
		/// The velocity of the listener measured in distance units per second.
		, const V3f* velocity_meters_per_second = nil
		) = 0;


	virtual ERet SetVolume(
		E_SOUND_CHANNEL channel
		, float01_t new_volume
		) = 0;

	/// set the master volume for all playing sounds
	virtual ERet SetMasterVolume(float01_t master_volume) = 0;

public:	// Sound Sources

	virtual ERet PlaySound3D(
		const AssetID& sound_shader_id
		, const V3f& position
		/// The velocity of the listener measured in distance units per second.
		, const V3f* velocity_meters_per_second = nil
		) = 0;

	/// aka Play 2D-Sound
	virtual ERet PlayAudioClip(
		const AssetID& audio_clip_id
		, const int play_flags = 0	// PlayAudioClipFlags
		, const float volume01 = 1
		, const E_SOUND_CHANNEL channel = SND_CHANNEL_MAIN
		) = 0;

#if 0
	PResult play();
	PResult stop();
	PResult kill();
	PResult pause();
	PResult resume();
	
	//! Event attributes
	PResult setVolume(float volume);
	PResult set3DAttributes(const Vectormath::Aos::Vector3 *position, const Vectormath::Aos::Vector3 *velocity = NULL, const Vectormath::Aos::Vector3 *direction = NULL);
#endif

public:


#if MX_DEBUG

	static bool s_log_PlaySound3D;

#endif // MX_DEBUG


protected:
	virtual ~NwSoundSystemI() {}
};
