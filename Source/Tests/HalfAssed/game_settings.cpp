//
#include "stdafx.h"
#pragma hdrstop

#include "game_settings.h"



mxDEFINE_CLASS(MyGameControlsSettings);
mxBEGIN_REFLECTION(MyGameControlsSettings)
	mxMEMBER_FIELD( invert_mouse_x ),
	mxMEMBER_FIELD( invert_mouse_y ),
	mxMEMBER_FIELD( mouse_sensitivity ),
mxEND_REFLECTION;
MyGameControlsSettings::MyGameControlsSettings()
{
	invert_mouse_x = false;
	invert_mouse_y = false;
	mouse_sensitivity = 1.0f;
}



mxDEFINE_CLASS(InGameSoundSettings);
mxBEGIN_REFLECTION(InGameSoundSettings)
	mxMEMBER_FIELD( effects_volume ),
	mxMEMBER_FIELD( music_volume ),
mxEND_REFLECTION;
InGameSoundSettings::InGameSoundSettings()
{
	effects_volume = 100;
	music_volume = 100;
}
void InGameSoundSettings::FixBadSettings()
{
	effects_volume = clampf(effects_volume, 0, 100);
	music_volume = clampf(music_volume, 0, 100);
}

mxDEFINE_CLASS(MyGameplaySettings);
mxBEGIN_REFLECTION(MyGameplaySettings)
	mxMEMBER_FIELD( destructible_environment ),
	mxMEMBER_FIELD( destructible_floor ),
	mxMEMBER_FIELD( shallow_craters ),
mxEND_REFLECTION;
MyGameplaySettings::MyGameplaySettings()
{
	destructible_environment = true;
	destructible_floor = false;
	shallow_craters = true;
}


mxDEFINE_CLASS(InGameSettings);
mxBEGIN_REFLECTION(InGameSettings)
	mxMEMBER_FIELD( controls ),
	mxMEMBER_FIELD( sound ),
	mxMEMBER_FIELD( gameplay ),
mxEND_REFLECTION;
InGameSettings::InGameSettings()
{

}

mxDEFINE_CLASS(MyGameUserSettings);
mxBEGIN_REFLECTION(MyGameUserSettings)
	mxMEMBER_FIELD( ingame ),
	mxMEMBER_FIELD( window ),
mxEND_REFLECTION;
MyGameUserSettings::MyGameUserSettings()
{
	this->SetDefaults();
}

void MyGameUserSettings::SetDefaults()
{
	window.SetDefaults();
}

mxDEFINE_CLASS(NwModelEditorSettings);
mxBEGIN_REFLECTION(NwModelEditorSettings)
	//mxMEMBER_FIELD( orbiting_camera ),
	mxMEMBER_FIELD( free_camera ),
	mxMEMBER_FIELD( mesh_transform ),
mxEND_REFLECTION;

mxBEGIN_REFLECT_ENUM( VoxelBrushShape8 )
	mxREFLECT_ENUM_ITEM( Sphere, VoxelBrushShape::Sphere ),
	mxREFLECT_ENUM_ITEM( Cube, VoxelBrushShape::Cube ),
	mxREFLECT_ENUM_ITEM( Mesh, VoxelBrushShape::Mesh ),
mxEND_REFLECT_ENUM


mxDEFINE_CLASS(VoxelBrushShape);
mxBEGIN_REFLECTION(VoxelBrushShape)
mxEND_REFLECTION;

mxDEFINE_CLASS(VoxelBrush_Sphere);
mxBEGIN_REFLECTION(VoxelBrush_Sphere)
	mxMEMBER_FIELD( center ),
	mxMEMBER_FIELD( radius ),
mxEND_REFLECTION;
VoxelBrush_Sphere::VoxelBrush_Sphere()
{
	center = CV3d(0);
	radius = 1;
}

mxDEFINE_CLASS(VoxelBrush_Cube);
mxBEGIN_REFLECTION(VoxelBrush_Cube)
	mxMEMBER_FIELD( center ),
	mxMEMBER_FIELD( half_size ),
