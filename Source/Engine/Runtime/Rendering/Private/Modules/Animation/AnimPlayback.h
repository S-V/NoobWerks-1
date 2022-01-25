// animations sequencing, sampling and blending/mixing
#pragma once

#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Rendering/Private/Modules/Animation/PlaybackController.h>


namespace Rendering
{
struct NwAnimClip;


/// higher priority clips dominate lower priority clips.
/// Thus a high priority clip with a blend weight of 1.0
/// will completely obscure any previous lower priority clips.
/// 
/// The typical priority order is:
/// 1. Action (e.g. Fire) (Highest Priority)
/// 2. Movement
/// 3. Idle (Lowest Priority)
///
struct NwAnimPriority
{
	typedef U8 Value;

	enum Enum
	{
		Bad = 0,	// for internal use only!
		Lowest = 1,
		Medium = 128,
		Required = 255
	};
};


/// Typically, when crossfading from one animation to another, the magic number is to blend over 0.2 seconds.
static const int DEFAULT_ANIMATION_BLEND_TIME_MSEC = 200;

/// because we it in 16-bit ints
static const int MAX_ANIMATION_DURATION_MSEC = MAX_UINT16;


struct PlayAnimFlags
{
	enum Enum
	{
		/// do not restart anim
		/// when adding an anim which is already playing
		DoNotRestart = BIT(0),

		Looped		= BIT(1),

		//RemoveWhenFinished	= BIT(2),

		Defaults = 0
	};
	typedef U32 Raw;
};


///
struct NwPlayAnimParams
{
	// Start from current weighting, will ramp up to destination weighting (default 100%) over a second or so
	//float	start_blend_weight;

	/// The final weight used for priority blending in between the fade-in and fade-out period.
	float01_t	target_weight;	// default = 1, i.e. fade-in

	// The crossfading time
	// Here uint16 saves memory and is guaranteed to be positive:
	MillisecondsUInt16	fade_in_time_msec;
	MillisecondsUInt16	fade_out_time_msec;

	/// An animation can be interrupted before it is completed.
	MillisecondsUInt16	fade_out_start_time_msec;

	///
	NwAnimPriority::Value	priority;

	// 12

public:
	NwPlayAnimParams(ENoInit)
	{}

	NwPlayAnimParams()
	{
		SetDefaults();
	}

	void SetDefaults()
	{
		priority = NwAnimPriority::Required;

		target_weight = 1;
		//max_time_ratio = 1;

		//base_time_msec = 0;

		fade_in_time_msec = DEFAULT_ANIMATION_BLEND_TIME_MSEC;	// 0.2 seconds
		fade_out_time_msec = DEFAULT_ANIMATION_BLEND_TIME_MSEC;	// 0.2 seconds

		fade_out_start_time_msec = ~0;
	}

	NwPlayAnimParams& SetPriority( NwAnimPriority::Value new_priority ) {
		priority = new_priority;
		return *this;
	}
	NwPlayAnimParams& SetFadeInTime( MillisecondsT fade_in_time_msec ) {
		fade_in_time_msec = fade_in_time_msec;
		return *this;
	}
	NwPlayAnimParams& SetFadeOutTime( MillisecondsT fade_out_time_msec ) {
		fade_out_time_msec = fade_out_time_msec;
		return *this;
	}
	NwPlayAnimParams& SetBlendWeight( float01_t new_blend_weight ) {
		mxASSERT(new_blend_weight >= 0 && new_blend_weight <= 1);
		target_weight = new_blend_weight;
		return *this;
	}
	//NwPlayAnimParams& SetMaxTimeRatio( float01_t new_max_time_ratio ) {
	//	mxASSERT(max_time_ratio >= 0 && max_time_ratio <= 1);
	//	max_time_ratio = new_max_time_ratio;
	//	return *this;
	//}
};


struct NwStartAnimParams
{
	float01_t	start_time_ratio;

