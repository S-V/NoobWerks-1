// Game-wide settings (both debug/developer and in-game/user settings).
#pragma once

#include <Rendering/Public/Settings.h>	// RrGlobalSettings
#include <Rendering/Public/Globals.h>	// RrDirectionalLight
#include <Rendering/Private/Modules/Atmosphere/Atmosphere.h>

#include <Engine/WindowsDriver.h>	// NwWindowSettings

#include <Planets/Noise.h>	// HybridMultiFractal
#include <ProcGen/FastNoiseSupport.h>	// FastNoiseSettings
#include <ProcGen/Noise/NwNoiseFunctions.h>
#pragma comment( lib, "ProcGen.lib" )

#include <Voxels/public/vx5.h>	// VX5::RunTimeEngineSettings

// VoxMatID
#include <VoxelsSupport/VoxelTerrainRenderer.h>

//#include <Utility/VoxelEngine/VoxelTools.h>	// for VoxelEditorSettings
#include <Developer/ModelEditor/ModelEditor.h>	// OrbitingCamera



/*
===============================================================================
	PUBLIC SETTINGS
===============================================================================
*/

struct MyGameControlsSettings: CStruct
{
	bool	invert_mouse_x;
	bool	invert_mouse_y;
	float	mouse_sensitivity;

public:
	mxDECLARE_CLASS(MyGameControlsSettings, CStruct);
	mxDECLARE_REFLECTION;
	MyGameControlsSettings();
};


struct InGameSoundSettings: CStruct
{
	//U8		total_volume;	// 0-255 master volume
	int		effects_volume;	// [0..100]
	int		music_volume;	// [0..100]

public:
	mxDECLARE_CLASS(InGameSoundSettings, CStruct);
	mxDECLARE_REFLECTION;
	InGameSoundSettings();
	void FixBadSettings();
};


struct InGameVideoSettings: CStruct
{
	bool	enable_vsync;

public:
	mxDECLARE_CLASS(InGameVideoSettings, CStruct);
	mxDECLARE_REFLECTION;
	InGameVideoSettings();
};

struct MyGameplaySettings: CStruct
{
	bool	highlight_enemies;

public:
	mxDECLARE_CLASS(MyGameplaySettings, CStruct);
	mxDECLARE_REFLECTION;
	MyGameplaySettings();
};


struct InGameSettings: CStruct
{
	MyGameControlsSettings	controls;
	InGameVideoSettings		video;
	InGameSoundSettings		sound;
	MyGameplaySettings		gameplay;

public:
	mxDECLARE_CLASS(InGameSettings, CStruct);
	mxDECLARE_REFLECTION;
	InGameSettings();
};

struct SgDeveloperSettings: CStruct
{
	bool	show_hud;

	bool	visualize_BVH;

	bool	use_mesh_LODs;

	bool	use_multithreaded_update;

public:
	mxDECLARE_CLASS(SgDeveloperSettings, CStruct);
	mxDECLARE_REFLECTION;
	SgDeveloperSettings();
};

/// which can be changed by the user
struct SgUserSettings: CStruct
{
	InGameSettings		ingame;
	NwWindowSettings	window;

	SgDeveloperSettings	developer;

public:
	mxDECLARE_CLASS(SgUserSettings, CStruct);
	mxDECLARE_REFLECTION;
	SgUserSettings();

	void SetDefaults();
};




struct MyGamePlayerMovementSettings: CStruct
{
	// phys char controller
	float	move_impulse;
	float	jump_impulse;

	// interval between footsteps
	SecondsF	walk_foot_cycle_interval;
	SecondsF	run_foot_cycle_interval;

public:
	mxDECLARE_CLASS(MyGamePlayerMovementSettings, CStruct);
	mxDECLARE_REFLECTION;
	MyGamePlayerMovementSettings();
};



struct MyGamePhysicsSettings: CStruct
{
	bool	debug_draw;

public:
	mxDECLARE_CLASS(MyGamePhysicsSettings, CStruct);
	mxDECLARE_REFLECTION;
	MyGamePhysicsSettings();
};






