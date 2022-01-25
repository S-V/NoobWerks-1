#include "stdafx.h"
#pragma hdrstop

#include <gainput/gainput.h>
#include <Engine/WindowsDriver.h>
#include "game_experimental.h"




#if TEST_LOAD_RED_FACTION_MODEL

#include <Developer/Mesh/MeshImporter.h>	// TcModel

#include <Renderer/Modules/Animation/SkinnedMesh.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_ModelLoader.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_AnimationLoader.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Experimental.h>




ERet loadTestV3C(
				 TestSkinnedModel &RF_test_model_
				 , NwClump * storage_clump
				 , AllocatorI & scratch
				 )
{
	TcModel		tc_model( scratch );

	mxDO(RF1::load_V3D_from_file(
		&tc_model
		,
		//destroyed meddesk:
		//"D:/dev/RF_PC/orig_data/meshes/meddesk_corpse.v3m"	//+ with +16 fix
		"D:/dev/RF_PC/orig_data/meshes/Fighter01.v3m"	//+ "pepelats"


		// the large spaceship in which the miners tried to escape
		//"D:/dev/RF_PC/orig_data/meshes/transport01.v3c"	//+

		// the rockets that brought down the escaping ship
		//"D:/dev/RF_PC/orig_data/meshes/trnsprt_rocket.v3c"	//+


		// turret attached to ceiling
		//"D:/dev/RF_PC/orig_data/meshes/auto_turret.v3c"	//+


		// Robots:
		// small flying robot
		//"D:/dev/RF_PC/orig_data/meshes/mft2.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/combot.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/Tankbot.v3c"	//+ big black scary robot
		//"D:/dev/RF_PC/orig_data/meshes/cutter01.v3c"	//+ big working robot

		___ // Animals:

		___ // Bat
		//"D:/dev/RF_PC/orig_data/meshes/bat1.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/fish.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/sea_creature.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/baby_reeper.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/reeper.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/mutant1.v3c"	//+ skeleton is wrong
		//"D:/dev/RF_PC/orig_data/meshes/rock_snake.v3c"	//+ skeleton slightly offset


		// Characters:
		//"D:/dev/RF_PC/orig_data/meshes/tech01.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/admin_fem.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/admin_male.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/nurse1.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/medic01.v3c"	//+ skeleton is wrong

		//"D:/dev/RF_PC/orig_data/meshes/non_env_miner_fem.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/Hendrix.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/eos.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/masako.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/Capek.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/riot_guard.v3c"	//+ skeleton is wrong
		//"D:/dev/RF_PC/orig_data/meshes/ult2_guard.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/multi_guard2.v3c"	//+ with antennae
		//"D:/dev/RF_PC/orig_data/meshes/ult_scientist.v3c"	//+ 27 bones
		//"D:/dev/RF_PC/orig_data/meshes/env_scientist.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/elite_security_guard.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/Envirosuit_Guard.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/miner.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/miner4.v3c"	//+ miner lying in bed
		//"D:/dev/RF_PC/orig_data/meshes/UltorCommand.v3c"	//+- a guard, parsed with errors
		//"D:/dev/RF_PC/orig_data/meshes/merc_grunt.v3c"	//+ skeleton slightly out of alignment
		//"D:/dev/RF_PC/orig_data/meshes/merc_heavy.v3c"	//+






		// First-person weapons:
		//"D:/dev/RF_PC/orig_data/meshes/fp_rmt_chrg.v3c"	//+ skeleton is wrong!!!
		//"D:/dev/RF_PC/orig_data/meshes/fp_aslt_rfl.v3c"	//+ skeleton is wrong!!!
		//"D:/dev/RF_PC/orig_data/meshes/fp_railgun.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/fp_rocketlauncher.v3c"	//+
		//"D:/dev/RF_PC/orig_data/meshes/fp_hmac.v3c"	//+ HMG
		//"D:/dev/RF_PC/orig_data/meshes/fp_shol.v3c"	//+ fusion, skeleton is wrong!!!

		// Third-person view weapon models:
		//"D:/dev/RF_PC/orig_data/meshes/Weapon_RocketLauncher.v3m"	//-

		, storage_clump

		, &RF_test_model_
		));

	return ALL_OK;
}

ERet loadTestRFA(
				 RF1::Anim &RF_test_anim_
				 , const TestSkinnedModel& RF_test_model
				 )
{
	RF1::load_RFA_from_file(

		//"D:/dev/RF_PC/orig_data/motions/transport_idle.rfa"
		//"D:/dev/RF_PC/orig_data/motions/Transport_takeoff.rfa"

		//"D:/dev/RF_PC/orig_data/motions/trnsprt_rockets_fire.rfa"
		//"D:/dev/RF_PC/orig_data/motions/trnsprt_rockets_idle.rfa"

		//"D:/dev/RF_PC/orig_data/motions/turt_attack_stand.rfa"
		//"D:/dev/RF_PC/orig_data/motions/mft2_attack.rfa"


		// Robots:
		//"D:/dev/RF_PC/orig_data/motions/com1_ready_arms.rfa"
		//
		//"D:/dev/RF_PC/orig_data/motions/cutter_fly.rfa"
		//"D:/dev/RF_PC/orig_data/motions/cutter_attack-new.rfa"


		___ // Bat
		//"D:/dev/RF_PC/orig_data/motions/bat1_fly_slow.rfa"
		//"D:/dev/RF_PC/orig_data/motions/bat1_fly_fast.rfa"
		//"D:/dev/RF_PC/tools/RFToolkit/MVFReduce/bat1_fly_fast.rfa"

		//"D:/dev/RF_PC/orig_data/motions/fish_stand.rfa"
		//"D:/dev/RF_PC/orig_data/motions/fish_swim_slow.rfa"
		//"D:/dev/RF_PC/orig_data/motions/fish_swim_fast.rfa"

		___ // Sea creature
		//"D:/dev/RF_PC/orig_data/motions/seac_attack_ram.rfa"
		//"D:/dev/RF_PC/orig_data/motions/seac_swim_slow.rfa"
		//"D:/dev/RF_PC/orig_data/motions/seac_flinch.rfa"
		//"D:/dev/RF_PC/orig_data/motions/seac_death.rfa"
		//"D:/dev/RF_PC/orig_data/motions/seac_corpse.rfa"

		//"D:/dev/RF_PC/orig_data/motions/rsnk_attack_spit.rfa"




		//"D:/dev/RF_PC/orig_data/motions/tech01_run.rfa"
		
		"D:/dev/RF_PC/orig_data/motions/ADFM_typing.rfa"
		//"D:/dev/RF_PC/orig_data/motions/admin_fem_idle01.rfa"
		//"D:/dev/RF_PC/orig_data/motions/admin_fem_run.rfa"
		//"D:/dev/RF_PC/orig_data/motions/admin_fem_stand.rfa"
		//"D:/dev/RF_PC/orig_data/motions/capk_nano_attack.rfa"

		//"D:/dev/RF_PC/orig_data/motions/mrc1_cower.rfa"
		//"D:/dev/RF_PC/orig_data/motions/mrc1_flinch_back.rfa"

		//"D:/dev/RF_PC/orig_data/motions/fp_rmt_chrg_idlerub.rfa"

		, RF_test_anim_
		, RF_test_model
		);

	return ALL_OK;
}

#endif // TEST_LOAD_RED_FACTION_MODEL
