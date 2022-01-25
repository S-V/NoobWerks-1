//
#include "stdafx.h"
#pragma hdrstop

#include <stdint.h>

#include <Core/Util/Tweakable.h>
#include <Engine/Engine.h>	// NwTime
#include <Renderer/Core/MeshInstance.h>
#include <Renderer/Modules/Animation/AnimatedModel.h>

#include "FPSGame.h"
#include "game_player.h"
#include "game_settings.h"
#include "experimental/game_experimental.h"
#include "game_world/game_world.h"
#include "game_animation/game_animation.h"



ERet MyGameWorld::_InitNPCs(
							const MyGameSettings& game_settings
							, NwClump* scene_clump
							)
{
	mxDO(NPCs.Init(
		scene_clump
		));

	if(0)//+
	{
		NwAnimatedModel *	new_anim_model;
		mxDO(NwAnimatedModel_::Create(
			new_anim_model
			, *_scene_clump
			, MakeAssetID(
			//"monster_demon_trite"
			//"weapon_rocketlauncher"
			"monster_zombie_commando_cgun"
			)
			
			, &MyGame::ProcessAnimEventCallback
			));

		NwAnimatedModel_::AddToWorld(
			new_anim_model
			, render_world
			);

		new_anim_model->PlayAnim(
			mxHASH_STR("idle")
			, NwPlayAnimParams().SetPriority(NwAnimPriority::Lowest)
			, PlayAnimFlags::Looped
			);

		//test
		new_anim_model->PlayAnim(
			mxHASH_STR(
				//"range_attack_loop1"
				"reload"
			)
			, NwPlayAnimParams().SetPriority(NwAnimPriority::Required)
			, PlayAnimFlags::Looped
			);
	}


	if(0)
	{
		NwAnimatedModel *	new_anim_model;
		mxDO(NwAnimatedModel_::Create(
			new_anim_model
			, *_scene_clump
			, MakeAssetID(
				"monster_zombie_commando_cgun"
				//"monster_demon_pinky"
			)
			, &MyGame::ProcessAnimEventCallback
			));

		new_anim_model->PlayAnim(mxHASH_STR("idle"));
	}



	if(0)//+
	{
		NwAnimatedModel *	new_anim_model;
		mxDO(NwAnimatedModel_::Create(
			new_anim_model
			, *_scene_clump
			, MakeAssetID(
			//"monster_demon_imp"//+
			//"monster_demon_archvile"//+
			//"monster_demon_revenant"//+, but body is not transparent
			//"monster_demon_wraith"//+
			"monster_demon_mancubus"//+
			//"monster_boss_sabaoth"//~+ tank
			//"monster_demon_hellknight"//~+ saliva not transparent
			//"monster_demon_cherub"//~~ body is transparent, but wings are opaque
			//"monster_boss_cyberdemon"//~~ body is transparent
			//"monster_boss_maledict"//+
			)
			
			, &MyGame::ProcessAnimEventCallback
			));

		NwAnimatedModel_::AddToWorld(
			new_anim_model
			, render_world
			);

		new_anim_model->PlayAnim(
			mxHASH_STR("idle")
			, NwPlayAnimParams().SetPriority(NwAnimPriority::Lowest)
			, PlayAnimFlags::Looped
			);

		////test
		//new_anim_model->PlayAnim(
		//	mxHASH_STR(
		//		"range_attack_loop"
		//	)
		//	, NwPlayAnimParams().SetPriority(NwAnimPriority::Required)
		//	, PlayAnimFlags::Looped
		//	);
	}

	//
#if 0
	NPCs.Spawn_Spider(
		*this
		, CV3f(3,2
			, 10 //
		)
		);
#endif
	//
#if 0
	EnemyNPC* new_npc;
	NPCs.Spawn_Imp(
		new_npc
		, *this
		, CV3f(-6, -6, 20)
		);
#endif

	//
#if 0	// perf test
	for(int x = 0; x < 5; x++ )
	{
		for(int y = 0; y < 5; y++ )
		{
			NPCs.Spawn_Spider(
				*this
				, CV3f(x*3, y*3, 0) - CV3f(6,6,0)
				);
		}
	}
#endif

	return ALL_OK;
}

void MyGameWorld::_DeinitNPCs()
{
	NPCs.Deinit(*this);
}
