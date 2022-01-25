#pragma once

#include <Base/Base.h>
#include <Base/Math/Random.h>
#include <Base/Template/Containers/Blob.h>

#include <Core/ObjectModel/Clump.h>
#include <Core/Assets/Asset.h>	// NwResource
#include <Core/Assets/AssetLoader.h>	// Resource loaders
#include <Core/Assets/AssetReference.h>	// TResPtr<>


namespace FMOD
{
    class Sound;
}//namespace FMOD


struct NwSoundFlags
{
	enum Enum
	{
		/// true for background music
		Is2D	= BIT(0),

		/// true for background music
		Looping	= BIT(1),

		Defaults = 0
	};
};
mxDECLARE_FLAGS( NwSoundFlags::Enum, U32, SoundFlagsT );

/// sound properties
struct NwSoundDef: CStruct
{
	/// not reflected
	//U32				fourcc;

	SoundFlagsT		flags;

public:
	mxDECLARE_CLASS( NwSoundDef, CStruct );
	mxDECLARE_REFLECTION;
	NwSoundDef();

	enum { FOUR_CC = MCHAR4( 'S', 'N', 'D', 'F' ) };
};

/*
*/
class NwAudioClip: public NwSharedResource
{
public_internal:
	TPtr< FMOD::Sound >	fmod_sound;

	//AssetID		dbg_id;

public:
	mxDECLARE_CLASS( NwAudioClip, NwSharedResource );
	mxDECLARE_REFLECTION;

	NwAudioClip();
};

///
class NwAudioFileLoader: public TbAssetLoader_Null
{
	typedef TbAssetLoader_Null Super;

public:
	NwAudioFileLoader(
		ProxyAllocator & parent_allocator
		);

	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual ERet reload( NwResource * instance, const TbAssetLoadContext& context ) override;
};


/// so that this struct can be read as a whole
struct NwSoundSourceData
{
	float		min_distance;
	float		max_distance;

	/// The maximum volume for this sound (volume inside minDistance)
	/// in dB.  Negative values get quieter
	float		volume;

	SoundFlagsT		flags;

public:
	NwSoundSourceData()
	{
		// default values used by FMOD
		min_distance = 1;
		max_distance = 10000;
		volume = 1;
		flags = NwSoundFlags::Defaults;
	}
};
ASSERT_SIZEOF(NwSoundSourceData, 16);

/*
 * modeled after Doom 3's idSoundShader
*/
class NwSoundSource
	: public NwSharedResource
	, public NwSoundSourceData
{
public_internal:
	/// the next sound to play will be picked randomly
	TArray< TResPtr< NwAudioClip > >	sounds;

	// reduce dynamic memory allocations
	enum { PREALLOCED_SOUNDS = 6 };	// "gibs.sndshd"/gib_item_splat has 6 sounds
	TReservedStorage< TResPtr< NwAudioClip >, PREALLOCED_SOUNDS >	sounds_storage;

public:
	mxDECLARE_CLASS( NwSoundSource, NwSharedResource );
	mxDECLARE_REFLECTION;

	NwSoundSource();

	ERet ensureLoaded(LoadFlagsT fmod_sound_flags);

	ERet getRandomSound(
		NwAudioClip **sound_
		, NwRandom & rng
		) const;
};

///
class NwSoundShaderLoader: public TbAssetLoader_Null
{
	typedef TbAssetLoader_Null Super;
public:
	NwSoundShaderLoader( ProxyAllocator & parent_allocator );

	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual ERet reload( NwResource * instance, const TbAssetLoadContext& context ) override;
};