struct NwModelEditorSettings: CStruct
{
	//OrbitingCamera	orbiting_camera;
	NwFlyingCamera		free_camera;

	NwLocalTransform	mesh_transform;

public:
	mxDECLARE_CLASS(NwModelEditorSettings, CStruct);
	mxDECLARE_REFLECTION;
};


///
struct VoxelBrushShape: CStruct
{
	enum Enum
	{
		Sphere,
		Cube,

		/// triangle mesh
		Mesh,
	};

public:
	mxDECLARE_CLASS(VoxelBrushShape, CStruct);
	mxDECLARE_REFLECTION;
};
mxDECLARE_ENUM( VoxelBrushShape::Enum, U8, VoxelBrushShape8 );

struct VoxelBrush_Sphere: VoxelBrushShape
{
	V3d		center;
	double	radius;

public:
	mxDECLARE_CLASS(VoxelBrush_Sphere, VoxelBrushShape);
	mxDECLARE_REFLECTION;
	VoxelBrush_Sphere();
};

struct VoxelBrush_Cube: VoxelBrushShape
{
	V3d		center;
	double	half_size;

public:
	mxDECLARE_CLASS(VoxelBrush_Cube, VoxelBrushShape);
	mxDECLARE_REFLECTION;
	VoxelBrush_Cube();
};

struct VoxelBrush_Mesh: VoxelBrushShape
{
	TResPtr<NwMesh>	mesh;

	//VoxMatID	material_id;

	//V3d		translation;
	//double	scale;

public:
	mxDECLARE_CLASS(VoxelBrush_Mesh, VoxelBrushShape);
	mxDECLARE_REFLECTION;
	VoxelBrush_Mesh();
};

///
struct VoxelTool
{
	enum Enum
	{
		//== Sculpting

		// working with brushes (simple shapes):
		CSG_Add_Shape,		// Additive Brush
		CSG_Subtract_Shape,	// Subtractive Brush

		// working with (closed) meshes:
		CSG_Add_Model,
		CSG_Subtract_Model,

		//== volume_material painting
		Pick_Material,
		Paint_Brush,
	
		/// use a polygonal model as a paintbrush
		Paint_Brush_Model,


		// smoothing
		SmoothGeometry,			// Smoothing Brush
		//SmoothMaterialTransitions,
		SharpenEdges,	// removes noise, recovers sharp edges and corners

		Grow,
		Shrink,

		//== Voxel Clipboard
		Copy,
		Cut,
		Paste,

		COUNT,
	};
};
mxDECLARE_ENUM( VoxelTool::Enum, U8, VoxelTool8 );

struct VoxelEditorSettings: CStruct
{
	VoxelTool8			current_tool_type;
	VoxelBrushShape8	current_shape_type;
	VoxMatID			current_material;

	// local-to-world matrix
	M44f				gizmo_transform;

	// saved shape settings
	VoxelBrush_Sphere	sphere_brush;
	VoxelBrush_Cube		cube_brush;
	VoxelBrush_Mesh		mesh_brush;

public:
	mxDECLARE_CLASS(VoxelEditorSettings, CStruct);
	mxDECLARE_REFLECTION;
	VoxelEditorSettings();
};



struct NwInGameEditorSettings: CStruct
{
	NwModelEditorSettings	model_editor;
	VoxelEditorSettings		voxel_editor;

public:
	mxDECLARE_CLASS(NwInGameEditorSettings, CStruct);
	mxDECLARE_REFLECTION;
};



struct DemoSkyRenderMode
{
	enum Enum
	{
		None,
		Skybox,
		Skydome,
		Analytic,
	};
};
mxDECLARE_ENUM( DemoSkyRenderMode::Enum, U8, DemoSkyRenderModeT );



struct DebugTorchLightParams: CStruct
{
	bool	enabled;
	V3f		color;	//!< rgb = diffuse color, a = falloff
	float	radius;

public:
	mxDECLARE_CLASS(DebugTorchLightParams, CStruct);
	mxDECLARE_REFLECTION;
	DebugTorchLightParams();
};



