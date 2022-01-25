#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>
// NwNuklearScoped
//#include <Utility/GUI/Nuklear/Nuklear_Support.h>
#include "common.h"


/*
-----------------------------------------------------------------------------
	NwGameState_ImGui_DebugHUD
-----------------------------------------------------------------------------
*/
class NwGameState_ImGui_DebugHUD: public NwGameStateI
{
public:
	NwGameState_ImGui_DebugHUD();

	virtual EStateRunResult handleInput(
		NwAppI* app
		, const NwTime& game_time
		) override;

	virtual void Update(
		const NwTime& game_time
		, NwAppI* app
		) override;

	virtual void DrawUI(
		const NwTime& game_time
		, NwAppI* app
		) override;

private:
	void _Draw_Editor_UI_for_NwWeaponDef();
	ERet _Save_NwWeaponDef_to_File(const NwWeaponDef& weapon_def);
	ERet _Load_NwWeaponDef_from_File(NwWeaponDef &weapon_def_);
};


class DevConsoleState: public NwGameStateI {
public:
	DevConsoleState()
	{
		this->DbgSetName("DevConsole");
	}

	//virtual int GetStateMask() override
	//{ return BIT(GameStates::dev_console); }

	virtual void Update(
		const NwTime& game_time
		, NwAppI* app
		) override;

	virtual void DrawUI(
		const NwTime& game_time
		, NwAppI* app
		) override;
};
