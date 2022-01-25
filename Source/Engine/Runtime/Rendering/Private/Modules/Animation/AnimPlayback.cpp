#include <Base/Base.h>
#pragma hdrstop

#include <Rendering/Private/Modules/Animation/AnimClip.h>
#include <Rendering/Private/Modules/Animation/AnimatedModel.h>
#include <Rendering/Private/Modules/Animation/_AnimationSystem.h>
#include <Rendering/Private/Modules/Animation/AnimPlayback.h>


#define nwDEBUG_ANIMS	(0 && MX_DEVELOPER)

#define nwDBG_SLOW_DOWN_ANIMS	(0 && MX_DEVELOPER)

#if nwDBG_SLOW_DOWN_ANIMS
	#define nwDBG_SLOW_DOWN_FACTOR	(0.1f)
#endif


namespace Rendering
{
//tbPRINT_SIZE_OF(NwPlayAnimParams);


mxDEFINE_CLASS(NwAnimSamplingCache);
mxBEGIN_REFLECTION(NwAnimSamplingCache)
mxEND_REFLECTION;
//tbPRINT_SIZE_OF(NwAnimSamplingCache);


//=====================================================================

//tbPRINT_SIZE_OF(NwAnimPlayer);
//tbPRINT_SIZE_OF(NwAnimPlayer::AnimState);


NwAnimPlayer::NwAnimPlayer()
{
}


bool NwAnimPlayer::IsPlayingAnim(const NwAnimClip* animation) const
{
	return this->FindPlayingAnimIndex(animation) != INDEX_NONE;
}

int NwAnimPlayer::FindPlayingAnimIndex(const NwAnimClip* animation) const
{
	for( int i = 0; i < _active_anims._count; i++ )
	{
		const AnimState& active_anim = _active_anims._data[ i ];
		if( active_anim.anim == animation ) {
			return i;
		}
	}
	return INDEX_NONE;
}

const V3f NwAnimPlayer::averageLinearSpeed() const
{
	V3f	animation_speed = CV3f(0);
UNDONE;
	//for( int i = 0; i < num_playing_anims; i++ )
	//{
	//	const AnimState& anim_state = *_active_anims[ i ];

	//	animation_speed += anim_state.animation->anim_clip->speed * anim_state.current_weight;
	//}

	return animation_speed;
}


#if MX_DEVELOPER

void NwAnimPlayer::DbgDump(String &dbg_desc_) const
{
	StringStream	stream(dbg_desc_);

	stream,"Num anims: ",_active_anims.num(),"\n";
	nwFOR_EACH_INDEXED(const AnimState& anim_state, _active_anims, i)
	{
		stream,"[",i,"]: anim",anim_state.anim->dbgName(),", weight: ",anim_state.current_blend_weight,"\n";
	}
}

void NwAnimPlayer::DbgSimulate()
{
	NwAnimPlayer::UpdateOutput	output;

	for(int i = 0; i < 10; i++)
	{
		this->UpdateOncePerFrame(
			16.0f/1000.0f,
			output
			);
	}
}

#endif

//=====================================================================

mxSTOLEN("Nebula 3, AnimJob::FixFadeTimes()");
/**
    This method checks if the fade-in plus the fade-out times would 
    be bigger then the play-duration, if yes it will fix the fade times
    in order to prevent "blend-popping".
*/
static void FixFadeTimes(
						 NwPlayAnimParams & params
						 , const SecondsF anim_duration_seconds
						 )
{
	const MillisecondsT anim_duration_msec = SEC2MS(anim_duration_seconds);
	const MillisecondsT total_fade_time_msec = params.fade_in_time_msec + params.fade_out_time_msec;

	if(total_fade_time_msec && total_fade_time_msec > anim_duration_msec)
	{
		const float01_t scale = float(anim_duration_msec) / float(total_fade_time_msec);
		params.fade_in_time_msec = MillisecondsT(params.fade_in_time_msec * scale);
		params.fade_out_time_msec = MillisecondsT(params.fade_out_time_msec * scale);
	}

	//
	params.fade_out_start_time_msec = smallest(
		params.fade_out_start_time_msec,
		anim_duration_msec - params.fade_out_time_msec
		);
}

void NwAnimPlayer::PlayAnim(
	const NwAnimClip* anim_clip
	, const NwPlayAnimParams& params
	, const PlayAnimFlags::Raw play_anim_flags
	, const NwStartAnimParams& start_params
	)
{
	mxASSERT_PTR(anim_clip);
	mxASSERT(params.fade_in_time_msec >= 0);
	mxASSERT(params.fade_out_time_msec >= 0);

#if nwDEBUG_ANIMS
	DBGOUT("PlayAnim: '%s'", anim_clip->dbgName());
#endif

	const SecondsF anim_duration_seconds = anim_clip->ozz_animation.duration();

	const bool anim_should_be_looped = ( play_anim_flags & PlayAnimFlags::Looped );

	//
	const int existing_active_anim_index = this->FindPlayingAnimIndex(anim_clip);

	// such animation is already playing
	if( existing_active_anim_index != INDEX_NONE )
	{
		// don't add the animation twice if it is already playing

		AnimState & existing_active_anim = _active_anims[ existing_active_anim_index ];

		if( !(play_anim_flags & PlayAnimFlags::DoNotRestart) )
		{
			// reset the local clock so that the animation plays from the beginning
			existing_active_anim.playback_controller.reset(start_params.start_time_ratio);
		}

		existing_active_anim.playback_controller.set_loop(anim_should_be_looped);
		existing_active_anim.playback_controller.set_playback_speed(start_params.speed);

		existing_active_anim.params = params;
		if(!anim_should_be_looped) {
			FixFadeTimes(existing_active_anim.params, anim_duration_seconds);
		}
	}
	else
	{
		mxENSURE(!_active_anims.isFull(),
			void(),
			"Cannot play '%s' (max: %d anims)",
			anim_clip->dbgName(), _active_anims.num()
			);

		AnimState	new_active_anim;
		new_active_anim.anim = anim_clip;

		new_active_anim.playback_controller.reset(start_params.start_time_ratio);
		new_active_anim.playback_controller.set_loop(anim_should_be_looped);
		new_active_anim.playback_controller.set_playback_speed(start_params.speed);

#if nwDBG_SLOW_DOWN_ANIMS
	new_active_anim.playback_controller.set_playback_speed(nwDBG_SLOW_DOWN_FACTOR);
#endif

		new_active_anim.anim_duration_msec = SEC2MS(anim_duration_seconds);

		new_active_anim.params	= params;

		if(!anim_should_be_looped) {
			FixFadeTimes(new_active_anim.params, anim_duration_seconds);
		}

		//
		const UINT dst_index = _FindDstIndexForInsertingAnim(params.priority);
		_active_anims.Insert(new_active_anim, dst_index);
	}
}

bool NwAnimPlayer::PlayAnimIfNotPlayingAlready(
	const NwAnimClip* anim_clip
	, const NwPlayAnimParams& params
	, const PlayAnimFlags::Raw play_anim_flags
	, const NwStartAnimParams& start_params
	)
{
	if( !this->IsPlayingAnim(anim_clip) )
	{
		PlayAnim(
			anim_clip
			, params
			, play_anim_flags
			, start_params
			);

		return true;
	}

	return false;
}

UINT NwAnimPlayer::_FindDstIndexForInsertingAnim(const NwAnimPriority::Value new_anim_priority) const
{
	nwFOR_EACH_INDEXED(const AnimState& playing_anim, _active_anims, i)
	{
		if(playing_anim.params.priority <= new_anim_priority) {
			return i;
		}
	}
	return _active_anims.num();
}

void NwAnimPlayer::SetActiveAnimsPaused(bool pause)
{
	nwFOR_LOOP(UINT, i, _active_anims._count)
	{
		_active_anims._data[i].playback_controller._is_playing = !pause;
	}
}

void NwAnimPlayer::SetPlaybackSpeedOfActiveAnims(float playback_speed_multiplier)
{
	nwFOR_LOOP(UINT, i, _active_anims._count)
	{
		_active_anims._data[i].playback_controller._playback_speed = playback_speed_multiplier;
	}
}

void NwAnimPlayer::RemoveAnimationsPlayingClip(const NwAnimClip* anim_clip)
{
	for( int i = _active_anims.num() - 1; i >= 0; i-- )
	{
		if( _active_anims._data[i].anim == anim_clip )
		{
			DBGOUT("Removing anim '%s'.", anim_clip->dbgName());

			_active_anims.RemoveAt(i);
		}
	}

	mxASSERT2(_active_anims.num() > 0, "should always play at least one animation!");
}

SecondsF NwAnimPlayer::RemainingPlayTime(const NwAnimClip* anim_clip) const
{
	nwFOR_EACH_INDEXED(const AnimState& playing_anim, _active_anims, i)
	{
		if(playing_anim.anim == anim_clip)
		{
			const float01_t remaining_time_ratio = 1.0f - playing_anim.playback_controller._time_ratio;
			return MS2SEC(playing_anim.anim_duration_msec) * remaining_time_ratio;
		}
	}
	// the anim is not playing
	return 0;
}

ERet NwAnimPlayer::FadeOutAnim(
							   const NwAnimClip* anim_clip
							   , const MillisecondsU32 max_fade_out_time_msec
							   )
{
	for( int i = 0; i < _active_anims._count; i++ )
	{
		AnimState& anim_state = _active_anims._data[i];
		if( anim_state.anim == anim_clip )
		{

#if nwDEBUG_ANIMS
			DBGOUT("FadeOutAnim: '%s'", anim_clip->dbgName());
#endif
			// Set to non-looping -
			// - otherwise, the anim won't be removed when its weight reaches zero.
			anim_state.playback_controller._is_looping = false;

			//
			const MillisecondsT total_anim_duration_msec = anim_state.anim_duration_msec;

			const MillisecondsT curr_playback_time_msec = MillisecondsT(
				anim_state.playback_controller._time_ratio * total_anim_duration_msec
				);

			// Start fading out NOW!
			anim_state.params.fade_out_start_time_msec = curr_playback_time_msec;

			//
			const MillisecondsT remaining_playback_time_msec = total_anim_duration_msec - curr_playback_time_msec;

			anim_state.params.fade_out_time_msec = smallest(
				max_fade_out_time_msec,
				remaining_playback_time_msec
				);

			return ALL_OK;
		}
	}

	return ERR_OBJECT_NOT_FOUND;
}

//ERet NwAnimPlayer::ModifyAnimParams(
//	const NwAnimClip* anim_clip
//	, const NwPlayAnimParams& new_params 
//	)
//{
//	for( int i = 0; i < _active_anims._count; i++ )
//	{
//		if( _active_anims._data[i].anim == anim_clip )
//		{
//			_active_anims._data[i].params = new_params;
//			return ALL_OK;
//		}
//	}
//	return ERR_OUT_OF_MEMORY;
//}
ERet NwAnimPlayer::SetLooping(
	const NwAnimClip* anim_clip
	, const bool loop 
	)
{
	for( int i = 0; i < _active_anims._count; i++ )
	{
		if( _active_anims._data[i].anim == anim_clip )
		{
			_active_anims._data[i].playback_controller._is_looping = loop;
			return ALL_OK;
		}
	}
	return ERR_OUT_OF_MEMORY;
}

static
float _ComputeCurrentBlendWeight(
	const NwAnimPlayer::AnimState& anim_state
	, bool &anim_is_stopped_
	)
{
	const NwPlaybackController& playback_ctrl = anim_state.playback_controller;
	const NwPlayAnimParams& anim_params = anim_state.params;

	const float01_t target_weight = anim_params.target_weight;

	//
	const MillisecondsT curr_playback_time_msec = MillisecondsT(
		anim_state.anim_duration_msec * playback_ctrl._time_ratio
		);

	// Put most frequent cases first

	if( !playback_ctrl._is_looping )
	{
		//
		if( curr_playback_time_msec >= anim_params.fade_in_time_msec
			&& curr_playback_time_msec < anim_params.fade_out_start_time_msec )
		{
			// A non-looped animation is between the fade-in and fade-out phase.
			return target_weight;
		}
		else if( curr_playback_time_msec < anim_params.fade_in_time_msec )
		{
			// A non-looped animation at the beginning -> fade in.
			const float01_t fade_in_ratio = (float)curr_playback_time_msec / (float)anim_params.fade_in_time_msec;
			return target_weight * fade_in_ratio;
		}
		else
		{
			// curr_playback_time_msec >= fade_out_start_time_msec

			// A non-looped animation is past the fade out time -> fade out.

			//const MillisecondsT playback_time_remaining_msec = total_anim_duration_msec - curr_playback_time_msec;
			MillisecondsT remaining_time_to_play_msec
				= anim_params.fade_out_start_time_msec + anim_params.fade_out_time_msec
				- curr_playback_time_msec
				;
			remaining_time_to_play_msec = largest(remaining_time_to_play_msec, 0);

			anim_is_stopped_ = (remaining_time_to_play_msec == 0);

			const float01_t fade_out_ratio = (float)remaining_time_to_play_msec / (float)anim_params.fade_out_time_msec;
			return target_weight * fade_out_ratio;
		}
	}
	else
	{
		// This a looping animation.

		anim_is_stopped_ = false;

		if( curr_playback_time_msec < anim_params.fade_in_time_msec
			&& !playback_ctrl.did_ever_wrap_around )
		{
			// A looped animation at the beginning -> fade in.
			const float01_t fade_in_ratio = (float)curr_playback_time_msec / (float)anim_params.fade_in_time_msec;
			return target_weight * fade_in_ratio;
		}
		else
		{
			return target_weight;
		}
	}
}

void NwAnimPlayer::UpdateOncePerFrame(
	const SecondsF delta_seconds
	, UpdateOutput &output_
	)
{
	mxASSERT(delta_seconds > 0);
	//_StartScheduledAnims();

//	DBGOUT("\n\n\tUpdateOncePerFrame()!");
	mxASSERT2(_active_anims.num() > 0, "Must always play at least one anim!");

	//
	output_.anims_count = 0;

	//
	const UINT max_num_anims_to_play = smallest(
		_active_anims.num(), MAX_SIMULTANEOUS_ANIMS
		);

	if(mxUNLIKELY( !max_num_anims_to_play )) {
		return;
	}

	//
	float		accum_total_weight = 0;
	U32		which_anims_stopped = 0;

	//
	for( UINT j = 0; j < max_num_anims_to_play; j++ ) {
		_active_anims._data[j].current_blend_weight = 0;
	}

	//
	UINT iActiveAnim = 0;
	while( iActiveAnim < max_num_anims_to_play )
	{
		const UINT iCurrAnim = iActiveAnim++;
		AnimState & anim_state = _active_anims._data[ iCurrAnim ];

#if nwDEBUG_ANIMS
		if( iCurrAnim > 0 )
		{
			mxASSERT2(_active_anims[ iCurrAnim-1 ].params.priority >= anim_state.params.priority,
				"Active anims must be sorted by decreasing priority!");
		}
#endif

		// Advance the local clock.
		anim_state.playback_controller.AdvanceClock(
			MS2SEC(anim_state.anim_duration_msec)
			, delta_seconds
			);

		//
		bool	anim_is_stopped = false;
		const float current_weight = _ComputeCurrentBlendWeight(
			anim_state
			, anim_is_stopped
			);

		//
		if( anim_is_stopped )
		{
#if nwDEBUG_ANIMS
			DBGOUT("Anim '%s' is stopped!", anim_state.anim->dbgName());
#endif
			which_anims_stopped |= BIT(iCurrAnim);

			anim_state.playback_controller.reset();
			anim_state.current_blend_weight = 0;
			continue;
		}

		//
#if 0 && MX_DEVELOPER
		if( max_num_anims_to_play > 1 )// && current_weight < 1 )
		{
			DEVOUT("[%d] Anim '%s': curr_weight = %f",
				iCurrAnim, anim_state.anim->dbgName(), current_weight);
		}
#endif

		// the previous layer has more "weight"
		const float01_t clamped_blend_weight = clampf(
			current_weight,
			0,
			1.0f - accum_total_weight
			);

		anim_state.current_blend_weight = clamped_blend_weight;

		accum_total_weight += clamped_blend_weight;

//#if nwDEBUG_ANIMS
//		if(
//			strstr(anim_state.anim->dbgName(), "rocketlauncher_view_lower")
//			||
//			strstr(anim_state.anim->dbgName(), "rocketlauncher_view_raise")
//			)
//		{
//			if(anim_state.playback_controller._time_ratio >= 0.99f) {
//				DEVOUT("Anim '%s' _time_ratio >= 0.99f!", anim_state.anim->dbgName());
//			}
//			else if(anim_state.playback_controller._time_ratio <= 0.01f) {
//				DEVOUT("Anim '%s' _time_ratio <= 0.01f!", anim_state.anim->dbgName());
//			}
//			else {
//				DEVOUT("Anim '%s': _time_ratio = %f!",
//					anim_state.anim->dbgName(), anim_state.playback_controller._time_ratio
//					);
//			}
//		}
//#endif

		//
		if( accum_total_weight >= 1.0f )
		{
			// skip lower priority layers...

//#if MX_DEVELOPER
//			if( iCurrAnim < max_num_anims_to_play - 1 )
//			{
//				DBGOUT("Skipping anims after '%s'!", active_anim.anim_clip->dbgName());
//			}
//#endif
			break;
		}

	}//for each anim to play


	//
	const UINT num_anims_to_play = iActiveAnim;
	mxASSERT2(num_anims_to_play > 0, "should always play at least one animation!");


	// allocate anim sample buffers
	for( UINT i = 0; i < num_anims_to_play; i++ )
	{
		AnimState & active_anim = _active_anims[ i ];

		const bool this_anim_stopped = which_anims_stopped & BIT(i);

		if( !this_anim_stopped )
		{
			// The weight can be zero when the anim just begins or finishes playing.
			mxASSERT(active_anim.current_blend_weight >= 0);

			if(active_anim.current_blend_weight > 0)
			{
				if(mxUNLIKELY(nil == active_anim.sampling_cache))
				{
					_AllocateSamplingCache(active_anim);
				}

				//
				const NwPlaybackController& playback_ctrl = active_anim.playback_controller;

				//
				const UINT dst_anim_index = output_.anims_count++;
				NwPlayingAnim &playing_anim = output_.playing_anims[ dst_anim_index ];
				//
				playing_anim.anim_clip = active_anim.anim;
				playing_anim.sampling_cache = active_anim.sampling_cache;
				playing_anim.blend_weight	= active_anim.current_blend_weight;
				playing_anim.prev_time_ratio = playback_ctrl._previous_time_ratio;
				playing_anim.curr_time_ratio = playback_ctrl._time_ratio;
			}

//#if MX_DEVELOPER
//			if( max_num_anims_to_play > 1 )
//			{
//				DEVOUT("[%d] Anim '%s' weight = %f",
//					dst_anim_index, playing_anim.anim_clip->dbgName(), playing_anim.target_weight);
//			}
//#endif
		}
		else
		{
			_ReleaseSamplingCache(active_anim);
		}
	}//For each anim.

	// remove stopped anims
	for( int i = _active_anims.num() - 1; i >= 0; i-- )
	{
		const bool this_anim_stopped = which_anims_stopped & BIT(i);

		if( this_anim_stopped )
		{
#if nwDEBUG_ANIMS
			const AnimState& active_anim = _active_anims[ i ];
			DBGOUT("Removing anim '%s'.", active_anim.anim->dbgName());
#endif
			_active_anims.RemoveAt(i);
		}
	}//For each anim.

	mxASSERT2(_active_anims.num() > 0, "should always play at least one animation!");
}

#if 0
/**
This method priority-inserts new anim jobs from the startedAnimJobs
array into the animJobs array and sets the base time of the anim job so
that it is properly synchronized with the current time. This method
is called from Evaluate().
*/
void NwAnimPlayer::_StartScheduledAnims(const MillisecondsT current_time)
{
	UNDONE;

	nwFOR_EACH(const NwPlayAnimParams& scheduled_anim, _scheduled_anims)
	{
		ActiveAnim	new_anim;
		new_anim.anim			= scheduled_anim.anim;
		new_anim.priority		= scheduled_anim.priority;
		new_anim.anim_duration_sec	= scheduled_anim.anim->ozz_animation.duration();

		new_anim.playback_controller.reset();

		//new_anim.time_when_started	= current_time;
		//new_anim.current_weight		= 0;
		new_anim.target_weight		= scheduled_anim.blend_weight;

		const UINT dst_index = _FindDstIndexForInsertingAnim(scheduled_anim.priority);
		_active_anims.Insert(new_anim, dst_index);
	}

	// clear the started anim jobs array for the next "frame"
    _scheduled_anims.RemoveAll();

}
#endif

ERet NwAnimPlayer::_AllocateSamplingCache(
	AnimState & playing_anim
	)
{
	mxASSERT(nil == playing_anim.sampling_cache);
	mxOPTIMIZE("num bones")
	playing_anim.sampling_cache = NwAnimationSystem::Get().AllocateSamplingCache(
		nwRENDERER_MAX_BONES
		);
	return ALL_OK;
}

void NwAnimPlayer::_ReleaseSamplingCache(
	AnimState & playing_anim
	)
{
	mxASSERT_PTR(playing_anim.sampling_cache);
	NwAnimationSystem::Get().ReleaseSamplingCache(playing_anim.sampling_cache);
	playing_anim.sampling_cache = nil;
}



#if 0
/**
* @function
* @name pc.AnimationComponent#play
* @description Start playing an animation
* @param {Animation} newAnim The name of the animation asset to begin playing.
* @param {Number} [transition_duration] The time in seconds to blend from the current
* animation state to the start of the animation being set.
*/
void NwAnimPlayer::PlayAnim_Exclusive(
	const NwAnimClip* new_animation
	, F32 transition_duration /*= TB_DEFAULT_ANIMATION_BLEND_TIME*/
	, F32 target_blend_weight /*= 1.0f*/
	, const PlayAnimFlags::Raw flags
	)
{
	mxASSERT_PTR(new_animation);

IF_DEBUG bool was_already_playing_this_anim = this->IsPlayingAnim(new_animation);
//if(was_already_playing_this_anim) {
//	return;
//}

	int added_anim_index = this->PlayAnim_Additive(
		new_animation
		, transition_duration
		, target_blend_weight
		, flags
		);

	if (!was_already_playing_this_anim)
	{
DBGOUT("PlayAnimExclusive: '%s', idx = %d",
	   new_animation->dbgName(), added_anim_index
	   );
	}

	// fade out other animations
	this->FadeOthers(
		added_anim_index
		, transition_duration
		, 0 // target_blend_weight
		);
}

int NwAnimPlayer::PlayAnim_Additive(
	const NwAnimClip* new_animation
	, F32 transition_duration /*= TB_DEFAULT_ANIMATION_BLEND_TIME*/
	, F32 target_blend_weight /*= 1.0f*/
	, const PlayAnimFlags::Raw flags
	)
{
	mxASSERT_PTR(new_animation);
UNDONE;
return 0;
}

void NwAnimPlayer::FadeOut(
	F32 transition_duration /*= TB_DEFAULT_ANIMATION_BLEND_TIME*/
	)
{
	UNDONE;
	//for( int i = 0; i < num_playing_anims; i++ )
	//{
	//	AnimState & anim_state = *_active_anims[ i ];

	//	anim_state.target_weight = 0;
	//	anim_state.fade_transition_duration = transition_duration;
	//}
}

void NwAnimPlayer::FadeOthers(
	const int this_anim_index
	, F32 transition_duration
	, F32 target_blend_weight
	)
{
	//for( int i = 0; i < num_playing_anims; i++ )
	//{
	//	if( this_anim_index != i )
	//	{
	//		AnimState & anim_state = *_active_anims[ i ];

	//		anim_state.target_weight = 0;
	//		anim_state.fade_transition_duration = transition_duration;
	//	}
	//}
}
#endif
namespace AnimJobs
{

static
void UpdateModelJointMatrices(
						 NwAnimatedModel & anim_model
						 /// Buffer of model space matrices.
						 , const M44f* __restrict models
						 )
{
	mxASSERT(IS_16_BYTE_ALIGNED(models));

	NwSkinnedMesh *	skinned_model	= anim_model._skinned_mesh;
	idRenderModel *	render_model	= anim_model.render_model;

	const UINT num_joints = render_model->joint_matrices.num();
	mxASSERT(num_joints == skinned_model->ozz_skeleton.num_joints());

	// object-space joint transforms
	M44f * __restrict	joint_matrices = render_model->joint_matrices.raw();
	mxASSERT(IS_16_BYTE_ALIGNED(joint_matrices));

	//
	const M44f* __restrict	inverse_bind_pose = skinned_model->inverse_bind_pose_matrices.raw();
	mxASSERT(IS_16_BYTE_ALIGNED(inverse_bind_pose));

	//
	for( UINT i = 0; i < num_joints; i++ )
	{
		// first transform by the inverse bind pose matrix, then by the bone/joint matrix
		const M44f joint_matrix = inverse_bind_pose[i] * models[i];

		// transpose before sending to the GPU, because the shader post-multiplies vertices by row-major matrices
		joint_matrices[i] = M44_Transpose( joint_matrix );
	}
}


ERet ComputeJointMatricesJob::Run( const NwThreadContext& context, int start, int end )
{
	AllocatorI &	local_scratchpad = *context.heap;	// thread-local allocator
	AllocatorI &	global_scratchpad = MemoryHeaps::jobs();	// global thread-safe allocator

	const int num_playing_anims = anim_update_output.anims_count;
	mxASSERT(num_playing_anims > 0);

	//
	const ozz::animation::Skeleton&	ozz_skeleton = anim_model._skinned_mesh->ozz_skeleton;

	const int num_joints		= ozz_skeleton.num_joints();
	const int num_soa_joints	= ozz_skeleton.num_soa_joints();

	//
	enum Constants
	{
		MAX_BLENDED_ANIMS = mxCOUNT_OF(anim_update_output.playing_anims),
		SSE2_ALIGNMENT = 16,
		ROOT_JOINT = 0
	};
	
	TScopedPtr< ozz::math::SoaTransform >	locals_per_each_anim[ MAX_BLENDED_ANIMS ];
	for(UINT i = 0; i < mxCOUNT_OF(locals_per_each_anim); i++ ) {
		new(&locals_per_each_anim[i]) TScopedPtr< ozz::math::SoaTransform >(local_scratchpad);
	}

	// Prepares blending layers.
	ozz::animation::BlendingJob::Layer	ozz_blend_layers[MAX_BLENDED_ANIMS];

	//
	for( int i = 0; i < num_playing_anims; i++ )
	{
		NwPlayingAnim& playing_anim = anim_update_output.playing_anims[ i ];
		mxASSERT(playing_anim.blend_weight > 0);

		// Buffer of local transforms as sampled from animation.
		ozz::math::SoaTransform *	locals;
		mxDO(nwAllocArray( locals, num_soa_joints, local_scratchpad, SSE2_ALIGNMENT ));

		new(&locals_per_each_anim[ i ]) TScopedPtr< ozz::math::SoaTransform >(
			local_scratchpad, locals
			);

		ozz::Range< ozz::math::SoaTransform >	locals_ozz_range(
			locals,
			locals + num_soa_joints
			);

		//
		ozz::animation::SamplingJob	ozz_sampling_job;
		ozz_sampling_job.animation	= &playing_anim.anim_clip->ozz_animation;
		ozz_sampling_job.cache		= &playing_anim.sampling_cache->ozz_sampling_cache;
		ozz_sampling_job.ratio		= playing_anim.curr_time_ratio;
		ozz_sampling_job.output		= locals_ozz_range;

		mxENSURE(ozz_sampling_job.Run(), ERR_UNKNOWN_ERROR, "");

		//
		ozz_blend_layers[i].transform	= locals_ozz_range;
		ozz_blend_layers[i].weight		= playing_anim.blend_weight;
	}//


	//
	const bool should_use_blending = num_playing_anims > 1;

	/// Buffer of local transforms which stores the blending result.
	TScopedPtr< ozz::math::SoaTransform >	blended_locals( local_scratchpad );
	mxDO(nwAllocArray( blended_locals.ptr, num_soa_joints, blended_locals.allocator, SSE2_ALIGNMENT ));

	//
	if( !should_use_blending )
	{
		// Fast path - no blending needed.
	}
	else
	{
		//
		// Blends animations.
		// Blends the local spaces transforms computed by sampling all animations
		// (1st stage just above), and outputs the result to the local space
		// transform buffer blended_locals_

		// Setups blending job.
		ozz::animation::BlendingJob ozz_blending_job;
		ozz_blending_job.threshold = 0.1f;
		ozz_blending_job.layers = ozz::Range< const ozz::animation::BlendingJob::Layer >(
			ozz_blend_layers,
			num_playing_anims
			);
		ozz_blending_job.bind_pose = ozz_skeleton.joint_bind_poses();
		ozz_blending_job.output = ozz::Range< ozz::math::SoaTransform >(
			blended_locals.ptr,
			blended_locals.ptr + num_soa_joints
			);

		// Blends.
		mxENSURE(ozz_blending_job.Run(), ERR_UNKNOWN_ERROR, "");
	}


	/// Buffer of model space matrices. These are computed by the local-to-model
	/// job after the blending stage.
	TScopedPtr< ozz::math::Float4x4 >	models( local_scratchpad );
	mxDO(nwAllocArray( models.ptr, num_joints, models.allocator, SSE2_ALIGNMENT ));


	// Converts from local space to model space matrices.
	ozz::animation::LocalToModelJob	ozz_local_to_model_job;
	{
		ozz_local_to_model_job.skeleton = &ozz_skeleton;

		if( !should_use_blending )
		{
			ozz::math::SoaTransform *	locals = locals_per_each_anim[0].ptr;
			ozz_local_to_model_job.input = ozz::Range< ozz::math::SoaTransform >(
				locals,
				locals + num_soa_joints
				);
		}
		else
		{
			ozz_local_to_model_job.input = ozz::Range< ozz::math::SoaTransform >(
				blended_locals.ptr,
				blended_locals.ptr + num_soa_joints
			);
		}

		ozz_local_to_model_job.output = ozz::Range< ozz::math::Float4x4 >(
			models.ptr,
			models.ptr + num_joints
			);
	}
	mxENSURE(ozz_local_to_model_job.Run(), ERR_UNKNOWN_ERROR, "");





	//
	const int barrel_joint_index = anim_model.barrel_joint_index;
	if( anim_model.barrel_joint_index != INDEX_NONE )
	{
		anim_model.job_result.barrel_joint_mat_local = (M44f&) models.ptr[ barrel_joint_index ];
	}



	// root motion

	//anim_model.job_result.root_joint_offset
	//	= (V3f&) models.ptr[ 0 ].cols[3]// * anim_update_output.playing_anims[0].blend_weight
	//	;

	//
	V3f	blended_root_joint_pos_in_model_space = CV3f(0);

	for( int iAnim = 0; iAnim < num_playing_anims; iAnim++ )
	{
		const NwPlayingAnim& playing_anim = anim_update_output.playing_anims[ iAnim ];

		const ozz::math::SoaFloat3& soa_trans = locals_per_each_anim[ iAnim ].ptr[ROOT_JOINT].translation;

		const V3f root_joint_pos_in_model_space = CV3f(
			soa_trans.x.m128_f32[0],
			soa_trans.y.m128_f32[0],
			soa_trans.z.m128_f32[0]
		);

		blended_root_joint_pos_in_model_space
			+= root_joint_pos_in_model_space * playing_anim.blend_weight
			;
	}

	anim_model.job_result.blended_root_joint_pos_in_model_space
		= blended_root_joint_pos_in_model_space
		;

	//NOTE: fires if "attack1" is playing on trite:
	//mxASSERT(
	//	(blended_root_joint_pos_in_model_space - anim_model.job_result.root_joint_offset)
	//	.lengthSquared() < 1e-4f
	//	);


	//
	UpdateModelJointMatrices(
		anim_model,
		ozzMatricesToM44f( models.ptr )
		);

	return ALL_OK;
}

}//namespace AnimJobs

}//namespace Rendering
