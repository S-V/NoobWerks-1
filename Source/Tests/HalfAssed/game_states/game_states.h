// NOTE: a lot of dependencies!!! include with caution!!!
#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>
#include "game_states/game_state_main_game.h"
#include "game_states/game_state_debug_hud.h"
#include "game_states/game_state_game_paused.h"
#include "game_states/game_state_do_really_wanna_exit.h"
#include "game_states/game_state_world_editor.h"

#include "game_states/windows/game_state_loading_progress.h"
#include "game_states/windows/game_state_escape_menu.h"
#include "game_states/windows/game_state_game_options_menu.h"
#include "game_states/windows/game_state_u_died_game_over.h"
#include "game_states/windows/game_state_game_completed.h"


/*
-----------------------------------------------------------------------------
	MyGameStates
-----------------------------------------------------------------------------
*/
struct MyGameStates: NonCopyable
{
	RTSGameState_MainGame		state_main_game;
	MyGameState_LoadingProgress	loading_progress;
	MyGameState_EscapeMenu		escape_menu;
	MyGameState_GameOptionsMenu	game_options_menu;
	MyGameState_GameCompleted	game_completed;

	RTSGameState_GamePaused		game_paused;

	RTSGameState_DoYouReallyWannaExit	state_do_really_wanna_exit;
	MyGameState_UDiedGameOver	u_died_game_over;

	NwGameState_ImGui_DebugHUD	state_debug_hud;

	// Editors
	MyGameState_LevelEditor			world_editor;
};


enum GUI_CONSTANTS
{
	UI_BUTTON_HEIGHT = 40,

	/// 3 widgets in a row (2*padding + button + 2*padding) == num of cols
	UI_SPACING_COLS = 2,
	UI_NUM_ITEMS_IN_ROW = UI_SPACING_COLS + 1 + UI_SPACING_COLS,
};