struct GameInputSettings: CStruct
{
	// 0.1..10.0f
	float	mouse_sensitivity;

public:
	mxDECLARE_CLASS(GameInputSettings, CStruct);
	mxDECLARE_REFLECTION;
	GameInputSettings();
};


struct VoxelSettings: CStruct
{
	VX5::RunTimeEngineSettings	engine;

public:
	mxDECLARE_CLASS(VoxelSettings, CStruct);
	mxDECLARE_REFLECTION;
	VoxelSettings();
};


struct SgCheatSettings: CStruct
{
	// the player can fly through walls
	// Used for flying through walls and outside the level.
	// Super helpful to skip sections to spotcheck areas of a level
	// during testing of a map during development
	// or to take screenshots from outside the level looking in.
	bool	enable_noclip;

	// the player is not affected by gravity
	/// Disables gravity for the player. Player still collides with everything.
	/// Useful flying through to test collisions in anticipation of enemy knockback or rocket jumping.
	bool	disable_gravity;

	// the player is invincible
	bool	godmode_on;

	bool	infinite_ammo;

	/// Makes player invisible to enemies and triggers.
	//notarget

	bool	slow_down_animations;

	//// Enemies don't attack the player
	//bool	disable_AI;

public:
	mxDECLARE_CLASS(SgCheatSettings, CStruct);
	mxDECLARE_REFLECTION;
	SgCheatSettings();
};


struct MyGameAISettings: CStruct
{
	bool	debug_stop_AI;
	bool	debug_draw_AI;
public:
	mxDECLARE_CLASS(MyGameAISettings, CStruct);
	mxDECLARE_REFLECTION;
	MyGameAISettings();
};

/// can be accessed via developer console
struct SgAppSettings: CStruct
{
	///
	String64	path_to_levels;

	/// where voxel databases are stored
	String64	path_to_userdata;


	GameInputSettings	input;

	SgCheatSettings	cheats;

	MyGameAISettings	ai;

	MyGamePlayerMovementSettings	player_movement;

	MyGamePhysicsSettings	physics;


	//Engine settings
	int	window_offset_x;
	int	window_offset_y;
	RrGlobalSettings	renderer;
	


	//=== NPC AI

	//AI_Settings			ai;


	//=== World Rendering
	bool	dbg_draw_game_world;

	// Sky
	RrDirectionalLight	sun_light;

	//
	DemoSkyRenderModeT		skyRenderingMode;
	String64				skyboxAssetId;
	AssetID					skydomeAssetId;


	//=== Voxel Terrain settings:
	VoxelSettings	voxels;

public:	// Development.

	NwInGameEditorSettings	editor;

public:	// Debugging / Terrain

	/// 3rd person view for testing & debugging
	V3d		dbg_3rd_person_camera_position;
	//View3D	dbg_3rd_person_camera_for_terrain_RoI;
	double	dbg_3rd_person_camera_speed;
	bool	dbg_use_3rd_person_camera_for_terrain_RoI;	//!< Center the clipmap around the fake camera?

	//

	bool	showGUI;	// draw debug HUD/UI?
	bool	draw_fake_camera;	//!< draw 3rd person camera?
	bool	drawGizmo;
	bool	drawCrosshair;

	//bool	drawSky;
	DebugTorchLightParams	player_torchlight_params;
	DebugTorchLightParams	terrain_roi_torchlight_params;

	bool	ui_show_game_settings;
	bool	ui_show_cheats;

	bool	ui_show_debug_text;

	bool 	dev_showSceneSettings;//!< debugging
	bool 	dev_showGraphicsDebugger;//!< debugging
	bool 	dev_showRendererSettings;//!< debugging

	bool 	dev_show_voxel_engine_settings;//!< debugging
	bool 	dev_show_voxel_engine_debug_menu;//!< debugging
	bool 	dev_showTweakables;//!< debugging
	bool 	dev_showPerfStats;//!< debugging
	bool 	dev_showMemoryUsage;//!< debugging
	bool 	ui_show_planet_settings;//!< debugging

public:
	mxDECLARE_CLASS(SgAppSettings, CStruct);
	mxDECLARE_REFLECTION;
	SgAppSettings();

	void FixBadValues();
};
