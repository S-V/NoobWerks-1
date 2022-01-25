#pragma once

//#include <optional-lite/optional.hpp>	// nonstd::optional<>
//
//#include <Rendering/Public/Settings.h>	// Rendering::RrGlobalSettings
//#include <Rendering/Public/Core/VertexFormats.h>	// tbDECLARE_VERTEX_FORMAT
//#include <Rendering/Private/Modules/Sky/SkyModel.h>	// Rendering::SkyBoxGeometry
//#include <Rendering/Private/Modules/Atmosphere/Atmosphere.h>
//#include <Rendering/Private/Modules/Decals/_DecalsSystem.h>
//#include <Rendering/Private/Modules/Particles/BurningFire.h>
//#include <Rendering/Private/Modules/Particles/Explosions.h>
//#include <Utility/Camera/NwFlyingCamera.h>

#include "compile_config.h"

#if GAME_CFG_WITH_VOXEL_GI
#include <Rendering/Private/Modules/VoxelGI/_vxgi_system.h>
#include <Rendering/Private/Modules/VoxelGI/IrradianceField.h>
#endif


struct MyGameRendererData: NonCopyable
{
	// Rendering
	Rendering::NwCameraView		scene_view;


#if GAME_CFG_WITH_VOXEL_GI
	//
	Rendering::VXGI::VoxelGrids				voxel_grids;
	Rendering::VXGI::IrradianceField	irradiance_field;
#endif

	//
	AxisArrowGeometry			_gizmo_renderer;

	//
	TPtr< NwClump >		_scene_clump;

	bool	_is_drawing_debug_lines;
};
