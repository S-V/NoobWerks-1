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

	ERet Initialize(
		const NwRenderSystemI* render_system
		);
	void Shutdown();

	virtual EStateRunResult handleInput(
		NwAppI* game_app
		, const NwTime& game_time
		) override;

	virtual void Update(
		const NwTime& game_time
		, NwAppI* game_app
		) override;

	virtual void DrawUI(
		const NwTime& game_time
		, NwAppI* game_app
		) override;

private:
	void _draw_actual_ImGui(
		const NwTime& game_time
		, NwAppI* game_app
		);

	void _ImGUI_DrawDeveloperHUD();

	void _Draw_Editor_UI_for_NwWeaponDef();
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
		, NwAppI* game_app
		) override;

	virtual void DrawUI(
		const NwTime& game_time
		, NwAppI* game_app
		) override;
};
