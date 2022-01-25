//
#pragma once

//#include <Graphics/Public/graphics_formats.h>
//#include <Graphics/Public/graphics_utilities.h>
//#include <Rendering/Public/Settings.h>

//
namespace DevTools
{

struct UIFlags {
	enum Enum {
		/// Memory
		ShowUI_Core			= BIT(0),

		/// low-level frame profiler (spikes/lags)
		ShowUI_Graphics			= BIT(1),
		/// inspect render textures
		ShowUI_GraphicsDebugger	= BIT(2),

		/// high-level frame profiler
		ShowUI_Rendering	= BIT(3),

		ShowUI_Physics		= BIT(4),

		ShowUI_Sound		= BIT(5),


		ShowUI_VoxelEngine	= BIT(10),
	};
};
mxDECLARE_FLAGS( UIFlags::Enum, U16, UIFlagsT );

ERet Draw_ImGUI_HUD(
	//RuntimeSettings & settings
	);

}//namespace DevTools
