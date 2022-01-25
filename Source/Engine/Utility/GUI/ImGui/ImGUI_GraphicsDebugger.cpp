#include <Base/Base.h>
#pragma hdrstop
#include <Core/Core.h>
#include <Core/ObjectModel/Clump.h>
#include <Core/Assets/AssetReference.h>

#if MX_DEVELOPER
#include <Core/Util/Tweakable.h>
#endif // MX_EDITOR

#include <Engine/WindowsDriver.h>
#include <Scripting/Scripting.h>
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_formats.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Graphics/Public/graphics_shader_system.h>
#include <GPU/Public/Debug/GraphicsDebugger.h>


#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>	// ImGuiWindow
#if MX_AUTOLINK
#pragma comment( lib, "ImGui.lib" )
#endif //MX_AUTOLINK


namespace Graphics
{
namespace DebuggerTool
{

void ImGui_RenderSurfaces()
{
	TSpan< Graphics::DebuggerTool::Surface > surfaces =
		Graphics::DebuggerTool::getSurfaces()
		;

	if( ImGui::Begin("Graphics Debugger") )
	{
		for( UINT i = 0; i < surfaces.num(); i++ )
		{
			Graphics::DebuggerTool::Surface & surface = surfaces[i];

			ImGui::Checkbox(
				surface.name.c_str(),
				&surface.is_being_displayed_in_ui
				);

			//
		}
	}
	ImGui::End();

	//

	for( UINT i = 0; i < surfaces.num(); i++ )
	{
		Graphics::DebuggerTool::Surface & surface = surfaces[i];

		if( surface.is_being_displayed_in_ui )
		{
			ImGui::SetNextWindowSize(ImVec2(256,256), ImGuiCond_FirstUseEver);

			if( ImGui::Begin(
				surface.name.c_str(),
				&surface.is_being_displayed_in_ui,
				ImGuiWindowFlags_NoScrollbar ) )
			{
				ImGuiWindow* window = ImGui::GetCurrentWindow();

				ImGui::Image( (ImTextureID) surface.handle.id, window->Size );
			}
			ImGui::End();
		}
	}
}

}//namespace DebuggerTool
}//namespace Graphics
