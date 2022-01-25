#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>

/*
-----------------------------------------------------------------------------
	MyGameState_LoadingProgress
-----------------------------------------------------------------------------
*/
class MyGameState_LoadingProgress: public NwGameStateI
{
	bool is_window_open;

public:
	MyGameState_LoadingProgress();

	ERet Initialize(
		);

	void Shutdown();

	virtual EStateRunResult handleInput(
		NwGameI* game_app
		, const NwTime& game_time
		) override;

	virtual void drawUI(
		float delta_time_in_seconds
		, NwGameI* game_app
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
