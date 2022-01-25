// NOTE: a lot of dependencies!!! include with caution!!!
#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>

#include "app_states/game_state_main_game.h"
#include "app_states/app_state_in_game_ui.h"
#include "app_states/game_state_debug_hud.h"
#include "app_states/game_state_game_paused.h"
#include "app_states/game_state_do_really_wanna_exit.h"

#include "app_states/windows/game_state_loading_progress.h"
#include "app_states/windows/game_state_escape_menu.h"
#include "app_states/windows/game_state_game_options_menu.h"
#include "app_states/windows/game_state_u_died_game_over.h"
//#include "app_states/windows/app_state_game_over.h"
#include "app_states/windows/game_state_game_completed.h"


/*
-----------------------------------------------------------------------------
	SgAppStates
-----------------------------------------------------------------------------
*/
struct SgAppStates: NwNonCopyable
{
	SgState_MainGame		state_main_game;
	MyGameState_LoadingProgress	loading_progress;
	SgAppState_InGameUI			in_game_ui;
	MyGameState_EscapeMenu		escape_menu;
	MyGameState_GameOptionsMenu	game_options_menu;
	MyGameState_GameCompleted	game_completed;

	SgGameState_GamePaused		game_paused;

	SgGameState_DoYouReallyWannaExit	state_do_really_wanna_exit;
	MyGameState_UDiedGameOver	u_died_game_over;	//-

	NwGameState_ImGui_DebugHUD	state_debug_hud;
};


enum GUI_CONSTANTS
{
	UI_BUTTON_HEIGHT = 40,

	/// 3 widgets in a row (2*padding + button + 2*padding) == num of cols
	UI_SPACING_COLS = 2,
	UI_NUM_ITEMS_IN_ROW = UI_SPACING_COLS + 1 + UI_SPACING_COLS,
};