mxEND_REFLECTION;
VoxelBrush_Cube::VoxelBrush_Cube()
{
	center = CV3d(0);
	half_size = 1;
}

mxDEFINE_CLASS(VoxelBrush_Mesh);
mxBEGIN_REFLECTION(VoxelBrush_Mesh)
	mxMEMBER_FIELD( mesh ),
	//mxMEMBER_FIELD( translation ),
	//mxMEMBER_FIELD( scale ),
mxEND_REFLECTION;
VoxelBrush_Mesh::VoxelBrush_Mesh()
{
	//material_id = VoxMat_RockA;
	//translation = CV3d(0);
	//scale = 1;
}

mxBEGIN_REFLECT_ENUM( VoxelTool8 )
	mxREFLECT_ENUM_ITEM( CSG_Add_Shape, VoxelTool::CSG_Add_Shape ),
	mxREFLECT_ENUM_ITEM( CSG_Subtract_Shape, VoxelTool::CSG_Subtract_Shape ),

	mxREFLECT_ENUM_ITEM( CSG_Add_Model, VoxelTool::CSG_Add_Model ),
	mxREFLECT_ENUM_ITEM( CSG_Subtract_Model, VoxelTool::CSG_Subtract_Model ),

	mxREFLECT_ENUM_ITEM( Pick_Material, VoxelTool::Pick_Material ),
	mxREFLECT_ENUM_ITEM( Paint_Brush, VoxelTool::Paint_Brush ),
	mxREFLECT_ENUM_ITEM( Paint_Brush_Model, VoxelTool::Paint_Brush_Model ),

	mxREFLECT_ENUM_ITEM( SmoothGeometry, VoxelTool::SmoothGeometry ),
	mxREFLECT_ENUM_ITEM( SharpenEdges, VoxelTool::SharpenEdges ),

	mxREFLECT_ENUM_ITEM( Grow, VoxelTool::Grow ),
	mxREFLECT_ENUM_ITEM( Shrink, VoxelTool::Shrink ),

	mxREFLECT_ENUM_ITEM( Copy, VoxelTool::Copy ),
	mxREFLECT_ENUM_ITEM( Cut, VoxelTool::Cut ),
	mxREFLECT_ENUM_ITEM( Paste, VoxelTool::Paste ),
mxEND_REFLECT_ENUM

mxDEFINE_CLASS(VoxelEditorSettings);
mxBEGIN_REFLECTION(VoxelEditorSettings)
	mxMEMBER_FIELD( current_tool_type ),
	mxMEMBER_FIELD( current_shape_type ),
	mxMEMBER_FIELD( current_material ),

	mxMEMBER_FIELD( gizmo_transform ),

	mxMEMBER_FIELD( sphere_brush ),
	mxMEMBER_FIELD( cube_brush ),
	mxMEMBER_FIELD( mesh_brush ),
mxEND_REFLECTION;
VoxelEditorSettings::VoxelEditorSettings()
{
	current_tool_type = VoxelTool::CSG_Add_Shape;
	current_shape_type = VoxelBrushShape::Cube;
	current_material = VoxMat_RockA;

	gizmo_transform = M44_Identity();
}

mxDEFINE_CLASS(NwInGameEditorSettings);
mxBEGIN_REFLECTION(NwInGameEditorSettings)
	mxMEMBER_FIELD( model_editor ),
	mxMEMBER_FIELD( voxel_editor ),
mxEND_REFLECTION;


mxBEGIN_REFLECT_ENUM( DemoSkyRenderModeT )
	mxREFLECT_ENUM_ITEM( None, DemoSkyRenderMode::None ),
	mxREFLECT_ENUM_ITEM( Skybox, DemoSkyRenderMode::Skybox ),
	mxREFLECT_ENUM_ITEM( Skydome, DemoSkyRenderMode::Skydome ),
	mxREFLECT_ENUM_ITEM( Analytic, DemoSkyRenderMode::Analytic ),
mxEND_REFLECT_ENUM


mxDEFINE_CLASS(DebugTorchLightParams);
mxBEGIN_REFLECTION(DebugTorchLightParams)
	mxMEMBER_FIELD( enabled ),
	mxMEMBER_FIELD( color ),
	mxMEMBER_FIELD( radius ),
