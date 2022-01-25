//
#include "stdafx.h"
#pragma hdrstop

#include "gameplay_constants.h"



mxBEGIN_REFLECT_ENUM( PickableItem8 )
	mxREFLECT_ENUM_ITEM( Medkit, Gameplay::ITEM_MEDKIT ),

	mxREFLECT_ENUM_ITEM( Plasmagun, Gameplay::ITEM_PLASMAGUN ),
	//mxREFLECT_ENUM_ITEM( RocketLauncher, Gameplay::ITEM_ROCKETLAUNCHER ),

	mxREFLECT_ENUM_ITEM( RocketAmmo, Gameplay::ITEM_ROCKET_AMMO ),

	mxREFLECT_ENUM_ITEM( BFG_case, Gameplay::ITEM_BFG_CASE ),
mxEND_REFLECT_ENUM
