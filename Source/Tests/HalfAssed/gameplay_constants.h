//
#pragma once


namespace Gameplay
{
	const int PLAYER_HEALTH = 100;
	//MAX_ARMOUR

	//
	enum EPickableItem
	{
		ITEM_MEDKIT,

		ITEM_PLASMAGUN,
		//ITEM_ROCKETLAUNCHER,

		ITEM_ROCKET_AMMO,	//-
		//ITEM_PLASMA_AMMO,

		ITEM_BFG_CASE,

		ITEM_COUNT
	};

	const int ROCKETS_IN_AMMOPACK = 10;
	const int PLASMACELLS_IN_AMMOPACK = 50;
};

mxDECLARE_ENUM( Gameplay::EPickableItem, U8, PickableItem8 );


namespace WeaponStats
{
	enum
	{
		PLASMAGUN_DAMAGE = 50,
		ROCKET_DAMAGE = 250,
	};
};


namespace NPCStats
{
	const float CAN_SEE_PLAYER_RANGE = 50;
};
