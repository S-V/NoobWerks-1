//
#include "stdafx.h"
#pragma hdrstop

#include <Base/Math/BoundingVolumes/VectorAABB.h>
#include <Base/Math/Utils.h>	// CalcDamping

#include <Core/Util/Tweakable.h>

#include <Renderer/Core/Mesh.h>
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Private/ShaderInterop.h>

#include "gameplay/gameplay_mgr.h"
#include "game_app.h"


#define SPAWN_LESS_ENEMIES	0//	(MX_DEBUG)	//!GAME_CFG_RELEASE_BUILD


ERet SgGameplayMgr::_SetupPlayerBase(
								   SgWorld& world
								   , SgPhysicsMgr& physics_mgr
								   )
{
	{
		SgShipHandle	h_ship_to_defend;
		mxDO(CreateSpaceship(
			h_ship_to_defend
			, obj_freighter
			, world
			, physics_mgr
			, CV3f(0,0,0)
			, fl_ship_is_mission_goal
			));
		SetShipToDefend(h_ship_to_defend);
	}




	SgShipHandle	heavy_battleship0_handle;
	{
		mxDO(CreateSpaceship(
			heavy_battleship0_handle
			, obj_heavy_battleship_ship
			, world
			, physics_mgr
			, CV3f(-20,0,10)
			));
	}


	SgShipHandle	heavy_battleship1_handle;
	{
		mxDO(CreateSpaceship(
			heavy_battleship1_handle
			, obj_heavy_battleship_ship
			, world
			, physics_mgr
			, CV3f(+20,0,10)
			));
	}


	//SgShipHandle	heavy_battleship1_handle;
	//{
	//	mxDO(CreateSpaceship(
	//		heavy_battleship1_handle
	//		, obj_heavy_battleship_ship
	//		, world
	//		, physics_mgr
	//		, CV3f(+20,0,0)
	//		));
	//}


	//SgShipHandle	heavy_battleship1_handle;
	//{
	//	mxDO(CreateSpaceship(
	//		heavy_battleship1_handle
	//		, obj_heavy_battleship_ship
	//		, world
	//		, physics_mgr
	//		, CV3f(+20,0,0)
	//		));
	//}




	{
		SgShipHandle	combat_fighter_ship_handle;
		mxDO(CreateSpaceship(
			combat_fighter_ship_handle
			, obj_combat_fighter_ship
			, world
			, physics_mgr
			, CV3f(+10,-10,0)
			));
	}




	//
	{

#if SPAWN_LESS_ENEMIES
		int num_ships_x = 20;
		int num_ships_y = 10;
#else
		int num_ships_x = 50;
		int num_ships_y = 30;
#endif

		float	spacing = 8;
		CV3f	pos_offset(
			num_ships_x * -(spacing * 0.5f),
			20,
			0
			);

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				SgShipHandle	fighter_ship_handle;
				mxDO(CreateSpaceship(
					fighter_ship_handle
					, obj_small_fighter_ship
					, world
					, physics_mgr
					, CV3f(i,j,0) * spacing + pos_offset
					));
			}
		}
	}


	//
	{
#if SPAWN_LESS_ENEMIES
		int num_ships_x = 20;
		int num_ships_y = 10;
#else
		int num_ships_x = 50;
		int num_ships_y = 30;
#endif

		float	spacing = 8;
		CV3f	pos_offset(
			num_ships_x * -(spacing * 0.5f),
			-60,
			-40
			);

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				SgShipHandle	fighter_ship_handle;
				mxDO(CreateSpaceship(
					fighter_ship_handle
					, obj_combat_fighter_ship
					, world
					, physics_mgr
					, CV3f(i,j,0) * spacing + pos_offset
					));
			}
		}
	}



	return ALL_OK;
}





