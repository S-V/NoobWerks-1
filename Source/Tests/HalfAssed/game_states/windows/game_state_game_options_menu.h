#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>

/*
-----------------------------------------------------------------------------
	MyGameState_GameOptionsMenu
-----------------------------------------------------------------------------
*/
class MyGameState_GameOptionsMenu: public NwGameStateI
{
	bool should_navigate_back;

	bool settings_did_changed;

public:
	MyGameState_GameOptionsMenu();

	virtual void onWillBecomeActive() override
	{
		should_navigate_back = false;
		settings_did_changed = false;
	}

	virtual void onWillBecomeInactive() override;

	virtual EStateRunResult handleInput(
		NwGameI* game_app
		, const NwTime& game_time
		) override;

	virtual void tick(
		float delta_time_in_seconds
		, NwGameI* game_app
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

private:
	void _Draw_ImGui_Stuff(
		float delta_time_in_seconds
		, NwGameI* game_app
		, const NwViewport viewport
		);
};