	// sets the playback speed of the animation
	float		speed;	// default = 1

public:
	NwStartAnimParams()
	{
		start_time_ratio = 0;
		speed = 1;
	}
	NwStartAnimParams& SetStartTimeRatio( float new_start_time_ratio ) {
		start_time_ratio = new_start_time_ratio;
		return *this;
	}
	NwStartAnimParams& SetSpeed( float speed_multiplier ) {
		speed = speed_multiplier;
		return *this;
	}
};


/// Stores the result of an animation sampling operation
struct NwAnimSamplingCache: CStruct
{
	// Runtime anim sampling cache.
	ozz::animation::SamplingCache	ozz_sampling_cache;

	//32: 60

public:
	mxDECLARE_CLASS( NwAnimSamplingCache, CStruct );
	mxDECLARE_REFLECTION;

	enum { GRANULARITY = 32 };
};


/// helper struct for emitting animation events
struct NwPlayingAnim
{
	const NwAnimClip *		anim_clip;
	NwAnimSamplingCache *	sampling_cache;

	float01_t	blend_weight;

	float01_t	prev_time_ratio;
	float01_t	curr_time_ratio;
};

/// plays and blends animations
class NwAnimPlayer: NonCopyable
{
public_internal:
	enum
	{
		/// The maximum number of simultaneously running animations.
		/// Don't go overboard, 2-3 is fine.
		/// In fact, a large number can cause performance spikes
		/// when too many animations are blended.
		MAX_SIMULTANEOUS_ANIMS = 2,

		MAX_SCHEDULED_ANIMS = 4,
	};

	/// Internal information about a single playing animation clip within Animation.
	/// This structure contains all the data required to sample a single animation.
	struct AnimState
	{
		TPtr< const NwAnimClip >	anim;

		NwPlaybackController	playback_controller;

		// mostly immutable data
		NwPlayAnimParams	params;

		// copied from anim and cached to avoid pointer chasing
		MillisecondsUInt16		anim_duration_msec;	// immutable/'const'

		//
		float01_t		current_blend_weight;

		//
		TPtr< NwAnimSamplingCache >	sampling_cache;	// owned

		//32: 44
	};

	// sorted by decreasing priority
	TFixedArray< AnimState, MAX_SCHEDULED_ANIMS >	_active_anims;

	// scheduled anims, can be started in the future
	//TFixedArray< NwPlayAnimParams, MAX_SCHEDULED_ANIMS >	_scheduled_anims;

	//32: 248

public:
	NwAnimPlayer();

	void PlayAnim(
		const NwAnimClip* anim_clip
		, const NwPlayAnimParams& params = NwPlayAnimParams()
		, const PlayAnimFlags::Raw play_anim_flags = PlayAnimFlags::Defaults
		, const NwStartAnimParams& start_params = NwStartAnimParams()
		);

	/// returns true if anim was not already playing
	bool PlayAnimIfNotPlayingAlready(
		const NwAnimClip* anim_clip
		, const NwPlayAnimParams& params = NwPlayAnimParams()
		, const PlayAnimFlags::Raw play_anim_flags = PlayAnimFlags::Defaults
		, const NwStartAnimParams& start_params = NwStartAnimParams()
		);

	void SetActiveAnimsPaused(bool pause);

	// 1 == normal speed, 0 == paused
	void SetPlaybackSpeedOfActiveAnims(float playback_speed_multiplier);

	void RemoveAnimationsPlayingClip(const NwAnimClip* anim_clip);

	int NumPlayingAnims() const { return _active_anims.num(); }

	SecondsF RemainingPlayTime(const NwAnimClip* anim_clip) const;

	ERet FadeOutAnim(
		const NwAnimClip* anim_clip
		//
		, const MillisecondsU32 max_fade_out_time_msec = DEFAULT_ANIMATION_BLEND_TIME_MSEC
		);

