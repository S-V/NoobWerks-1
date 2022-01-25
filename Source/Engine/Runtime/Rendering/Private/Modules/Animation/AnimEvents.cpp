#include <Base/Base.h>
#pragma hdrstop

#include <Core/Client.h>	// NwTime
#include <Rendering/Private/Modules/Animation/AnimatedModel.h>
#include <Rendering/Private/Modules/Animation/AnimEvents.h>

namespace Rendering
{
mxBEGIN_REFLECT_ENUM( AnimEventTypeT )
	mxREFLECT_ENUM_ITEM( SOUND, FC_SOUND ),
	mxREFLECT_ENUM_ITEM( SOUND_VOICE, FC_SOUND_VOICE ),
	mxREFLECT_ENUM_ITEM( SOUND_VOICE2, FC_SOUND_VOICE2 ),
	mxREFLECT_ENUM_ITEM( SOUND_BODY, FC_SOUND_BODY ),
	mxREFLECT_ENUM_ITEM( SOUND_BODY2, FC_SOUND_BODY2 ),
	mxREFLECT_ENUM_ITEM( SOUND_BODY3, FC_SOUND_BODY3 ),
	mxREFLECT_ENUM_ITEM( SOUND_WEAPON, FC_SOUND_WEAPON ),
	mxREFLECT_ENUM_ITEM( LAUNCH_MISSILE, FC_LAUNCH_MISSILE ),
mxEND_REFLECT_ENUM

mxDEFINE_CLASS(NwAnimEventParameter);
mxBEGIN_REFLECTION(NwAnimEventParameter)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(hash),
mxEND_REFLECTION;


mxDEFINE_CLASS(NwAnimEvent);
mxBEGIN_REFLECTION(NwAnimEvent)
	mxMEMBER_FIELD(type),
	mxMEMBER_FIELD(parameter),
mxEND_REFLECTION;


mxDEFINE_CLASS(NwAnimEventList::FrameLookup);
mxBEGIN_REFLECTION(NwAnimEventList::FrameLookup)
	mxMEMBER_FIELD(time),
mxEND_REFLECTION;

mxDEFINE_CLASS(NwAnimEventList);
mxBEGIN_REFLECTION(NwAnimEventList)
	mxMEMBER_FIELD(_frame_lookup),
	mxMEMBER_FIELD(_frame_events),
mxEND_REFLECTION;

NwAnimEventList::NwAnimEventList()
{
}

void NwAnimEventList::ProcessEventsWithCallback(
				   UINT start_time
				   , UINT end_time
				   , AnimEventCallback* callback
				   , void * user_data
				   ) const
{
	const U32 num_events = _frame_lookup.num();
	const FrameLookup* frame_lookup = _frame_lookup.raw();

	const NwAnimEvent* frame_events = _frame_events.raw();

	//
	U32	i;

	// Locate the start of the event run.
	for( i = 0;
		( i < num_events ) && ( frame_lookup[i].time < start_time );
		i++ )
		;

	//
	if( end_time < start_time )
	{
		// Process all events to the end of the list and wrap
		for( ;
			i < num_events;
			i++ )
		{
			callback( frame_events[i], user_data );
		}

		i = 0;		// Restart from the beginning of the list.
	}

	// Process all the events up to the endTime (from the start of the list)
	for( ;
		( i < num_events ) && ( frame_lookup[i].time <= end_time );
		i++ )
	{
		callback( frame_events[i], user_data );
	}
}


namespace NwAnimEventList_
{
	void ProcessAnimEventsWithCallback(
		const NwPlayingAnim* playing_animations
		, const int num_playing_animations
		, NwAnimEventList::AnimEventCallback* callback
		, void * user_data
		)
	{
		for( UINT i = 0; i < num_playing_animations; i++ )
		{
			const NwPlayingAnim& playing_anim = playing_animations[ i ];

			const U32 start_frame = mmFloorToInt( playing_anim.prev_time_ratio * 65535.0f );
			const U32 end_frame = mmFloorToInt( playing_anim.curr_time_ratio * 65535.0f );

			playing_anim.anim_clip->events.ProcessEventsWithCallback(
				start_frame, end_frame
				, callback
				, user_data
				);
		}
	}
}//namespace

}//namespace Rendering
