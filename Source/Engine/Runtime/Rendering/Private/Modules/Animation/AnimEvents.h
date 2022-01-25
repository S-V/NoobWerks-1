#pragma once


namespace Rendering
{
struct NwPlayingAnim;

///
enum EAnimEventType
{
	FC_SOUND,
	FC_SOUND_VOICE,	// sounds made by monsters, like breathing
	FC_SOUND_VOICE2,
	FC_SOUND_BODY,
	FC_SOUND_BODY2,
	FC_SOUND_BODY3,

	FC_SOUND_WEAPON,

	//
	FC_MELEE,
	FC_DIRECT_DAMAGE,
	FC_BEGIN_ATTACK,
	FC_END_ATTACK,

	FC_MUZZLE_FLASH,
	FC_CREATE_MISSILE,
	FC_LAUNCH_MISSILE,
	FC_FIREMISSILEATTARGET,
};
mxDECLARE_ENUM( EAnimEventType, U32, AnimEventTypeT );


struct NwAnimEventParameter: CStruct
{
	AssetID		name;	// e.g. "bigboom" (sound resource id) or "snd_explode" (dict key)
	NameHash32	hash;
public:
	mxDECLARE_CLASS(NwAnimEventParameter,CStruct);
	mxDECLARE_REFLECTION;
};

/// Animation events are triggered when the "play cursor" passes over them.
/// AnimEvents are essential for synchronizing all types of stuff with an animation
/// (for instance, a walking animation should trigger events
/// when a foot touches the ground, so that footstep sounds and dust particles
/// can be created at the right time and position,
/// and finally events are useful for synchronizing the start of a new animation
/// with a currently playing animation
/// (for instance, start the "turn left" animation clip when the current animation
/// has the left foot on the ground, etc...).
///
struct NwAnimEvent: CStruct
{
	AnimEventTypeT			type;
	NwAnimEventParameter	parameter;
public:
	mxDECLARE_CLASS(NwAnimEvent,CStruct);
	mxDECLARE_REFLECTION;
};

// normalized time, relative the animation's duration, quantized to 16 bits;
// pros: integers are faster to compare and take less memory than floats;
// cons: limited precision - events can be called twice if ticks are too small.
typedef U16 AnimEventTimeT;

/// 
class NwAnimEventList: CStruct
{
public:
	struct FrameLookup: CStruct
	{
		AnimEventTimeT	time;
	public:
		mxDECLARE_CLASS( FrameLookup, CStruct );
		mxDECLARE_REFLECTION;
	};

	TArray< FrameLookup >	_frame_lookup;
	TArray< NwAnimEvent >	_frame_events;

public:
	mxDECLARE_CLASS(NwAnimEventList,CStruct);
	mxDECLARE_REFLECTION;

	NwAnimEventList();

	typedef void AnimEventCallback(
		const NwAnimEvent& anim_event
		, void * user_data
		);

	void ProcessEventsWithCallback(
		UINT start_time	// AnimEventTimeT
		, UINT end_time	// AnimEventTimeT
		, AnimEventCallback* callback
		, void * user_data
		) const;
};

namespace NwAnimEventList_
{
	void ProcessAnimEventsWithCallback(
		const NwPlayingAnim* playing_animations
		, const int num_playing_animations
		, NwAnimEventList::AnimEventCallback* callback
		, void * user_data
		);
}//namespace

}//namespace Rendering
