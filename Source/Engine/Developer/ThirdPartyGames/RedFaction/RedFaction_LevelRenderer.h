// Renderer for Red Faction 1 level files (*.rfl).
#pragma once

#include <Developer/ThirdPartyGames/RedFaction/RedFaction_LevelLoader.h>	// Rendering::NwCameraView

namespace RedFaction
{

	ERet drawLevel(
		RFL_Level & level	// non-const, because we load textures on-demand
		, const Rendering::NwCameraView& camera_view
		, NwClump * asset_storage
		, UINT *num_draw_calls_
		);

}//namespace RedFaction