mxEND_REFLECTION;
DebugTorchLightParams::DebugTorchLightParams()
{
	////m_torchlightPoint->color = V3_Set(0.9,0.6,0.7);
	enabled = false;
	color = V3_Set(0.6,0.8,0.7);
	radius = 250;
}




mxDEFINE_CLASS(GameInputSettings);
mxBEGIN_REFLECTION(GameInputSettings)
	mxMEMBER_FIELD( mouse_sensitivity ),
mxEND_REFLECTION;
GameInputSettings::GameInputSettings()
{
	mouse_sensitivity = 1.0f;
}

mxDEFINE_CLASS(VoxelSettings);
mxBEGIN_REFLECTION(VoxelSettings)
	mxMEMBER_FIELD( engine ),
mxEND_REFLECTION;
VoxelSettings::VoxelSettings()
{
}


mxDEFINE_CLASS(GameCheatsSettings);
mxBEGIN_REFLECTION(GameCheatsSettings)
	mxMEMBER_FIELD( enable_noclip ),
	mxMEMBER_FIELD( disable_gravity ),

	mxMEMBER_FIELD( godmode_on ),
	mxMEMBER_FIELD( infinite_ammo ),

	mxMEMBER_FIELD( slow_down_animations ),
	//mxMEMBER_FIELD( disable_AI ),
mxEND_REFLECTION;
GameCheatsSettings::GameCheatsSettings()
{
	enable_noclip = false;
	disable_gravity = false;

	godmode_on = false;
	infinite_ammo = false;

	slow_down_animations = false;

	//disable_AI = false;
}

mxDEFINE_CLASS(MyGameAISettings);
mxBEGIN_REFLECTION(MyGameAISettings)
	mxMEMBER_FIELD( debug_stop_AI ),
	mxMEMBER_FIELD( debug_draw_AI ),
mxEND_REFLECTION;
MyGameAISettings::MyGameAISettings()
{
	debug_stop_AI = false;
	//debug_draw_AI = false;
}




mxDEFINE_CLASS(MyGamePlayerMovementSettings);
mxBEGIN_REFLECTION(MyGamePlayerMovementSettings)
	mxMEMBER_FIELD( move_impulse ),
	mxMEMBER_FIELD( jump_impulse ),
	mxMEMBER_FIELD( walk_foot_cycle_interval ),
	mxMEMBER_FIELD( run_foot_cycle_interval ),
mxEND_REFLECTION;
MyGamePlayerMovementSettings::MyGamePlayerMovementSettings()
{
#if MX_DEBUG
	move_impulse = 40.0f;	// if player mass == 100
#else
	move_impulse = 9.0f;
#endif

	jump_impulse = 50.0f;

	walk_foot_cycle_interval = 0.5f;
	run_foot_cycle_interval = 0.3f;
}

mxDEFINE_CLASS(MyGamePhysicsSettings);
mxBEGIN_REFLECTION(MyGamePhysicsSettings)
	mxMEMBER_FIELD( debug_draw ),
mxEND_REFLECTION;
MyGamePhysicsSettings::MyGamePhysicsSettings()
{
	debug_draw = false;
}


