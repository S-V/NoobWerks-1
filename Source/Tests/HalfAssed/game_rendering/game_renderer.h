#pragma once

#include <optional-lite/optional.hpp>	// nonstd::optional<>

#include <Renderer/renderer_settings.h>	// RrRuntimeSettings
#include <Renderer/VertexFormats.h>	// tbDECLARE_VERTEX_FORMAT
#include <Renderer/Modules/Sky/SkyModel.h>	// Rendering::SkyBoxGeometry
#include <Renderer/Modules/Atmosphere/Atmosphere.h>
#include <Renderer/Modules/Decals/_DecalsSystem.h>
#include <Renderer/Modules/Particles/BurningFire.h>
#include <Renderer/Modules/Particles/Explosions.h>
#include <Utility/Camera/NwFlyingCamera.h>

#include "game_forward_declarations.h"
#include "game_rendering/particles/game_particles.h"


struct NwEngineSettings;

///
class MyGameRenderer: NonCopyable
{
public:

	// Rendering
	TPtr< TbRenderSystemI >		_render_system;

	NwCameraView				_camera_matrices;

	AxisArrowGeometry			_gizmo_renderer;

	GameParticleRenderer		particle_renderer;

	NwDecalsSystem				decals;
	BurningFireSystem			burning_fires;
	RrExplosionsSystem			explosions;

	//
	Atmosphere_Rendering::NwAtmosphereRenderingParameters	_atmosphere_rendering_parameters;

	//
	TPtr< NwClump >		_scene_clump;

	bool	_is_drawing_debug_lines;

public:
	MyGameRenderer();

	ERet initialize(
		const NwEngineSettings& engine_settings
		, const RrRuntimeSettings& renderer_settings
		, NwClump* scene_clump
		);

	void shutdown();

	//

	void beginFrame(
		const NwCameraView& camera_view
		, const RrRuntimeSettings& renderer_settings
		, const NwTime& game_time
		);

	//
	//const NwAtmosphereRenderingParameters& getAtmosphereRenderingParameters() const;


public:
	//ERet Render_VolumetricParticlesWithShader(
	//	const TSpan<ParticleVertex> particles
	//	);

	ERet Render_VolumetricParticlesWithShader(
		const AssetID& particle_shader_id
		, const int num_particles
		, ParticleVertex *&allocated_buffer_
		);

public:	// Debugging.

	ERet beginDebugLineDrawing(
		const NwCameraView& camera_view
		, GL::NwRenderContext & render_context
		);

	void endDebugLineDrawing();

	ERet drawWorld_Debug(
		const MyGameWorld& game_world
		, const ARenderWorld* render_world
		, const NwCameraView& camera_view
		, const MyGamePlayer& game_player
		, const MyGameSettings& game_settings
		, const RrRuntimeSettings& renderer_settings
		, const NwTime& game_time
		, GL::NwRenderContext & render_context
		);

public:
	ERet Submit_Skybox();

	ERet submit_Star(
		const NwStar& star
		, const MyGamePlayer& game_player
		, const NwCameraView& camera_view
		, GL::NwRenderContext &render_context
		);

private:
	//void _updateAtmosphereRenderingParameters(
	//	const NwCameraView& camera_view
	//	, const RrRuntimeSettings& renderer_settings
	//	, const MyGameSettings& game_settings
	//	);
};

/// for drawing raymarched billboard stars
struct BillboardStarVertex: CStruct
{
	/// sphere position and radius, relative to the camera
	V4f		sphere_in_view_space;

	// life: [0..1], if life > 1 => particle is dead
	V4f		velocity_and_life;

public:
	tbDECLARE_VERTEX_FORMAT(BillboardStarVertex);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};
