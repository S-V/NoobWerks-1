#include "stdafx.h"
#pragma hdrstop

//
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/RuntimeInclude.h>
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/IObject.h>
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/ObjectInterfacePerModule.h>
RUNTIME_MODIFIABLE_INCLUDE; //recompile runtime files when this changes

RUNTIME_COMPILER_LINKLIBRARY("user32.lib");
RUNTIME_COMPILER_LINKLIBRARY("advapi32.lib");
RUNTIME_COMPILER_LINKLIBRARY("kernel32.lib");
RUNTIME_COMPILER_LINKLIBRARY("gdi32.lib");
RUNTIME_COMPILER_LINKLIBRARY("winmm.lib");
RUNTIME_COMPILER_LINKLIBRARY("Dbghelp.lib");
RUNTIME_COMPILER_LINKLIBRARY("Imm32.lib");
RUNTIME_COMPILER_LINKLIBRARY("comctl32.lib");
/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "Base.lib");
//
/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "Core.lib");
//
/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "Graphics.lib");
//LINK : fatal error LNK1104: cannot open file 'DxErr.lib'
//
/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "Renderer.lib");

/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "bx.lib");
/*_._*/RUNTIME_COMPILER_LINKLIBRARY( "ozz-animation.lib");


//
#include <Base/Base.h>
#include <Base/Math/Random.h>
//
#include <Core/Tasking/TinyCoroutines.h>
#include <Core/Util/Tweakable.h>
//
#include <Renderer/Renderer.h>
#include <Renderer/Modules/Animation/SkinnedMesh.h>
#include <Renderer/Modules/idTech4/idMaterial.h>
//
#include <Utility/GameFramework/TbPlayerWeapon.h>

//
//#include "RCCpp_WeaponBehavior.h"
#include "experimental/rccpp.h"
#include "experimental/game_experimental.h"
#include "weapons/game_player_weapons.h"
#include "npc/npc_behavior_common.h"
#include "gameplay_constants.h"
#include "FPSGame.h"


namespace
{
enum
{
	MAGGOT_HEALTH = 4 * WeaponStats::PLASMAGUN_DAMAGE,

	MAGGOT_MELEE_ATTACK_DAMAGE = 30,
	//MAGGOT_LEAP_ATTACK_DAMAGE = 30,
};


struct Task_GreetPlayer: CoTask
{
public:
	CoTaskStatus Run( EnemyNPC & npc, const CoTimestamp& now )
	{
		$CoBEGIN;

		DBGOUT("Task_GreetPlayer!");

		//
		AI_Fun::PlayOneShotAnimIfNeeded(
			mxHASH_STR("sight")
			, npc
			);

		$CoWAIT_WHILE(npc.anim_model->IsPlayingAnim(
			mxHASH_STR("sight")
			));

		$CoEND;
	}
};

}//namespace


///
struct NPC_Behavior_Maggot
	: NPC_BehaviorI
	, TInterface<IID_IRCCPP_MAIN_LOOP,IObject>
	, TSingleInstance<NPC_Behavior_Maggot>
{
	typedef AI_Brain_Generic_Melee_NPC	AI_Brain_Maggot_t;

public:
    NPC_Behavior_Maggot()
    {
		PerModuleInterface::g_pSystemTable->npc_behaviors[
			MONSTER_MAGGOT
		] = this;
    }

	virtual ERet CreateDataForNPC(
		EnemyNPC & npc
		) override
	{
		FPSGame& game = FPSGame::Get();

		//
		npc.comp_health._health = MAGGOT_HEALTH;

		//
		AI_Brain_Maggot_t *	new_ai_data;
		mxDO(game.runtime_clump.New(new_ai_data));

		new_ai_data->my_health = npc.comp_health._health;

		new_ai_data->config.run_anim_id = mxHASH_STR("run");

		//
		npc.ai_brain = new_ai_data;

		return ALL_OK;
	}

	virtual void DeleteDataForNPC(
		EnemyNPC & npc
		) override
	{
		FPSGame& game = FPSGame::Get();

		if(AI_Brain_Maggot_t* ai_brain = (AI_Brain_Maggot_t*)npc.ai_brain._ptr)
		{
			game.runtime_clump.delete_(ai_brain);
		}
	}

	virtual void Think(
		EnemyNPC & npc
		, AI_Blackboard & blackboard
		, const NwTime& game_time
		, const CoTimestamp now
		) override
	{
		FPSGame& game = FPSGame::Get();

		//
		AI_Brain_Maggot_t& ai_brain = (AI_Brain_Maggot_t&) *npc.ai_brain;

		//
		//
		AI_Senses	ai_senses;
		ai_senses.Setup(
			npc
			, blackboard
			, game.world.physics_world
			);

		//
		ai_senses.did_receive_damage = npc.comp_health._health < ai_brain.my_health;
		ai_brain.my_health = npc.comp_health._health;

		//
		ai_brain.Think(
			npc
			, ai_senses
			, blackboard
			, game_time
			, now
			);
	}

	void _LookForPlayer(
		AI_Blackboard & blackboard
		)
	{
		//
	}

	virtual void InflictDamage(
		EnemyNPC & npc
		, int damage
		, const V3f& pos_WS
		) override
	{
		DBGOUT("InflictDamage: %d", damage);

		//
		FPSGame& game = FPSGame::Get();

		//
		AI_Brain_Maggot_t& ai_brain = (AI_Brain_Maggot_t&) *npc.ai_brain;
		//ai_brain.ai_state = NPC_AI_ALERTED;

		//
		npc.comp_health.DealDamage(damage);
		
		if( npc.comp_health.IsAlive() )
		{
			//
			AI_Fun::PlayOneShotAnimIfNeeded(
				mxHASH_STR("pain")
				, npc
				);

			// alert
			if(ai_brain.ai_state == NPC_AI_IDLE)
			{
				ai_brain.ai_state = NPC_AI_ALERTED;
			}
		}
		else
		{
			// dead

			//
			nwTWEAKABLE_CONST( int, MAGGOT_GIB_ITEMS_TO_SPAWN, 8 );
			nwTWEAKABLE_CONST( float, MAGGOT_GIB_ITEM_VELOCITY, 6 );

			game.world.gibs.SpawnGibs(
				pos_WS
				, npc.rigid_body->GetCenterWorld()
				, npc.rigid_body->GetAabbWorld()
				, 0.2f // scale
				, MAGGOT_GIB_ITEMS_TO_SPAWN
				, MAGGOT_GIB_ITEM_VELOCITY
				);

			game.world.NPCs.KillNPC(npc);
		}
	}

	virtual int GetDamageForDealingToPlayer(
		const NwAnimEventParameter& anim_event_parameter
		) override
	{
		return MAGGOT_MELEE_ATTACK_DAMAGE;
	}
};

REGISTERSINGLETON(NPC_Behavior_Maggot,true);