ERet SgGameplayMgr::_SetupEnemyBase(
									SgWorld& world
									, SgPhysicsMgr& physics_mgr
									)
{
	const V3f	enemy_base_pos = { 0, 1200, 0 };


	// first wave
	{
		int num_ships_x = 4;
		int num_ships_y = 4;
		int num_ships_z = 1;

		const SgShipHandle	h_ship_to_defend = GetShipToDefend();

		//
		CV3f	pos_offset = enemy_base_pos;
		float	spacing = 16;

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				for(int k=0; k<num_ships_z; k++)
				{
					SgShipHandle	enemy_ship_handle;
					mxDO(CreateSpaceship(
						enemy_ship_handle
						, obj_alien_skorpion_ship// obj_alien_fighter_ship
						, world
						, physics_mgr
						, CV3f(i,j,k) * spacing + pos_offset
						, fl_ship_is_my_enemy
						));
					FindSpaceship(enemy_ship_handle)->longterm_goal_handle = h_ship_to_defend;
				}
			}
		}
	}



	//
	if(1)
	{
//#if 1
//		int num_ships_x = 1;
//		int num_ships_y = 1;
//		int num_ships_z = 1;
//#elif 0
//		int num_ships_x = 3;
//		int num_ships_y = 3;
//		int num_ships_z = 3;
//#elif 1
//		int num_ships_x = 10;
//		int num_ships_y = 10;
//		int num_ships_z = 5;
//#elif 0
//		// 1000
//		int num_ships_x = 10;
//		int num_ships_y = 10;
//		int num_ships_z = 10;
//#elif 1
//		// 3375
//		int num_ships_x = 15;
//		int num_ships_y = 15;
//		int num_ships_z = 15;
//#endif

		//// 15625 (< 16384)
		//int num_ships_x = 25;
		//int num_ships_y = 25;
		//int num_ships_z = 25;
		// 

#if SPAWN_LESS_ENEMIES
		int num_ships_x = 5;
		int num_ships_y = 5;
		int num_ships_z = 5;
#elif 1
		int num_ships_x = 10;
		int num_ships_y = 20;
		int num_ships_z = 15;
#else
		// 3375
		int num_ships_x = 15;
		int num_ships_y = 15;
		int num_ships_z = 15;
#endif

		const SgShipHandle	h_ship_to_defend = GetShipToDefend();

		DEVOUT("Creating %d enemy ships...", num_ships_x*num_ships_y*num_ships_z);

		CV3f	pos_offset = enemy_base_pos;
		float	spacing = 8;

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				for(int k=0; k<num_ships_z; k++)
				{
					SgShipHandle	enemy_ship_handle;
					mxDO(CreateSpaceship(
						enemy_ship_handle
						, obj_alien_skorpion_ship// obj_alien_fighter_ship
						, world
						, physics_mgr
						, CV3f(i,j,k) * spacing + pos_offset
						, fl_ship_is_my_enemy
						));
					//FindSpaceship(enemy_ship_handle)->longterm_goal_handle = h_ship_to_defend;
				}
			}
		}
	}

	return ALL_OK;
}




