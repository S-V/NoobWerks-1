#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>
#include "common.h"


/*
-----------------------------------------------------------------------------
	SgState_MainGame
-----------------------------------------------------------------------------
*/
class SgState_MainGame: public NwGameStateI
{
public:

public:
	SgState_MainGame();

	virtual EStateRunResult handleInput(
		NwAppI* game_app
		, const NwTime& game_time
		) override;


	virtual bool pinsMouseCursorToCenter() const override
	{return true;}

	virtual bool hidesMouseCursor() const
	{return true;}


	virtual void Update(
		const NwTime& game_time
		, NwAppI* game
		) override;


	virtual EStateRunResult DrawScene(
		NwAppI* game_app
		, const NwTime& game_time
		) override;

	virtual void DrawUI(
		const NwTime& game_time
		, NwAppI* game_app
		) override;

private:
	void _HandlePlayerInputForControllingCamera(
		SgGameApp& game
		, const NwTime& game_time
		, const gainput::InputMap& input_map
		, const gainput::InputState& keyboard_input_state
		);

	void _handleInput_for_FirstPersonPlayerCamera(
		SgGameApp* game
		, const NwTime& game_time
		, const gainput::InputMap& input_map
		, const gainput::InputState& keyboard_input_state
		);

	void _processInput_DebugTerrainCamera(
		//const gainput::InputState& keyboard_input_state
		const gainput::InputMap& input_map
		, SgGameApp& game
		);

private:
	void _DrawDebugHUD_ImGui(
		const SgGameApp& game
		);

	void _DrawDebugHUD(
		const SgGameApp& game
		, const UInt2 window_size
		, nk_context* ctx
		);

	void _DrawInGameGUI(
		const SgGameApp& game
		, const UInt2 window_size
		, nk_context* ctx
		);
};
