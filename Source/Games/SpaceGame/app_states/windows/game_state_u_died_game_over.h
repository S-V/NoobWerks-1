#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>

/*
-----------------------------------------------------------------------------
	MyGameState_UDiedGameOver
-----------------------------------------------------------------------------
*/
struct MyGameState_UDiedGameOver: NwGameStateI
{
	bool player_wants_to_continue_running_the_game_despite_mission_failure;
	bool player_chose_to_start_again;

public:
	MyGameState_UDiedGameOver();

	ERet Initialize();

	void Shutdown();

	virtual EStateRunResult handleInput(
		NwAppI* game_app
		, const NwTime& game_time
		) override;

	virtual void DrawUI(
		const NwTime& game_time
		, NwAppI* game_app
		) override;

	virtual bool pausesGame() const override
	{return true;}

	virtual bool pinsMouseCursorToCenter() const override
	{
		return false;
	}
	virtual bool hidesMouseCursor() const override
	{
		return false;
	}

	//
	virtual bool allowsFirstPersonCameraToReceiveInput() const
	{return false;}

	virtual bool AllowPlayerToReceiveInput() const
	{return false;}

	virtual bool ShouldRenderPlayerWeapon() const
	{return false;}
};
