//
#include "stdafx.h"
#pragma hdrstop

#include <nativefiledialog/include/nfd.h>

#include <Core/Serialization/Text/TxTSerializers.h>

#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Private/Modules/VoxelGI/IrradianceField.h>
#include <Rendering/Private/Modules/VoxelGI/Private/VoxelBVH.h>
#include <Utility/GUI/ImGui/ImGUI_Renderer.h>

#include <Voxels/private/debug/vxp_debug.h>
#include <VoxelsSupport/VX_DebugHUD_ImGui.h>

#include <Engine/Private/EngineGlobals.h>

#include <Utility/GUI/ImGui/ImGUI_DebugConsole.h>
#include <Developer/DevTools/ImGuiHUD.h>

#include "app.h"
#include "rendering/renderer_data.h"
#include "app_states/game_states.h"
#include "app_states/game_state_debug_console.h"


namespace
{
	TPtr<ImGui_Console>	gs_console;
}//namespace

/*
-----------------------------------------------------------------------------
	NwGameState_ImGui_DebugConsole
-----------------------------------------------------------------------------
*/
NwGameState_ImGui_DebugConsole::NwGameState_ImGui_DebugConsole()
{
	this->DbgSetName("DebugConsole");
}

ERet NwGameState_ImGui_DebugConsole::Setup()
{
	gs_console.ConstructInPlace();
	return ALL_OK;
}

void NwGameState_ImGui_DebugConsole::Teardown()
{
	gs_console.Destruct();
}

EStateRunResult NwGameState_ImGui_DebugConsole::handleInput(
	NwAppI* app
	, const NwTime& game_time
)
{
	MyApp* game = checked_cast< MyApp* >( app );

	if( game->input.wasButtonDown( eGA_Exit ) )
	{
		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	if( game->input.wasButtonDown( eGA_Show_Debug_Console ) )
	{
		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	// debug console blocks main game UI
	return StopFurtherProcessing;
}

void NwGameState_ImGui_DebugConsole::Update(
								const NwTime& game_time
								, NwAppI* app
								)
{
	ImGui_Renderer::UpdateInputs();
}

void NwGameState_ImGui_DebugConsole::DrawUI(
								  const NwTime& game_time
								  , NwAppI* app
								  )
{
	MyApp* game = checked_cast< MyApp* >( app );

	MyAppSettings & app_settings = game->settings;

	bool is_console_opened = true;

	gs_console->Draw("Console", &is_console_opened);
}
