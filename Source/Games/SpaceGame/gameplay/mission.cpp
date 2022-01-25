//
#include "stdafx.h"
#pragma hdrstop

#include <Engine/Engine.h>	// NwTime
#include "gameplay/game_entities.h"
#include "world/world.h"
#include "game_app.h"
#include "gameplay/mission.h"


mxDEFINE_CLASS(SgMissionDef);
mxBEGIN_REFLECTION(SgMissionDef)
	//mxMEMBER_FIELD( mass ),
	////mxMEMBER_FIELD( model_scale ),

	//mxMEMBER_FIELD( max_speed ),
	//mxMEMBER_FIELD( thrust_acceleration ),

	//mxMEMBER_FIELD( mesh ),
mxEND_REFLECTION;
SgMissionDef::SgMissionDef()
{
	type = mission_destroy_structure;
}
