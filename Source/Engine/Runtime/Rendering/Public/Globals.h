/*
=============================================================================
	File:	Renderer.h
	Desc:	High-level renderer interface.
=============================================================================
*/
#pragma once

#include <Rendering/ForwardDecls.h>

#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Public/Scene/Entity.h>


namespace Rendering
{

namespace Globals
{
ERet Initialize(
	const U32 screen_width, const U32 screen_height
	, const RrGlobalSettings& settings
	, NwClump* object_storage_clump
	);

void Shutdown();

void ApplySettings(
	const RrGlobalSettings& settings
	);

/// Updates per-scene constants, resizes render targets if needed.
ERet BeginFrame(
	const NwCameraView& scene_view
	, const RrGlobalSettings& renderer_settings
	, const NwTime& game_time
	);

const RenderPath& GetRenderPath();

TbPrimitiveBatcher& GetImmediateDebugRenderer();
TbDebugLineRenderer& GetRetainedDebugView();


}//namespace Globals





struct FrontEndStats : CStruct
{
	int objects_total;	//!< total number of renderable objects
	int objects_visible;//!<
	int shadow_casters_rendered;
	//U32	models_culled;
	//int models_rendered;
	//int deferred_lights;
	//int	tilesX, tilesY;
public:
	mxDECLARE_CLASS(FrontEndStats, CStruct);
	mxDECLARE_REFLECTION;
};
extern FrontEndStats	g_FrontEndStats;


}//namespace Rendering
