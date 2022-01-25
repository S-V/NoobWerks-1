#pragma once

#include "common.h"


struct MyGameRendererData;


///
class MyGameRenderer: NonCopyable
{
public:
	TPtr<MyGameRendererData> 	_data;

public:
	MyGameRenderer();

	ERet Initialize(
		const NEngine::LaunchConfig & engine_settings
		, const Rendering::RrGlobalSettings& renderer_settings
		, NwClump* scene_clump
		);

	void Shutdown();

	//

	/// Sets up render states, updates matrices and global shader constants
	void BeginFrame(
		const NwTime& game_time
		, const MyWorld& world
		, const MyAppSettings& app_settings
		, const MyGameUserSettings& user_settings
		, const MyGamePlayer& player
		);

	//
	ERet RenderFrame(
		const MyWorld& world
		, const NwTime& game_time
		, const MyAppSettings& app_settings
		, const MyGameUserSettings& user_settings
		, const MyGamePlayer& player
		);

	void ApplySettings( const Rendering::RrGlobalSettings& new_settings );

public:
	const Rendering::NwCameraView& GetCameraView() const;
	Rendering::NwCameraView& GetCameraViewNonConst();

	//
	//const NwAtmosphereRenderingParameters& getAtmosphereRenderingParameters() const;


public:
	
	ERet _PrepareDebugLineRenderer();

	void _DrawWorld(
		const MyWorld& world
		, const Rendering::NwCameraView& scene_view
		, const NwTime& game_time
		, const MyAppSettings& app_settings
		, NGpu::NwRenderContext & render_context
		);

	ERet _Submit_Skybox();

private:	// Debugging.
	ERet _BeginDebugLineDrawing(
		const Rendering::NwCameraView& scene_view
		, NGpu::NwRenderContext & render_context
		);

	ERet DebugDrawWorld(
		const MyWorld& game_world
		, const Rendering::SpatialDatabaseI* spatial_database
		, const Rendering::NwCameraView& scene_view
		, const MyGamePlayer& game_player
		, const MyAppSettings& app_settings
		, const Rendering::RrGlobalSettings& renderer_settings
		, const NwTime& game_time
		, NGpu::NwRenderContext & render_context
		);

	void _FinishDebugLineDrawing();
};
