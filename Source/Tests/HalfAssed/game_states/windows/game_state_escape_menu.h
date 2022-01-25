#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>

/*
-----------------------------------------------------------------------------
	MyGameState_EscapeMenu
-----------------------------------------------------------------------------
*/
class MyGameState_EscapeMenu: public NwGameStateI
{
	bool should_navigate_back;
	bool should_navigate_to_options;

public:
	MyGameState_EscapeMenu();

	virtual void onWillBecomeActive() override
	{
		should_navigate_back = false;
		should_navigate_to_options = false;
	}

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
	{return true;}
};
