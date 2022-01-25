#pragma once

#include <Renderer/Modules/Animation/SkinnedMesh.h>	// NwAnimEvent


namespace MyGame
{
#if 0
	///
	struct NwAnimEventParams
	{
		//
		NwSoundSystemI *	sound_engine;

		// Position in 3D space of the channel.
		V3f		position;

		// Velocity in 'distance units per second' in 3D space of the channel.
		V3f		velocity;

		void *	user_data;
	};
#endif
	void ProcessAnimEventCallback(
		const NwAnimEvent& anim_event
		, void * user_data
		//, const NwAnimEventParams& parameters
		);

}//namespace