	//ERet ModifyAnimParams(
	//	const NwAnimClip* anim_clip
	//	, const NwPlayAnimParams& new_params 
	//	);
	ERet SetLooping(
		const NwAnimClip* anim_clip
		, const bool loop
		);

public:

	struct UpdateOutput
	{
		UINT			anims_count;
		NwPlayingAnim	playing_anims[MAX_SIMULTANEOUS_ANIMS];

	public:
		UpdateOutput() { anims_count = 0; }
	};

	void UpdateOncePerFrame(
		//const MillisecondsT current_time
		const SecondsF delta_seconds
		, UpdateOutput &output_
		);

public:
	bool IsPlayingAnim(const NwAnimClip* animation) const;
	int FindPlayingAnimIndex(const NwAnimClip* animation) const;

	///
	const V3f averageLinearSpeed() const;

#if MX_DEVELOPER
	
	void DbgDump(String &dbg_desc_) const;

	void DbgSimulate();

#endif

private:

	UINT _FindDstIndexForInsertingAnim(const NwAnimPriority::Value new_anim_priority) const;

	ERet _AllocateSamplingCache(
		AnimState & playing_anim
		);
	void _ReleaseSamplingCache(
		AnimState & playing_anim
		);

/*
/// Smoothly changes to an animation. Returns the play duration in seconds
	float TransitionToAnimation(ANIMATIONHANDLE animation, float transitionTime = 1.1f, float weight = 1, float startAtTime = 0, float playSpeed = 1);

	/// Weight for animation, if blending multiple animations
	void SetAnimationWeight(ANIMATIONHANDLE animation, float weight, float transitionTime=1.1f);
*/

	//void Initialize(
	//	BlendLayer* blend_layers
	//	, U32 num_blend_layers
	//	);

#if 0
	/// Sets the given animation, taking 'transition_duration' seconds to blend from
	/// the current animation state to the start of the target animation.
	/// TransitionDuration is the time (in seconds) when when previous animation fades-out
	/// and new animation fades-in, usually values around 0.1-0.5 make sensible smooth effect.
	/// TransitionDuration = 0 (default) means that no animation blending occurs.
	void PlayAnim_Exclusive(
		const NwAnimClip* new_animation
		//, const NameHash32 anim_name_hash
		, F32 transition_duration = TB_DEFAULT_ANIMATION_BLEND_TIME
		, F32 target_blend_weight = 1.0f
		, const PlayAnimFlags::Raw flags = PlayAnimFlags::Defaults
		);

	int PlayAnim_Additive(
		const NwAnimClip* new_animation
		, F32 transition_duration = TB_DEFAULT_ANIMATION_BLEND_TIME
		, F32 target_blend_weight = 1.0f
		, const PlayAnimFlags::Raw flags = PlayAnimFlags::Defaults
		);

	void FadeOut(
		F32 transition_duration = TB_DEFAULT_ANIMATION_BLEND_TIME
		);

	/// fade other playing animations
	void FadeOthers(
		const int this_anim_index
		, F32 transition_duration = TB_DEFAULT_ANIMATION_BLEND_TIME
		, F32 target_blend_weight = 0.0f
		);
#endif

};






struct NwAnimatedModel;

namespace AnimJobs
{
	///
	struct ComputeJointMatricesJob
	{
		NwAnimPlayer::UpdateOutput anim_update_output;

		NwAnimatedModel & anim_model;

	public:
		ComputeJointMatricesJob(
			NwAnimatedModel & anim_model
			, const NwAnimPlayer::UpdateOutput& anim_update_output
			)
			: anim_model(anim_model)
			, anim_update_output(anim_update_output)
		{}

		ERet Run( const NwThreadContext& context, int start, int end );
	};
	mxSTATIC_ASSERT(sizeof(ComputeJointMatricesJob) <= sizeof(NwJobData));

}//namespace AnimJobs

}//namespace Rendering