ERet SgGameplayMgr::SetupTestScene(
								   SgWorld& world
								   , SgPhysicsMgr& physics_mgr
								   )
{
	SgShipHandle	friendly_ship0_handle;
	mxDO(CreateSpaceship(
		friendly_ship0_handle
		, obj_small_fighter_ship
		, world
		, physics_mgr
		, CV3f(0,0,0)
		));

	//
	SgShipHandle	friendly_ship1_handle;
	mxDO(CreateSpaceship(
		friendly_ship1_handle
		, obj_heavy_battleship_ship
		, world
		, physics_mgr
		, CV3f(-0,-10,-20)
		));

	//
	SgShipHandle	enemy0_handle;
	mxDO(CreateSpaceship(
		enemy0_handle
		, obj_alien_skorpion_ship
		, world
		, physics_mgr
		, CV3f(-50,100,0)
		, fl_ship_is_my_enemy
		));

	SgShipHandle	enemy1_handle;
	mxDO(CreateSpaceship(
		enemy1_handle
		, obj_alien_skorpion_ship
		, world
		, physics_mgr
		, CV3f(50,100,0)
		, fl_ship_is_my_enemy
		));

#if 0
	for(int i = 0; i < obj_type_count; i++ )
	{
		SgShipHandle	tmp_ship_handle;

		mxDO(CreateSpaceship(
			tmp_ship_handle
			, (EGameObjType) i
			, world
			, physics_mgr
			, CV3f(i*20,0,0)
			));
	}
#endif

	//
#if 0
	SgShipHandle	friendly_ship0_handle;
	mxDO(CreateSpaceship(
		friendly_ship0_handle
		, obj_small_fighter_ship
		, world
		, physics_mgr
		, CV3f(0,0,0)
		));

	SgShipHandle	friendly_ship1_handle;
	mxDO(CreateSpaceship(
		friendly_ship1_handle
		, obj_small_fighter_ship
		, world
		, physics_mgr
		, CV3f(+5,0,0)
		));

	SgShipHandle	friendly_ship2_handle;
	mxDO(CreateSpaceship(
		friendly_ship2_handle
		, obj_small_fighter_ship
		, world
		, physics_mgr
		, CV3f(+10,0,0)
		));




	//
	SgShipHandle	enemy0_handle;
	mxDO(CreateSpaceship(
		enemy0_handle
		, obj_alien_fighter_ship
		, world
		, physics_mgr
		, CV3f(10,10,0)
		, fl_ship_is_my_enemy
		));
	GetSpaceship(enemy0_handle).target_handle = friendly_ship0_handle;


#if 0
	SgShipHandle	enemy1_handle;
	mxDO(CreateSpaceship(
		enemy1_handle
		, obj_small_fighter_ship
		, world
		, physics_mgr
		, CV3f(-20,10,0)
		));
	GetSpaceship(enemy1_handle).target_handle = alien_figher_ship_handle2;


	SgShipHandle	enemy2_handle;
	mxDO(CreateSpaceship(
		enemy2_handle
		, obj_small_fighter_ship
		, world
		, physics_mgr
		, CV3f(-20,-20,0)
		));
	GetSpaceship(enemy2_handle).target_handle = alien_figher_ship_handle3;
#endif
#endif





#if 0
	SgShipHandle	allied_ship_handle = { ~0 };
	mxDO(CreateSpaceship(
		allied_ship_handle
		, obj_heavy_battleship_ship
		, world
		, physics_mgr
		, CV3f(10,10,10)
		, fl_ship_is_my_enemy
		));
#endif


#if 0
	{
		int num_ships_x = 10;
		int num_ships_y = 10;

		CV3f	pos_offset(0, 0, 0);
		float	spacing = 8;

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				SgShipHandle	ship_handle;
				mxDO(CreateSpaceship(
					ship_handle
					, obj_small_fighter_ship
					, world
					, physics_mgr
					, CV3f(i,j,0) * spacing + pos_offset
					, fl_ship_is_my_enemy
					));
			}
		}
	}
#endif



#if 0
	{
		int num_ships_x = 2;
		int num_ships_y = 2;

		CV3f	pos_offset(20, 20, 0);
		float	spacing = 8;

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				SgShipHandle	ship_handle;
				mxDO(CreateSpaceship(
					ship_handle
					, obj_small_fighter_ship
					, world
					, physics_mgr
					, CV3f(i,j,0) * spacing + pos_offset
					//, fl_ship_is_my_enemy
					));

				FindSpaceship(ship_handle)->target_handle = alien_figher_ship_handle;
			}
		}
	}
#endif

#if 0
#if 0
	//
	mxDO(CreateSpaceship(
		SgShipHandle()
		, obj_capital_ship
		, world
		, physics_mgr
		, CV3f(200,200,200)
		, fl_ship_is_my_enemy
		));


	//
	mxDO(CreateSpaceship(
		SgShipHandle()
		, obj_spacestation
		, world
		, physics_mgr
		, CV3f(400,0,0)
		, fl_ship_is_mission_goal//fl_ship_is_my_enemy// 
		));

#endif

#if 1
#if 0
	int num_ships_x = 1;
	int num_ships_y = 1;
	int num_ships_z = 1;
#elif 0
	int num_ships_x = 3;
	int num_ships_y = 3;
	int num_ships_z = 3;
#elif 1
	// 1000
	int num_ships_x = 10;
	int num_ships_y = 10;
	int num_ships_z = 10;
#endif
	//// 3375
	//int num_ships_x = 15;
	//int num_ships_y = 15;
	//int num_ships_z = 15;

	//// 15625 (< 16384)
	//int num_ships_x = 25;
	//int num_ships_y = 25;
	//int num_ships_z = 25;


	DEVOUT("Creating %d ships...", num_ships_x*num_ships_y*num_ships_z);


	CV3f	pos_offset(7, 2, 3);
	float	spacing = 8;

	//
	for(int i=0; i<num_ships_x; i++)
	{
		for(int j=0; j<num_ships_y; j++)
		{
			for(int k=0; k<num_ships_z; k++)
			{
				SgShipHandle	ship_handle;
				mxDO(CreateSpaceship(
					ship_handle
					, obj_small_fighter_ship
					, world
					, physics_mgr
					, CV3f(i,j,k) * spacing + pos_offset
					, fl_ship_is_my_enemy
					));
				//FindSpaceship(enemy_ship_handle)->target_handle = allied_ship_handle;
			}
		}
	}
