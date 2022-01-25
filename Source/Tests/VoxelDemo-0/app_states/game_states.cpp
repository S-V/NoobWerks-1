//
#include "stdafx.h"
#pragma hdrstop

#include "app_states/game_states.h"

/*
-----------------------------------------------------------------------------
	MyGameStates
-----------------------------------------------------------------------------
*/

ERet MyGameStates::Setup()
{
	mxDO(state_debug_console.Setup());
	return ALL_OK;
}

void MyGameStates::Teardown()
{
	state_debug_console.Teardown();
}
