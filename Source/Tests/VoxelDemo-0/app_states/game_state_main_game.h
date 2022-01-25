#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>
#include "common.h"


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
		NwAppI* app
		, const NwTime& game_time
		) override;

	virtual bool pinsMouseCursorToCenter() const override
	{return true;}

	virtual bool hidesMouseCursor() const
	{return true;}


	virtual EStateRunResult DrawScene(
		NwAppI* app
		, const NwTime& game_time
		) override;

	virtual void DrawUI(
		const NwTime& game_time
		, NwAppI* app
		) override;

private:
	void _handleInput_for_FirstPersonPlayerCamera(
		MyApp* game
		, const NwTime& game_time
		, const gainput::InputMap& input_map
		, const gainput::InputState& keyboard_input_state
		);

	void _processInput_DebugTerrainCamera(
		//const gainput::InputState& keyboard_input_state
		const gainput::InputMap& input_map
		, MyApp* game
		);

private:
	void _DrawDebugHUD(
		const MyApp& game
		, const UInt2 window_size
		, nk_context* ctx
		);

	void _DrawInGameGUI(
		const MyApp& game
		, const UInt2 window_size
		, nk_context* ctx
		);
};
