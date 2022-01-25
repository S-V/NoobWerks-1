#include <Base/Base.h>
#pragma hdrstop

#include <Rendering/Public/Globals.h>
#include <Voxels/private/vxp_voxel_engine.h>

#include <Engine/Private/EngineGlobals.h>

#include <Utility/GUI/ImGui/ImGUI_Renderer.h>
#include <Utility/GUI/ImGui/ImGUI_GraphicsDebugger.h>

#include <Developer/DevTools/ImGuiHUD.h>


namespace DevTools
{

mxBEGIN_FLAGS( UIFlagsT )
	mxREFLECT_BIT( Core,			UIFlags::ShowUI_Core ),
	mxREFLECT_BIT( Graphics,		UIFlags::ShowUI_Graphics ),
	mxREFLECT_BIT( GraphicsDebugger,UIFlags::ShowUI_GraphicsDebugger ),
	mxREFLECT_BIT( Rendering,		UIFlags::ShowUI_Rendering ),
	mxREFLECT_BIT( Physics,			UIFlags::ShowUI_Physics ),
	mxREFLECT_BIT( Sound,			UIFlags::ShowUI_Sound ),

	mxREFLECT_BIT( VoxelEngine,		UIFlags::ShowUI_VoxelEngine ),
mxEND_FLAGS




struct DevToolsSettings: CStruct
{
	UIFlagsT	ui_flags;

public:
	mxDECLARE_CLASS(DevToolsSettings, CStruct);
	mxDECLARE_REFLECTION;
	DevToolsSettings();
};

DevToolsSettings::DevToolsSettings()
{
	ui_flags.raw_value = 0;
}


ERet Draw_ImGUI_HUD(
					//RuntimeSettings & settings
					)
{
	static DevToolsSettings	dev_tools_settings;

	//
	if( ImGui::Begin("Engine Settings") )
	{
		ImGui_DrawFlagsCheckboxesT(
			&dev_tools_settings.ui_flags
			, "Panels to show"
			);
	}
	ImGui::End();


	//
	if( dev_tools_settings.ui_flags & UIFlags::ShowUI_GraphicsDebugger )
	{
		Graphics::DebuggerTool::ImGui_RenderSurfaces();
	}


	//
	if( dev_tools_settings.ui_flags & UIFlags::ShowUI_Rendering )
	{
		if( ImGui::Begin("Renderer") )
		{
			Rendering::RrGlobalSettings& rendering_settings = NEngine::g_settings.rendering;

			//
			const EditPropertyGridResult result = ImGui_DrawPropertyGridT(
				&rendering_settings,
				"Renderer Settings"
				);
			if(result != PropertiesDidNotChange)
			{
				DBGOUT("Renderer settings changed.");
				Rendering::Globals::ApplySettings(rendering_settings);
			}


			//ImGui::SliderFloat(
			//	"Animation Speed"
			//	, &app_settings.renderer.animation.animation_speed_multiplier
			//	, RrAnimationSettings::minValue()
			//	, RrAnimationSettings::maxValue()
			//	);

			////
			//if( ImGui::Button("Regenerate Light Probes" ) )//zzz
			//{
			//	((Rendering::NwRenderSystem&)Rendering::Globals::)._test_need_to_relight_light_probes = true;
			//}

			//
			//const U64 rendererSettingsHash0 = app_settings.renderer.computeHash();

			//
			//const U64 rendererSettingsHash1 = app_settings.renderer.computeHash();
			//if( rendererSettingsHash0 != rendererSettingsHash1 )
			//{
			//	DEVOUT("Renderer Settings changed");
			//	game->world.spatial_database->modifySettings( app_settings.renderer );
			//}

			//			ImGui::Text("Objects visible: %u / %u (%u culled)", g_FrontEndStats.objects_visible, g_FrontEndStats.objects_total, g_FrontEndStats.objects_culled);
			//ImGui::Text("Shadow casters rendered: %u", g_FrontEndStats.shadow_casters_rendered);
			//ImGui::Text("Meshes: %u", Rendering::NwMesh::s_totalCreated);
			//IF_DEBUG ImGui::Checkbox("Debug Break?", &g_DebugBreakEnabled);

#if 0
			m_visibleObjects.Reset();
			ViewFrustum	viewFrustum;
			viewFrustum.ExtractFrustumPlanes( view_projection_matrix );
			U64 startTimeMSec = mxGetTimeInMilliseconds();
			m_cullingSystem->FindVisibleObjects(viewFrustum, m_visibleObjects);
			U64 elapsedTimeMSec = mxGetTimeInMilliseconds() - startTimeMSec;
			ImGui::Text("Test Culling: %u visible (%u msec)", m_visibleObjects.count[RE_Model], elapsedTimeMSec);
#endif
		}
		ImGui::End();
	}//if


	
	//
	if( dev_tools_settings.ui_flags & UIFlags::ShowUI_VoxelEngine )
	{
		VX5::VoxelEngine* voxel_engine = VX5::VoxelEngine::Ptr();

		if(voxel_engine)
		{
			VX5::RunTimeEngineSettings & voxel_engine_settings
				= voxel_engine->_settings
				;

			if( ImGui::Begin("Voxel Engine") )
			{
				const U64 voxel_engine_settings_hash_before = CalcPodHash(voxel_engine_settings);
				ImGui_DrawPropertyGrid(
					&voxel_engine_settings,
					mxCLASS_OF(voxel_engine_settings),
					"Voxel Engine Settings"
					);
				const U64 voxel_engine_settings_hash_after = CalcPodHash(voxel_engine_settings);
				if(voxel_engine_settings_hash_before != voxel_engine_settings_hash_after)
				{
					DBGOUT("Voxel Engine Settings changed!!!");
					voxel_engine->ApplySettings(voxel_engine_settings);
				}
			}
			ImGui::End();
		}
		else
		{
			ImGui::TextColored(
				ImVec4(1,0,0,1)
				, "No voxel engine"
				);
		}
	}//if

	return ALL_OK;
}

}//namespace DevTools
