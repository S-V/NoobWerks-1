#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>
#include "common.h"


/*
-----------------------------------------------------------------------------
	SgAppState_InGameUI

	Strategy Game UI
-----------------------------------------------------------------------------
*/
class SgAppState_InGameUI: public NwGameStateI
{
public:
	SgAppState_InGameUI();

	virtual EStateRunResult handleInput(
		NwAppI* game_app
		, const NwTime& game_time
		) override;

	virtual void Update(
		const NwTime& game_time
		, NwAppI* game_app
		) override;

	virtual EStateRunResult DrawScene(
		NwAppI* game
		, const NwTime& game_time
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

