#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>
// NwNuklearScoped
//#include <Utility/GUI/Nuklear/Nuklear_Support.h>
#include "common.h"


/*
-----------------------------------------------------------------------------
	NwGameState_ImGui_DebugConsole
-----------------------------------------------------------------------------
*/
class NwGameState_ImGui_DebugConsole
	: public NwGameStateI
	, TSingleInstance<NwGameState_ImGui_DebugConsole>
{
public:
	NwGameState_ImGui_DebugConsole();

	ERet Setup();
	void Teardown();

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
};
