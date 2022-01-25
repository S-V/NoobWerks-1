#pragma once


ERet initialize_game_GUI();
void shutdown_game_GUI();




struct InputState;

extern const char* GAME_WINDOW_TITLE__CONSTRUCTION_CENTER;

ERet RTS_Game_GUI_draw_Screen__construction_center(
	const InputState& input_state
	);
