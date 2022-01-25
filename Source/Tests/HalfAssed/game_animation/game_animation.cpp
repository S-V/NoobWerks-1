#include "stdafx.h"
#pragma hdrstop

#include <Renderer/Modules/idTech4/idEntity.h>

#include "FPSGame.h"
#include "game_settings.h"
#include "game_world/game_world.h"
#include "game_animation/game_animation.h"
#include "npc/npc_behavior_common.h"


namespace MyGame
{

static void DealDamageToPlayerIfInMeleeRange(
	const NwAnimEventParameter& anim_event_parameter
	, const NwAnimatedModel& anim_model
	)
{
	EnemyNPC& npc = *anim_model.npc;

	MyGamePlayer& player = FPSGame::Get().player;

	if(AI_Fun::_Check_IsPointInFrontOfNPC(
		npc
		, player.GetEyePosition()
		, 0.4f // 66 deg
		))
	{
		int damage = npc.behavior->GetDamageForDealingToPlayer(
			anim_event_parameter
			);

		idRandom	rnd(mxGetTimeInMilliseconds());
		damage += rnd.RandomInt(8);

		player.DealDamage(damage);
	}
}

#if GAME_CFG_WITH_SOUND

	static
	void PlaySound3D(
					 const NwAnimEventParameter& anim_event_parameter
					 , const V3f& position_in_world_space
					 , NwSoundSystemI& sound_system
					 , const idEntityDef* entity_def
					 )
	{
		if(entity_def)
		{
			const TResPtr<NwSoundSource>* sound_res = entity_def->sounds.find(anim_event_parameter.hash);
			mxASSERT(sound_res);
			if(sound_res)
			{
				sound_system.PlaySound3D(
					sound_res->_id
					, position_in_world_space
					, nil // velocity
					);
			}
		}
		else
		{
			const AssetID& sound_shader_id = anim_event_parameter.name;

			sound_system.PlaySound3D(
				sound_shader_id
				, position_in_world_space
				, nil // velocity
				);
		}
	}

#endif // GAME_CFG_WITH_SOUND



void ProcessAnimEventCallback(
	const NwAnimEvent& anim_event
	, void * user_data
	)
{
	const NwAnimatedModel& anim_model = *(NwAnimatedModel*) user_data;

	FPSGame& game = FPSGame::Get();


	//
#if GAME_CFG_WITH_SOUND

	NwSoundSystemI& sound_system = game.sound;

	// Position in 3D space of the channel.
	const idRenderModel& render_model = *anim_model.render_model;
	const V3f&	position = render_model.local_to_world.translation();

	// Velocity in 'distance units per second' in 3D space of the channel.
	//const V3f	velocity = CV3f(0);

#endif // GAME_CFG_WITH_SOUND


	//
	switch( anim_event.type )
	{

#pragma region Sounds

	case FC_SOUND:
	case FC_SOUND_VOICE:
	case FC_SOUND_VOICE2:
	case FC_SOUND_BODY:
	case FC_SOUND_BODY2:
	case FC_SOUND_BODY3:
#if GAME_CFG_WITH_SOUND
		PlaySound3D(
			anim_event.parameter
			, position
			, sound_system
			, anim_model.entity_def._ptr
			);
#endif // GAME_CFG_WITH_SOUND
		break;

	case FC_SOUND_WEAPON:
#if GAME_CFG_WITH_SOUND
		PlaySound3D(
			anim_event.parameter
			, position
			, sound_system
			, anim_model.entity_def._ptr
			);
#endif // GAME_CFG_WITH_SOUND
		break;

#pragma endregion


#pragma region Attacking

	case FC_MELEE:
		DealDamageToPlayerIfInMeleeRange(
			anim_event.parameter
			, anim_model
			);
		break;


	case FC_LAUNCH_MISSILE:

#if GAME_CFG_WITH_SOUND
		PlaySound3D(
			anim_event.parameter
			, position
			, sound_system
			, anim_model.entity_def._ptr
			);
#endif // GAME_CFG_WITH_SOUND

		DBGOUT("FC_LAUNCH_MISSILE");

		//sound_engine.PlaySound3D(AssetID());
//		if( 0==strcmp(AssetId_ToChars(anim_event.parameter),"monster_zombie_commando_breath_inhale") )
		//{
		//	ptWARN("dt = %.3f, frame = %llu"
		//		, params.delta_seconds, params.frameNumber
		//		);
		//	params.sound_engine->PlaySound3D(AssetID());
		//}
		break;

#pragma endregion

	mxNO_SWITCH_DEFAULT;
	}
}

}//namespace
