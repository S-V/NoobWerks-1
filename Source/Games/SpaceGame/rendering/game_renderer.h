// Game rendering
#pragma once

#include <optional-lite/optional.hpp>	// nonstd::optional<>

#include <Rendering/Public/Settings.h>	// RrGlobalSettings
#include <Rendering/Public/Core/VertexFormats.h>	// tbDECLARE_VERTEX_FORMAT
#include <Rendering/Private/Modules/Sky/SkyModel.h>	// Rendering::SkyBoxGeometry
#include <Rendering/Private/Modules/Atmosphere/Atmosphere.h>
#include <Rendering/Private/Modules/Decals/_DecalsSystem.h>
#include <Rendering/Private/Modules/Particles/BurningFire.h>
#include <Rendering/Private/Modules/Particles/Explosions.h>
#include <Utility/Camera/NwFlyingCamera.h>

#include "common.h"
#include "rendering/particles/game_particles.h"
#include "physics/aabb_tree.h"	// CulledObjectsIDs


struct NwEngineSettings;


struct SgRendererStats
{
	U32	num_objects_total;
	U32	num_objects_visible;
};


///
class SgRenderer: NwNonCopyable
{
public:
	TPtr< NwRenderSystemI >		_render_system;

	NwCameraView				scene_view;

	bool	should_highlight_visible_ships;


	AxisArrowGeometry			_gizmo_renderer;

	HTexture					h_skybox_cubemap;

	GameParticleRenderer		particle_renderer;

	NwDecalsSystem				decals;
	BurningFireSystem			burning_fires;
	RrExplosionsSystem			explosions;

	//
	Atmosphere_Rendering::NwAtmosphereRenderingParameters	_atmosphere_rendering_parameters;

	//
	TPtr< NwClump >		_scene_clump;

	bool	_is_drawing_debug_lines;

	SgRendererStats	stats;

public:
	SgRenderer();

	ERet initialize(
		const NwEngineSettings& engine_settings
		, const RrGlobalSettings& renderer_settings
		, NwClump* scene_clump
		);

	void shutdown();

	/// Sets up render states, updates matrices and global shader constants
	void BeginFrame(
		const NwTime& game_time
		, const SgWorld& world
		, const SgAppSettings& game_settings
		, const SgUserSettings& user_settings
		, const SgGameplayMgr& gameplay
		);

	//
	ERet RenderFrame(
		const SgWorld& world
		, const NwTime& game_time
		, const SgGameplayMgr& gameplay
		, const SgPhysicsMgr& physics_mgr
		, const SgAppSettings& game_settings
		, const SgUserSettings& user_settings
		, const SgHumanPlayer& player
		);

	void DrawCrosshair(bool highlight_as_enemy);

private:
	ERet _GenerateSkyboxCubemap(int seed = 0);

	ERet _PrepareDebugLineRenderer();

	void _DrawWorld(
		const SgWorld& world
		, const NwCameraView& camera_view
		, const NwTime& game_time
		, const SgAppSettings& game_settings
		, GL::NwRenderContext & render_context
		);

	ERet _DrawVisibleShips(
		const CulledObjectsIDs& culled_objects_ids
		, const NwCameraView& camera_view
		, const NwTime& game_time
		, const SgAppSettings& game_settings
		, const SgGameplayRenderData& gameplay_data
		, const SgPhysicsMgr& physics_mgr
		, GL::NwRenderContext & render_context
		);

	ERet _HighlightVisibleShips(
		const CulledObjectsIDs& culled_objects_ids
		, const NwCameraView& camera_view
		, const NwTime& game_time
		, const SgAppSettings& game_settings
		, const SgGameplayRenderData& gameplay_data
		, const SgPhysicsMgr& physics_mgr
		, GL::NwRenderContext & render_context
		);

	ERet _DrawProjectiles(
		const TSpan< const SgProjectile >& projectiles
		, const NwCameraView& camera_view
		, const NwTime& game_time
		, const SgAppSettings& game_settings
		, const SgGameplayRenderData& gameplay_data
		, const SgWorld& world
		, GL::NwRenderContext & render_context
		);

	void _DrawMisc(
		const NwCameraView& camera_view
		, const RrGlobalSettings& renderer_settings
		, const NwTime& game_time
		);

	ERet _HighlightSpaceshipThatCanBeControlledByPlayer();

	//
	//const NwAtmosphereRenderingParameters& getAtmosphereRenderingParameters() const;

private:	// Debugging.
	ERet _BeginDebugLineDrawing(
		const NwCameraView& camera_view
		, GL::NwRenderContext & render_context
		);

	ERet DebugDrawWorld(
		const SgWorld& game_world
		, const SpatialDatabaseI* spatial_database
		, const NwCameraView& camera_view
		, const SgHumanPlayer& game_player
		, const SgAppSettings& game_settings
		, const RrGlobalSettings& renderer_settings
		, const NwTime& game_time
		, GL::NwRenderContext & render_context
		);

	void _FinishDebugLineDrawing();

public:
	//ERet Render_VolumetricParticlesWithShader(
	//	const TSpan<ParticleVertex> particles
	//	);

	ERet Render_VolumetricParticlesWithShader(
		const AssetID& particle_shader_id
		, const int num_particles
		, ParticleVertex *&allocated_buffer_
		);

public:
	ERet Submit_Skybox();

	ERet submit_Star(
		const NwStar& star
		, const SgHumanPlayer& game_player
		, const NwCameraView& camera_view
		, GL::NwRenderContext &render_context
		);

private:
	//void _updateAtmosphereRenderingParameters(
	//	const NwCameraView& camera_view
	//	, const RrGlobalSettings& renderer_settings
	//	, const SgAppSettings& game_settings
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