mxDEFINE_CLASS(MyGameSettings);
mxBEGIN_REFLECTION(MyGameSettings)
	mxMEMBER_FIELD(path_to_levels),

	mxMEMBER_FIELD( cheats ),
	mxMEMBER_FIELD( ai ),

	mxMEMBER_FIELD( player_movement ),

	mxMEMBER_FIELD( physics ),

	//
	mxMEMBER_FIELD( editor ),

	mxMEMBER_FIELD( renderer ),

	//
	mxMEMBER_FIELD( dbg_3rd_person_camera_position ),
	mxMEMBER_FIELD( dbg_use_3rd_person_camera_for_terrain_RoI ),
	mxMEMBER_FIELD( dbg_3rd_person_camera_speed ),

	mxMEMBER_FIELD( window_offset_x ),
	mxMEMBER_FIELD( window_offset_y ),


	mxMEMBER_FIELD( dbg_draw_game_world ),

	mxMEMBER_FIELD( skyRenderingMode ),
	mxMEMBER_FIELD( skyboxAssetId ),
	mxMEMBER_FIELD( skydomeAssetId ),

	mxMEMBER_FIELD( voxels ),

	//
	mxMEMBER_FIELD( showGUI ),
	mxMEMBER_FIELD( draw_fake_camera ),
	mxMEMBER_FIELD( drawGizmo ),
	mxMEMBER_FIELD( drawCrosshair ),
	mxMEMBER_FIELD( player_torchlight_params ),
	mxMEMBER_FIELD( terrain_roi_torchlight_params ),

	mxMEMBER_FIELD( ui_show_game_settings ),
	mxMEMBER_FIELD( ui_show_cheats ),

	mxMEMBER_FIELD( ui_show_debug_text ),
	mxMEMBER_FIELD_WITH_CUSTOM_NAME( dev_showSceneSettings, Show_Scene_Settings ),
	mxMEMBER_FIELD_WITH_CUSTOM_NAME( dev_showGraphicsDebugger, Show_Graphics_Debugger ),
	mxMEMBER_FIELD( dev_showRendererSettings ),

	mxMEMBER_FIELD( show_editor_for_NwWeaponDef ),

	mxMEMBER_FIELD_WITH_CUSTOM_NAME( dev_showVoxelEngineSettings, Show_Voxel_Engine_Settings ),
	mxMEMBER_FIELD_WITH_CUSTOM_NAME( dev_showVoxelEngineDebugMenu, Show_Voxel_Debug_Menu ),
	mxMEMBER_FIELD( dev_showTweakables ),
	mxMEMBER_FIELD( dev_showPerfStats ),
	mxMEMBER_FIELD( dev_showMemoryUsage ),
	mxMEMBER_FIELD( ui_show_planet_settings ),
mxEND_REFLECTION;

MyGameSettings::MyGameSettings()
{

#if MX_DEVELOPER
	path_to_levels = "R:/levels";

	/// folder for storing persistent voxel world
	path_to_userdata = "R:/userdata";

#else
	path_to_levels = "Levels";

	/// folder for storing persistent voxel world
	path_to_userdata = "userdata";
#endif

	window_offset_x = 0;
	window_offset_y = 0;

	//
	dbg_disable_parallel_voxels = false;

	//
	dbg_draw_game_world = false;

	skyRenderingMode = DemoSkyRenderMode::Skybox;


	//fast_noise_scale = 1;
	//fast_noise_frequency_mul = 1.0f;


	showGUI = true;
	draw_fake_camera = false;
	drawGizmo = true;
	drawCrosshair = true;

	//drawSky = false;

	//fake_camera_position = V3_Zero();
	//fake_camera_direction = V3_FORWARD;
	dbg_3rd_person_camera_speed = 1.0f;
	dbg_use_3rd_person_camera_for_terrain_RoI = false;


	////csg_add_sphere_radius = VX5::CHUNK_SIZE_CELLS*0.4f;
	//csg_diff_sphere_radius = 0.4f;
	////paint_brush_radius = VX5::CHUNK_SIZE_CELLS*0.4f;

	//AO_step_size = 0.7f;
	//AO_steps = 4;

	//calc_vertex_pos_using_QEF = true;

	//vertexProjSteps = 4;
	//vertexProjLambda = 0.7f;

	//dbg_viz_SDF = false;

	ui_show_game_settings = true;
	ui_show_cheats = false;

	ui_show_debug_text = true;
	dev_showSceneSettings = false;
	dev_showGraphicsDebugger = false;
	dev_showRendererSettings = false;

	show_editor_for_NwWeaponDef = false;

	dev_showVoxelEngineSettings = false;
	dev_showVoxelEngineDebugMenu = false;
	dev_showTweakables = false;
	dev_showPerfStats = false;
	dev_showMemoryUsage = false;
	ui_show_planet_settings = false;
}

void MyGameSettings::FixBadValues()
{
	Str::NormalizePath(path_to_levels);
	Str::NormalizePath(path_to_userdata);
}