#endif
#endif

	return ALL_OK;
}








ERet SgGameplayMgr::SetupTestLevel(
	SgWorld& world
	, SgPhysicsMgr& physics_mgr
	)
{
	SgShipHandle	heavy_battleship0_handle;
	{
		mxDO(CreateSpaceship(
			heavy_battleship0_handle
			, obj_heavy_battleship_ship
			, world
			, physics_mgr
			, CV3f(-10,0,0)
			));
	}


	SgShipHandle	heavy_battleship1_handle;
	{
		mxDO(CreateSpaceship(
			heavy_battleship1_handle
			, obj_heavy_battleship_ship
			, world
			, physics_mgr
			, CV3f(+10,0,0)
			));
	}




	{
		SgShipHandle	combat_fighter_ship_handle;
		mxDO(CreateSpaceship(
			combat_fighter_ship_handle
			, obj_combat_fighter_ship
			, world
			, physics_mgr
			, CV3f(+10,-10,0)
			));
	}





	{
		int num_ships_x = 100;
		int num_ships_y = 10;

		CV3f	pos_offset(0, 20, 0);
		float	spacing = 8;

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				SgShipHandle	fighter_ship_handle;
				mxDO(CreateSpaceship(
					fighter_ship_handle
					, obj_small_fighter_ship
					, world
					, physics_mgr
					, CV3f(i,j,0) * spacing + pos_offset
					));
			}
		}
	}



	{
		int num_ships_x = 100;
		int num_ships_y = 10;

		CV3f	pos_offset(0, -20, 0);
		float	spacing = 8;

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				SgShipHandle	fighter_ship_handle;
				mxDO(CreateSpaceship(
					fighter_ship_handle
					, obj_small_fighter_ship
					, world
					, physics_mgr
					, CV3f(i,j,0) * spacing + pos_offset
					));
			}
		}
	}





	// enemies

	//mxDO(CreateSpaceship(
	//	SgShipHandle()
	//	, obj_spacestation
	//	, world
	//	, physics_mgr
	//	, CV3f(0, 400, 0)
	//	, fl_ship_is_mission_goal | fl_ship_is_my_enemy
	//	));


	if(1)
	{
#if 0
		int num_ships_x = 1;
		int num_ships_y = 1;
		int num_ships_z = 1;
#elif 0
		int num_ships_x = 3;
		int num_ships_y = 3;
		int num_ships_z = 3;
#elif 1
		int num_ships_x = 10;
		int num_ships_y = 10;
		int num_ships_z = 5;
#elif 0
		// 1000
		int num_ships_x = 10;
		int num_ships_y = 10;
		int num_ships_z = 10;
#elif 1
		// 3375
		int num_ships_x = 15;
		int num_ships_y = 15;
		int num_ships_z = 15;
#endif

		//// 15625 (< 16384)
		//int num_ships_x = 25;
		//int num_ships_y = 25;
		//int num_ships_z = 25;


		DEVOUT("Creating %d enemy ships...", num_ships_x*num_ships_y*num_ships_z);


		CV3f	pos_offset(0, 300, 0);
		float	spacing = 8;

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				for(int k=0; k<num_ships_z; k++)
				{
					SgShipHandle	enemy_ship_handle;
					mxDO(CreateSpaceship(
						enemy_ship_handle
						, obj_alien_fighter_ship
						, world
						, physics_mgr
						, CV3f(i,j,k) * spacing + pos_offset
						, fl_ship_is_my_enemy
						));
					FindSpaceship(enemy_ship_handle)->target_handle = heavy_battleship0_handle;
				}
			}
		}
	}

	return ALL_OK;
}

