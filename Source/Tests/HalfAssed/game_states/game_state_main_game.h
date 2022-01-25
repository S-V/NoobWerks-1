#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>
#include "game_forward_declarations.h"


enum EGameCameraMode
{
	GameCam_PlayerSpaceship,
	GameCam_DebugFreeFlight,
};

/*
-----------------------------------------------------------------------------
	RTSGameState_MainGame
-----------------------------------------------------------------------------
*/
class RTSGameState_MainGame: public NwGameStateI
{
public:
	EGameCameraMode		_game_camera_mode;

public:
	RTSGameState_MainGame();

	virtual EStateRunResult handleInput(
		NwGameI* game_app
		, const NwTime& game_time
		) override;

	virtual bool pinsMouseCursorToCenter() const override
	{return true;}

	virtual bool hidesMouseCursor() const
	{return true;}


	virtual EStateRunResult drawScene(
		NwGameI* game_app
		, const NwTime& game_time
		) override;

	virtual void drawUI(
		float delta_time_in_seconds
		, NwGameI* game_app
		) override;

private:
	void _handleInput_for_FirstPersonPlayerCamera(
		FPSGame* game
		, const NwTime& game_time
		, const gainput::InputMap& input_map
		, const gainput::InputState& keyboard_input_state
		);

	void _processInput_DebugTerrainCamera(
		//const gainput::InputState& keyboard_input_state
		const gainput::InputMap& input_map
		, FPSGame* game
		);

	void _handleInput_for_Controlling_Player_Weapons(
		FPSGame* game
		, const NwTime& game_time
		, const gainput::InputMap& input_map
		, const gainput::InputState& keyboard_input_state
		);

private:
	void _DrawDebugHUD_ImGui(
		const FPSGame& game
		);

	void _DrawDebugHUD(
		const FPSGame& game
		, const UInt2 window_size
		, nk_context* ctx
		);

	void _DrawInGameGUI(
		const FPSGame& game
		, const UInt2 window_size
		, nk_context* ctx
		);
};
