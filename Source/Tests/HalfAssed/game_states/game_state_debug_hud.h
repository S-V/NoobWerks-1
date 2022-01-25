#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>
// NwNuklearScoped
//#include <Utility/GUI/Nuklear/Nuklear_Support.h>
#include "game_forward_declarations.h"


/*
-----------------------------------------------------------------------------
	NwGameState_ImGui_DebugHUD
-----------------------------------------------------------------------------
*/
class NwGameState_ImGui_DebugHUD: public NwGameStateI
{
public:
	NwGameState_ImGui_DebugHUD();

	ERet Initialize(
		const TbRenderSystemI* render_system
		);
	void Shutdown();

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

private:
	void _draw_actual_ImGui(
		float delta_time_in_seconds
		, NwGameI* game_app
		);

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

	virtual void tick(
		float delta_time_in_seconds
		, NwGameI* game_app
		) override;

	virtual void drawUI(
		float delta_time_in_seconds
		, NwGameI* game_app
		) override;
};