ERet SgGameplayMgr::SetupLevel0(
	SgWorld& world
	, SgPhysicsMgr& physics_mgr
	)
{
	{
		SgShipHandle	h_ship_to_defend;
		mxDO(CreateSpaceship(
			h_ship_to_defend
			, obj_freighter
			, world
			, physics_mgr
			, CV3f(0,0,0)
			, fl_ship_is_mission_goal
			));
		SetShipToDefend(h_ship_to_defend);
	}




	SgShipHandle	heavy_battleship0_handle;
	{
		mxDO(CreateSpaceship(
			heavy_battleship0_handle
			, obj_heavy_battleship_ship
			, world
			, physics_mgr
			, CV3f(-20,0,10)
			));
	}


	SgShipHandle	heavy_battleship1_handle;
	{
		mxDO(CreateSpaceship(
			heavy_battleship1_handle
			, obj_heavy_battleship_ship
			, world
			, physics_mgr
			, CV3f(+20,0,10)
			));
	}


	//SgShipHandle	heavy_battleship1_handle;
	//{
	//	mxDO(CreateSpaceship(
	//		heavy_battleship1_handle
	//		, obj_heavy_battleship_ship
	//		, world
	//		, physics_mgr
	//		, CV3f(+20,0,0)
	//		));
	//}


	//SgShipHandle	heavy_battleship1_handle;
	//{
	//	mxDO(CreateSpaceship(
	//		heavy_battleship1_handle
	//		, obj_heavy_battleship_ship
	//		, world
	//		, physics_mgr
	//		, CV3f(+20,0,0)
	//		));
	//}




	{
		SgShipHandle	combat_fighter_ship_handle;
		mxDO(CreateSpaceship(
			combat_fighter_ship_handle
			, obj_combat_fighter_ship
			, world
			, physics_mgr
			, CV3f(+10,-10,0)
			));
	}





	{
		int num_ships_x = 50;
		int num_ships_y = 30;

		float	spacing = 8;
		CV3f	pos_offset(
			num_ships_x * -(spacing * 0.5f),
			20,
			0
			);

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				SgShipHandle	fighter_ship_handle;
				mxDO(CreateSpaceship(
					fighter_ship_handle
					, obj_small_fighter_ship
					, world
					, physics_mgr
					, CV3f(i,j,0) * spacing + pos_offset
					));
			}
		}
	}


	{
		int num_ships_x = 50;
		int num_ships_y = 20;

		float	spacing = 8;
		CV3f	pos_offset(
			num_ships_x * -(spacing * 0.5f),
			-60,
			-40
			);

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				SgShipHandle	fighter_ship_handle;
				mxDO(CreateSpaceship(
					fighter_ship_handle
					, obj_combat_fighter_ship
					, world
					, physics_mgr
					, CV3f(i,j,0) * spacing + pos_offset
					));
			}
		}
	}




	// enemies

	//mxDO(CreateSpaceship(
	//	SgShipHandle()
	//	, obj_spacestation
	//	, world
	//	, physics_mgr
	//	, CV3f(0, 400, 0)
	//	, fl_ship_is_mission_goal | fl_ship_is_my_enemy
	//	));


	if(0)
	{
#if 1
		int num_ships_x = 1;
		int num_ships_y = 1;
		int num_ships_z = 1;
#elif 0
		int num_ships_x = 3;
		int num_ships_y = 3;
		int num_ships_z = 3;
#elif 1
		int num_ships_x = 10;
		int num_ships_y = 10;
		int num_ships_z = 5;
#elif 0
		// 1000
		int num_ships_x = 10;
		int num_ships_y = 10;
		int num_ships_z = 10;
#elif 1
		// 3375
		int num_ships_x = 15;
		int num_ships_y = 15;
		int num_ships_z = 15;
#endif

		//// 15625 (< 16384)
		//int num_ships_x = 25;
		//int num_ships_y = 25;
		//int num_ships_z = 25;

		const SgShipHandle	h_ship_to_defend = GetShipToDefend();

		DEVOUT("Creating %d enemy ships...", num_ships_x*num_ships_y*num_ships_z);

		CV3f	pos_offset(0, 1200, 0);
		float	spacing = 8;

		//
		for(int i=0; i<num_ships_x; i++)
		{
			for(int j=0; j<num_ships_y; j++)
			{
				for(int k=0; k<num_ships_z; k++)
				{
					SgShipHandle	enemy_ship_handle;
					mxDO(CreateSpaceship(
						enemy_ship_handle
						, obj_alien_skorpion_ship// obj_alien_fighter_ship
						, world
						, physics_mgr
						, CV3f(i,j,k) * spacing + pos_offset
						, fl_ship_is_my_enemy
						));
					//FindSpaceship(enemy_ship_handle)->target_handle = freighter_ship_handle;
					FindSpaceship(enemy_ship_handle)->longterm_goal_handle = h_ship_to_defend;
				}
			}
		}
	}

	return ALL_OK;
}
