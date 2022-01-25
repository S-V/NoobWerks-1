//
#include <Base/Base.h>
#include <Core/Core.h>
#include <Core/Client.h>
#pragma hdrstop

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/RenderPipeline.h>

#include <Utility/GUI/ImGui/ImGUI_Renderer.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>

#include <Utility/GameFramework/NwGameStateManager.h>

#include "NwGameGUI.h"


ERet NwGameGUI::Initialize(
						 AllocatorI& allocator
						 , const size_t max_memory
						 )
{
	ImGui_Renderer::Config	cfg;
	cfg.save_to_ini = MX_DEVELOPER || MX_DEBUG;

	mxDO(ImGui_Renderer::Initialize(cfg));

	//
	mxDO(Nuklear_initialize(
		allocator
		, max_memory
		));

	return ALL_OK;
}

void NwGameGUI::Shutdown()
{
	Nuklear_shudown();

	//
	ImGui_Renderer::Shutdown();
}

ERet NwGameGUI::Draw(
	NwGameStateManager& state_mgr
	, const NwTime& game_time
	, NwAppI* app
	)
{
	// BEGIN DRAWING GUI
	{
		const U32	imgui_scene_pass_index = Rendering::Globals::GetRenderPath()
			.getPassDrawingOrder(mxHASH_STR("GUI"))
			;
		//
		ImGui_Renderer::BeginFrame(
			game_time.real.delta_seconds
			);

		//
		Nuklear_precache_Fonts_if_needed();


		// DRAW GUI

		//
		state_mgr.DrawUI(
			game_time
			, app
			);

		//
		const U32 scene_pass_index = Rendering::Globals::GetRenderPath()
			.getPassDrawingOrder(mxHASH_STR("HUD"))
			;
		Nuklear_render( scene_pass_index );

		//
		ImGui_Renderer::EndFrame(imgui_scene_pass_index);
	}
	// FINISH DRAWING GUI

	return ALL_OK;
}
