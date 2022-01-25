const bool DELETE_EXISTING_DATABASE_ON_START = 0;

#include <complex>

#include <Lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <Base/Base.h>
#include <Base/Math/Quaternion.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Template/Containers/BitSet/BitArray.h>
#include <Base/Text/String.h>
#include <Base/Math/ViewFrustum.h>

#include <bx/bx.h>	// uint8_t


#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Assets/AssetBundle.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Graphics/GraphicsSystem.h>
#include <Driver/Driver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/TxTSupport/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Renderer/Core/Material.h>
#include <Renderer/private/RenderSystem.h>
#include <Renderer/Shaders/__shader_globals_common.h>	// G_VoxelTerrain

#include <Renderer/private/render_world.h>	// G_VoxelTerrain

#include <Utility/Meshok/SDF.h>
#include <Utility/Meshok/BSP.h>
#include <Utility/Meshok/BIH.h>
#include <Utility/Meshok/Noise.h>
#include <Utility/Meshok/Volumes.h>
#include <Utility/MeshLib/Simplification.h>
#include <Utility/MeshLib/MeshCompiler.h>	// TcModel
#include <Utility/Meshok/Octree.h>

#include <Utility/TxTSupport/TxTConfig.h>
#include <Utility/TxTSupport/TxTWriter.h>

#include <Utility/VoxelEngine/VoxelizedMesh.h>
#include <Utility/MeshLib/CubeMesh.h>

//
#include <Developer/RunTimeCompiledCpp.h>

#include <DirectXMath/DirectXMath.h>


#include <Voxels/Data/vx5_chunk_data.h>
#include <Voxels/Base/Math/vx5_Quadrics.h>
#include <Voxels/Common/vx5_debug.h>
#include <Voxels/Meshing/Contouring/vx5_DualContouringTables.h>
#include <Voxels/Data/Chunking/vx5_GhostDataUpdate.h>

#include <Scripting/Scripting.h>
#include <Scripting/FunctionBinding.h>
// Lua G(L) crap
#undef G


#include "TowerDefenseGame.h"

#include "RCCpp_WeaponBehavior.h"

#if TB_TEST_LOAD_GLTF_MODEL
	#include <Utility/glTF/glTF_ModelLoader.h>
#endif // TB_TEST_LOAD_GLTF_MODEL


const char* PATH_TO_SETTINGS = "TowerDefenseGame.son";
const char* PATH_TO_SAVEFILE = "TowerDefenseGame.saved.son";




#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
	int USE_PLANETARY_TERRAIN_SHADER = 1;
#else
	int USE_PLANETARY_TERRAIN_SHADER = 0;
#endif




namespace CubeMeshUtil
{
	extern ERet createCubeMesh(
		NwMesh::Ref &mesh_
		, NwClump & storage
		);
}






mxBEGIN_FLAGS( tbPhysicsSystemFlagsT )
	mxREFLECT_BIT( dbg_pause_physics_simulation, tbPhysicsSystemFlags::dbg_pause_physics_simulation ),
mxEND_FLAGS

mxDEFINE_CLASS(Demo_PhysicsSettings);
mxBEGIN_REFLECTION(Demo_PhysicsSettings)
	mxMEMBER_FIELD( flags ),
	mxMEMBER_FIELD( enable_player_collisions ),
	mxMEMBER_FIELD( enable_player_gravity ),
	mxMEMBER_FIELD( dbg_draw_voxel_terrain_collisions ),
	mxMEMBER_FIELD( dbg_draw_bullet_physics ),
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




mxDEFINE_CLASS(VoxelEditorTools);
mxBEGIN_REFLECTION(VoxelEditorTools)
	mxMEMBER_FIELD( place_mesh_tool_is_enabled ),
	mxMEMBER_FIELD( gizmo_operation ),
	mxMEMBER_FIELD( last_saved_gizmo_transform_of_mesh ),
mxEND_REFLECTION;
VoxelEditorTools::VoxelEditorTools()
{
	place_mesh_tool_is_enabled = false;
	gizmo_operation = GizmoOperation::Translate;
	last_saved_gizmo_transform_of_mesh = M44_Identity();
}

void VoxelEditorTools::resetAllToDefaultSettings(
	const float chunk_length_units_in_world_space
	)
{
	place_mesh_tool_is_enabled = false;
	gizmo_operation = GizmoOperation::Translate;
	last_saved_gizmo_transform_of_mesh = M44_Identity();
}



mxDEFINE_CLASS(VX_Dbg_ChunkID);
mxBEGIN_REFLECTION(VX_Dbg_ChunkID)
	mxMEMBER_FIELD( coords ),
	mxMEMBER_FIELD( LoD_index ),
mxEND_REFLECTION;
VX_Dbg_ChunkID::VX_Dbg_ChunkID()
{
	coords = UInt3(0);
	LoD_index = 0;
}

mxBEGIN_FLAGS( VX_Dbg_CoarseNeighborsMaskT )
	mxREFLECT_BIT( VX_Neighbor0, VX_Neighbor0 ),
	mxREFLECT_BIT( VX_Neighbor1, VX_Neighbor1 ),
	mxREFLECT_BIT( VX_Neighbor2, VX_Neighbor2 ),
	mxREFLECT_BIT( VX_Neighbor3, VX_Neighbor3 ),

	mxREFLECT_BIT( VX_Neighbor4, VX_Neighbor4 ),
	mxREFLECT_BIT( VX_Neighbor5, VX_Neighbor5 ),
	mxREFLECT_BIT( VX_Neighbor6, VX_Neighbor6 ),
mxEND_FLAGS

mxBEGIN_FLAGS( VX_Dbg_OctantsMaskT )
	mxREFLECT_BIT( VX_Octant0, VX_Octant0 ),
	mxREFLECT_BIT( VX_Octant1, VX_Octant1 ),
	mxREFLECT_BIT( VX_Octant2, VX_Octant2 ),
	mxREFLECT_BIT( VX_Octant3, VX_Octant3 ),

	mxREFLECT_BIT( VX_Octant4, VX_Octant4 ),
	mxREFLECT_BIT( VX_Octant5, VX_Octant5 ),
	mxREFLECT_BIT( VX_Octant6, VX_Octant6 ),
	mxREFLECT_BIT( VX_Octant7, VX_Octant7 ),
mxEND_FLAGS


mxDEFINE_CLASS(VoxelEngineDebugState::LevelOfDetailStitchingState);
mxBEGIN_REFLECTION(VoxelEngineDebugState::LevelOfDetailStitchingState)
	mxMEMBER_FIELD( debug_chunk_id ),

	mxMEMBER_FIELD( mask_to_show_bounds_only_for_specified_coarse_neighbors ),
	mxMEMBER_FIELD( show_bounds_for_C0_continuity_only ),
	mxMEMBER_FIELD( ________________padding_for_ImGui_0 ),

	mxMEMBER_FIELD( show_coarse_cells_needed_for_C0_continuity ),
	mxMEMBER_FIELD( show_coarse_cells_needed_for_C1_continuity ),
	mxMEMBER_FIELD( top_level_octants_mask_in_seam_octree ),
	mxMEMBER_FIELD( ________________padding_for_ImGui_1 ),
mxEND_REFLECTION;
VoxelEngineDebugState::LevelOfDetailStitchingState::LevelOfDetailStitchingState()
{
	mask_to_show_bounds_only_for_specified_coarse_neighbors = VX_AllNeighbors;
	show_bounds_for_C0_continuity_only = true;

	//
	show_coarse_cells_needed_for_C0_continuity = true;
	show_coarse_cells_needed_for_C1_continuity = true;
	top_level_octants_mask_in_seam_octree = VX_AllOctants;
}

mxDEFINE_CLASS(VoxelEngineDebugState::GhostCellsVisualization);
mxBEGIN_REFLECTION(VoxelEngineDebugState::GhostCellsVisualization)
	mxMEMBER_FIELD( show_cached_coarse_cells ),
	mxMEMBER_FIELD( show_ghost_cells ),
	mxMEMBER_FIELD( show_cells_overlapping_maximal_neighbors_mesh ),
mxEND_REFLECTION;
VoxelEngineDebugState::GhostCellsVisualization::GhostCellsVisualization()
{
	show_cached_coarse_cells = true;
	show_ghost_cells = true;
	show_cells_overlapping_maximal_neighbors_mesh = true;
}

mxDEFINE_CLASS(VoxelEngineDebugState);
mxBEGIN_REFLECTION(VoxelEngineDebugState)
	mxMEMBER_FIELD( LoD_stitching ),
	mxMEMBER_FIELD( ghost_cells ),
	mxMEMBER_FIELD( show_ghost_cells_in_these_neighbors_mask ),
mxEND_REFLECTION;
VoxelEngineDebugState::VoxelEngineDebugState()
{
	show_ghost_cells_in_these_neighbors_mask.raw_value = 0;
}


mxDEFINE_CLASS(DemoPlanetRenderingSettings);
mxBEGIN_REFLECTION(DemoPlanetRenderingSettings)
	mxMEMBER_FIELD( enable_normal_mapping ),
	mxMEMBER_FIELD( dbg_show_tangents ),
mxEND_REFLECTION;
DemoPlanetRenderingSettings::DemoPlanetRenderingSettings()
{
	enable_normal_mapping = true;
	dbg_show_tangents = false;
}

mxDEFINE_CLASS(TowerDefenseGame::AppSettings);
mxBEGIN_REFLECTION(TowerDefenseGame::AppSettings)
mxMEMBER_FIELD( pbr_test_use_deferred ),
	//
	mxMEMBER_FIELD( fast_noise ),
	mxMEMBER_FIELD( fast_noise_result_scale ),
	mxMEMBER_FIELD( fast_noise_frequency_mul ),

	mxMEMBER_FIELD( planet ),

	mxMEMBER_FIELD( physics ),

	mxMEMBER_FIELD( ai ),

	mxMEMBER_FIELD( renderer ),


	mxMEMBER_FIELD( use_fake_camera ),
	mxMEMBER_FIELD( fake_camera_speed ),
	mxMEMBER_FIELD( fake_camera ),

	mxMEMBER_FIELD( window_offset_x ),
	mxMEMBER_FIELD( window_offset_y ),


	mxMEMBER_FIELD( dbg_draw_game_world ),

	mxMEMBER_FIELD( skyRenderingMode ),
	mxMEMBER_FIELD( skyboxAssetId ),
	mxMEMBER_FIELD( skydomeAssetId ),

	//
	mxMEMBER_FIELD( voxels_draw_debug_data ),
	mxMEMBER_FIELD( voxels_debug_viz ),

	mxMEMBER_FIELD( voxels ),
	mxMEMBER_FIELD( voxel_tool ),
	mxMEMBER_FIELD( phys_max_raycast_distance_for_voxel_tool ),

	mxMEMBER_FIELD( dev_voxel_tools ),
	mxMEMBER_FIELD( voxel_engine_debug_state ),
	mxMEMBER_FIELD( voxel_editor_tools ),

	//
	mxMEMBER_FIELD( showGUI ),
	mxMEMBER_FIELD( draw_fake_camera ),
	mxMEMBER_FIELD( drawGizmo ),
	mxMEMBER_FIELD( drawCrosshair ),
	//mxMEMBER_FIELD( drawSky ),
	mxMEMBER_FIELD( player_torchlight_params ),
	mxMEMBER_FIELD( terrain_roi_torchlight_params ),

	mxMEMBER_FIELD( csg_diff_sphere_radius ),

	mxMEMBER_FIELD( AO_step_size ),
	mxMEMBER_FIELD( AO_steps ),

	mxMEMBER_FIELD( calc_vertex_pos_using_QEF ),

	mxMEMBER_FIELD( vertexProjSteps ),
	mxMEMBER_FIELD( vertexProjLambda ),

	mxMEMBER_FIELD( dbg_viz_SDF ),

	mxMEMBER_FIELD_WITH_CUSTOM_NAME( dev_showSceneSettings, Show_Scene_Settings ),
	mxMEMBER_FIELD_WITH_CUSTOM_NAME( dev_showGraphicsDebugger, Show_Graphics_Debugger ),
	mxMEMBER_FIELD( dev_showRendererSettings ),
	mxMEMBER_FIELD_WITH_CUSTOM_NAME( dev_showVoxelEngineSettings, Show_Voxel_Engine_Settings ),
	mxMEMBER_FIELD_WITH_CUSTOM_NAME( dev_showVoxelEngineDebugMenu, Show_Voxel_Debug_Menu ),
	mxMEMBER_FIELD( dev_showTweakables ),
	mxMEMBER_FIELD( dev_showPerfStats ),
	mxMEMBER_FIELD( dev_showMemoryUsage ),

	mxMEMBER_FIELD( weapon_def ),
mxEND_REFLECTION;

TowerDefenseGame::AppSettings::AppSettings()
{
	pbr_test_use_deferred = false;

	window_offset_x = 0;
	window_offset_y = 0;

	voxels_draw_debug_data = true;
	phys_max_raycast_distance_for_voxel_tool = 10e4f;

	dbg_draw_game_world = false;

	skyRenderingMode = DemoSkyRenderMode::Skybox;


	fast_noise_result_scale = 1.0f;
	fast_noise_frequency_mul = 1.0f;


	showGUI = true;
	draw_fake_camera = false;
	drawGizmo = true;
	drawCrosshair = true;

	//drawSky = false;

	//fake_camera_position = V3_Zero();
	//fake_camera_direction = V3_FORWARD;
	fake_camera_speed = 1.0f;
	use_fake_camera = true;

	//csg_add_sphere_radius = VX5::CHUNK_SIZE_CELLS*0.4f;
	csg_diff_sphere_radius = 0.4f;
	//paint_brush_radius = VX5::CHUNK_SIZE_CELLS*0.4f;

	AO_step_size = 0.7f;
	AO_steps = 4;

	calc_vertex_pos_using_QEF = true;

	vertexProjSteps = 4;
	vertexProjLambda = 0.7f;

	dbg_viz_SDF = false;

	dev_showSceneSettings = false;
	dev_showGraphicsDebugger = false;
	dev_showRendererSettings = false;
	dev_showVoxelEngineSettings = false;
	dev_showVoxelEngineDebugMenu = false;
	dev_showTweakables = false;
	dev_showPerfStats = false;
	dev_showMemoryUsage = false;
}


TowerDefenseGame::TowerDefenseGame( AllocatorI & allocator )
	: m_dbgView( allocator )
	, m_blobTree( allocator )
#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
	, _planet_procgen_data( allocator )
#endif // TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
	, _stamped_mesh( allocator )
	, _worldState( allocator )
	, _dbg_pts( allocator )
	, _voxel_terrain_renderer(allocator)
//, _path_to_app_cache( engine_settings.path_to_app_cache )


{
#if ENABLE_HOT_RELOADING_OF_SCENE_SCRIPT
	m_sceneScriptChanged = 0;
#endif

}

ERet TowerDefenseGame::Initialize( const EngineSettings& engine_settings )
{


#if ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

	mxDO(RunTimeCompiledCpp::initialize( &g_SystemTable ));

#endif // ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP


	{
		WindowsDriver::Settings	options;
		options.window_width = engine_settings.window_width;
		options.window_height = engine_settings.window_height;
		options.window_x = m_settings.window_offset_x;
		options.window_y = m_settings.window_offset_y;
		options.window_y = Max(options.window_y, 20);
		mxDO(WindowsDriver::Initialize(options));

		HWND consoleWindow = GetConsoleWindow();
		SetWindowPos( consoleWindow, NULL, 0, 0, 1024, 800, SWP_NOSIZE | SWP_NOZORDER );
	}
	{
		mxDO(Testbed::Graphics::initialize(MemoryHeaps::graphics()));

		GL::Settings graphicsSettings;
		graphicsSettings.window = WindowsDriver::GetNativeWindowHandle();
		graphicsSettings.cmd_buf_size = mxMiB(engine_settings.Gfx_Cmd_Buf_Size_MiB);
		mxDO(GL::Initialize(graphicsSettings));
	}

	mxDO(DemoApp::Initialize(engine_settings));

	mxDO(initializeResourceLoaders( MemoryHeaps::resources() ));

	mxDO(initialize_Renderer( engine_settings ));

	_bindFunctionsToLua();

	this->m_gizmo_renderer.BuildGeometry();

	mxDO(Utilities::preallocateObjectLists( m_runtime_clump ));

	//
#if VOXEL_TERRAIN5_WITH_PHYSICS

		_physics_world.initialize();
		_physics_world.m_dynamicsWorld.setDebugDrawer(&_physics_debug_drawer);


		#if !VOXEL_TERRAIN5_CREATE_TERRAIN_COLLISION

		static btCollisionObject	infinitePlane;

		static btStaticPlaneShape	planeShape( toBulletVec(V3_UP), 0 );

		infinitePlane.setCollisionShape( &planeShape );
		_physics_world.m_dynamicsWorld.addCollisionObject( &infinitePlane );

		#endif // VOXEL_TERRAIN5_CREATE_TERRAIN_COLLISION

		#if VOXEL_TERRAIN5_WITH_PHYSICS_CHARACTER_CONTROLLER
			_player_controller.create(
				nil
				, &_physics_world
				, 100
				, 0.4f//VX5::getChunkSizeAtLoD(0)*0.32
				, 1.8f//VX5::getChunkSizeAtLoD(0)*0.64
				, nil
			);

			btTransform	t;
			t.setIdentity();
			t.setOrigin(toBulletVec(camera.m_eyePosition));
			_player_controller.getRigidBody()->setCenterOfMassTransform(t);
		#endif

		_physics_debug_drawer.init(&_render_system->m_debugRenderer);

#endif


		//_test_rotation_matrix
		//	= M33_Multiply(
		//		M33_Multiply( M33_RotationX(DEG2RAD(45)), M33_RotationY(DEG2RAD(45)) )
		//		, M33_RotationZ(DEG2RAD(45))
		//	);
		_test_rotation_matrix
			= M33_Multiply( M33_RotationX(DEG2RAD(45)), M33_RotationY(DEG2RAD(45)) );
//_test_rotation_matrix = M33_Identity();
//_test_rotation_matrix = M33_RotationZ(DEG2RAD(10));




	//

	AllocatorI & scratch = MemoryHeaps::temporary();

	//
#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
	//
	TPtr< NwMaterial >			procedural_voxel_planet_material;
	mxDO(Resources::Load(
		procedural_voxel_planet_material._ptr,
		MakeAssetID("material_voxel_planet"), &m_runtime_clump
	));
	//procedural_voxel_planet_material->effect->debugPrint();
	_voxel_terrain_material_instance->setFromMaterial( procedural_voxel_planet_material );

#else // !TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE

	mxDO(Demos::VoxelEngine_LoadTerrainMaterial(
		GL::getMainRenderContext(), scratch, m_runtime_clump, _voxel_terrain_renderer.materials
		));

#endif // !TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE



	//
#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
	mxDO(_planet_procgen_data.heightmap.loadFromFile(
		//"D:/dev/__PLANET_PROJECT/Moon.hmp"
		"D:/dev/__PLANET_PROJECT/Moon.hmp"
		));
#endif // TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE




	//mxDO(Resources::Load( _fallback_material._ptr, AssetID_From_FileName("material_default"), &m_runtime_clump ));



	//
	VX5::OctreeWorld::CreationSettings worldSettings;

#define REL_PATH_TO_DB_WITH_VOXELS		"Z_1"
#define REL_PATH_TO_DB_WITH_SEAMS		"Z_2"
#define REL_PATH_TO_DB_WITH_METADATA	"Z_3"

	worldSettings.path_to_DB_with_voxels = RELEASE_BUILD ? REL_PATH_TO_DB_WITH_VOXELS : "R:/"REL_PATH_TO_DB_WITH_VOXELS;
	worldSettings.path_to_DB_with_Seams = RELEASE_BUILD ? REL_PATH_TO_DB_WITH_SEAMS : "R:/"REL_PATH_TO_DB_WITH_SEAMS;
	worldSettings.path_to_DB_with_metadata = RELEASE_BUILD ? REL_PATH_TO_DB_WITH_METADATA : "R:/"REL_PATH_TO_DB_WITH_METADATA;


#if mxARCH_TYPE == mxARCH_32BIT
	//worldSettings.max_size_of_DB_with_metadata = mxMiB(8);
	//worldSettings.max_size_of_DB_with_voxels = mxMiB(64);
	//worldSettings.max_size_of_DB_with_Seams = mxMiB(64);
	worldSettings.max_size_of_DB_with_metadata = mxMiB(8);
	worldSettings.max_size_of_DB_with_voxels = mxMiB(64);
	worldSettings.max_size_of_DB_with_Seams = mxMiB(256);
#else
	worldSettings.max_size_of_DB_with_metadata = mxMiB(64);
	worldSettings.max_size_of_DB_with_voxels = mxTiB(1);
	worldSettings.max_size_of_DB_with_Seams = mxGiB(1);
#endif
worldSettings.delete_existing_database = DELETE_EXISTING_DATABASE_ON_START;
	worldSettings.clientApp = this;

	ATaskScheduler* task_sched_a = &TaskScheduler_Serial::s_instance;
	ATaskScheduler* task_sched_b = &TaskScheduler_Jq::s_instance;
	ATaskScheduler* task_sched = DEBUG_USE_SERIAL_JOB_SYSTEM
		? task_sched_a : task_sched_b;
		//? &TaskScheduler_Serial::s_instance : &TaskScheduler_Jq::s_instance;

	worldSettings.jobSystem = task_sched;

worldSettings.maxLoD = NUM_WORLD_LODS - 1;

//
const double chunk_size_at_LoD0 = VX5::getChunkSizeAtLoD( 0 );
const double chunk_size_at_maxLoD = VX5::getChunkSizeAtLoD( worldSettings.maxLoD );
const double world_size = chunk_size_at_maxLoD;
const double world_radius = world_size * 0.5;
//	worldSettings.octreeOrigin = CV3f(0.f);
	worldSettings.octreeOrigin = CV3d( -world_radius );


	//
	VX::g_DbgView = &m_dbgView;






	
	const V3d world_center = worldSettings.octreeOrigin + CV3d(world_radius);


	//
#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
	const double real_Moon_radius_in_meters = 1.7374e6;	// 1,737.4 km
	const double real_Moon_diameter_in_meters = real_Moon_radius_in_meters * 2.0;

	const int num_LoD0_chunks_along_Moon_size = ::ceil( real_Moon_diameter_in_meters / chunk_size_at_LoD0 );

	const UINT LoD_required_to_display_at_realistic_scale =
		calculate_LoD_needed_to_accomodate_planet_of_diameter(
			real_Moon_radius_in_meters, chunk_size_at_LoD0
		);
	DBGOUT("Need %u LoDs to display Moon at real-life scale! (%d minimal chunks)",
		LoD_required_to_display_at_realistic_scale+1, num_LoD0_chunks_along_Moon_size
		);


	//

	const int planet_heightmap_texture_width = 1 << _planet_procgen_data.heightmap.log2_of_mip0_width;
	_planet_procgen_data.updateHeightmapResolution( real_Moon_radius_in_meters, planet_heightmap_texture_width );
	DBGOUT("Heightmap resolution: %.2f pixels per degree or ~%.2f meters per pixel",
		planet_heightmap_texture_width / 360.0f, _planet_procgen_data.texel_size
		);



	//
	_planet_procgen_data.parameters.center_in_world_space = V3d::fromXYZ( world_center );
	const double mini_Moon_radius = world_radius * 0.7;
	_planet_procgen_data.parameters.setRadius( mini_Moon_radius );
	_planet_procgen_data.updateHeightmapResolution( mini_Moon_radius, planet_heightmap_texture_width );
	_planet_procgen_data.max_height = VX5::getCellSizeAtLoD(0);

#endif // TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE




//m_dbgView.addBox( world_bounds, RGBAf::darkgray );

	//
	m_settings.fast_noise.applyTo( _fast_noise );

	//
	mxDO(m_blobTreeBuilder.Initialize());
	_setupLuaForBlobTreeCompiler( V3f::fromXYZ(world_center), world_radius );

m_blobTree.user_param = world_radius;

	this->rebuildBlobTreeFromLuaScript();

#if ENABLE_HOT_RELOADING_OF_SCENE_SCRIPT
	efsw::WatchID id = m_luaScriptWatcher.addWatch(PATH_TO_LUA_SCRIPTS, this, false);
	m_luaScriptWatcher.watch();
#endif // ENABLE_HOT_RELOADING_OF_SCENE_SCRIPT

	_voxel_world._settings = m_settings.voxels;
	_voxel_world._settings.physics.island_detection = VX::IslandDetect_VoxelBased;
	_voxel_world._settings.generate_occlusion_data = true;

	mxDO(_voxel_world.Initialize(worldSettings));

	mxDO(_voxel_terrain_renderer.Initialize());
	_voxel_terrain_renderer._assetStorage = &m_runtime_clump;







#if TB_TEST_USE_SOUND_ENGINE
	mxDO(_sound_engine.initialize(
		&m_runtime_clump
		));
#endif // #if TB_TEST_USE_SOUND_ENGINE

	mxDO(_worldState.initialize(
		&m_runtime_clump
		, &_physics_world
		));


#if DRAW_SOLDIER
	_worldState.spawnNPC_Marine( _physics_world );
#endif

//	_worldState.create_Weapon_Plasmagun( _render_world, _physics_world );
//	_worldState.createTurret_Plasma( _render_world, _physics_world );




#if 0
{
	NwMesh *	mesh;
	mxDO(Resources::Load( mesh, MakeAssetID(
		"unit_sphere_ico"
		//"unit_cube"
		) ));

	NwMaterial *	material;
	mxDO(Resources::Load( material, MakeAssetID(
		"test_moon"
		//"uv-test"
		) ));

	//
	const M44f	initial_graphics_transform = M44_BuildTS( CV3f(-100), CV3f(50) );

	mxDO(RrMeshInstance::CreateFromMesh(
		_test_render_model._ptr,
		mesh,
		initial_graphics_transform,
		m_runtime_clump
		, material
		));

	for( UINT i = 0; i < _test_render_model->_materials.num(); i++ )
	{
		_test_render_model->_materials[i] = material;
	}

	const AABBf mesh_aabb_in_world_space = AABBf::fromSphere(CV3f(0), 9999); //mesh->;

	//
	_test_render_model->m_proxy = _render_world->addEntity(
		_test_render_model
		, RE_RigidModel
		, mesh_aabb_in_world_space
		);
}
#endif



{
	//
	mxDO(CubeMeshUtil::buildCubeMesh(
		&_stamped_mesh
		));

	//
	NwMesh::Ref		mesh;
	mxDO(CubeMeshUtil::createCubeMesh(
		mesh
		, m_runtime_clump
		));

	//
	NwMaterial *	fallback_material;
	mxDO(Resources::Load( fallback_material, MakeAssetID(
		"uv-test"
		) ));

	//
	const M44f	initial_world_transform = M44_Identity();

	mxDO(RrMeshInstance::ñreateFromMesh(
		_voxel_editor_model._ptr
		, mesh
		, m_runtime_clump
		, &initial_world_transform
		));

	const AABBf mesh_aabb_in_world_space = _stamped_mesh.local_AABB.transformed( initial_world_transform );

	//
	_voxel_editor_model->m_proxy = _render_world->addEntity(
		_voxel_editor_model
		, RE_RigidModel
		, mesh_aabb_in_world_space
		);
}

// Setup gizmo from saved data.
{
	_editor_manipulator.setGizmoEnabled( m_settings.voxel_editor_tools.place_mesh_tool_is_enabled );
	_editor_manipulator.setGizmoOperation( m_settings.voxel_editor_tools.gizmo_operation );
	_editor_manipulator.setLocalToWorldTransform( m_settings.voxel_editor_tools.last_saved_gizmo_transform_of_mesh );
}




#if TB_TEST_LOAD_GLTF_MODEL
glTF::createMeshFromFile(
	&gltf_mesh._ptr,

	//"D:/research/engines/DiligentSamples/Samples/GLTFViewer/assets/models/DamagedHelmet/DamagedHelmet.gltf"
	//"D:/research/engines/DiligentSamples/Samples/GLTFViewer/assets/models/MetalRoughSpheres/MetalRoughSpheres.gltf"
	//"D:/research/engines/DiligentSamples/Samples/GLTFViewer/assets/models/CesiumMan/CesiumMan.gltf"
	//"D:/research/engines/DiligentSamples/Samples/GLTFViewer/assets/models/FlightHelmet/FlightHelmet.gltf"

	//"D:/research/__test/glTF/WaterBottle/WaterBottle.gltf"	// great for testing - 1 mesh, no node transforms
	//"D:/research/__test/glTF/DamagedHelmet/DamagedHelmet.gltf"	// displayed as black as charcoal
	"D:/research/__test/glTF/FlightHelmet/FlightHelmet.gltf"	// good. large textures - can be used to test texture streaming!

	, &m_runtime_clump
	, gltf_submesh_materials
	);
#endif // TB_TEST_LOAD_GLTF_MODEL










	mxDO(_game_player.load(m_runtime_clump));




	return ALL_OK;
}

void TowerDefenseGame::Shutdown()
{

	_game_player.unload();

#if ENABLE_HOT_RELOADING_OF_SCENE_SCRIPT
	m_luaScriptWatcher.removeWatch(PATH_TO_LUA_SCRIPTS);
#endif

	VX::g_DbgView = &VX::ADebugView::ms_dummy;
	{
		HWND hWnd = (HWND) WindowsDriver::GetNativeWindowHandle();
		RECT rect;
		GetWindowRect( hWnd, &rect );
		m_settings.window_offset_x = rect.left;
		m_settings.window_offset_y = rect.top;
	}
	m_settings.voxels = _voxel_world._settings;




	_worldState.shutdown(
		);

#if TB_TEST_USE_SOUND_ENGINE
	_sound_engine.shutdown();
#endif // #if TB_TEST_USE_SOUND_ENGINE



	// NOTE: shutdown the voxel engine before the physics engine so that chunks can release collision shapes

	_voxel_terrain_renderer.Shutdown();

	_voxel_world.Shutdown();



#if VOXEL_TERRAIN5_WITH_PHYSICS
#if VOXEL_TERRAIN5_WITH_PHYSICS_CHARACTER_CONTROLLER
	_player_controller.destroy();
#endif

	_physics_world.Shutdown( m_runtime_clump );
#endif

	m_blobTreeBuilder.unbindFromLua(Scripting::GetLuaState());
	m_blobTreeBuilder.Shutdown();


	//
	m_runtime_clump.clear();


	//
	ARenderWorld::staticDestroy(_render_world);
	_render_world = nil;

	//
	shutdown_Renderer();

	_render_system->shutdown();
	TbRenderSystemI::destroy( _render_system );
	_render_system = nil;

	//
	shutdownResourceLoaders( MemoryHeaps::resources() );



	DemoApp::Shutdown();

	GL::Shutdown();
	Testbed::Graphics::shutdown();

	WindowsDriver::Shutdown();

	//
	//Resources::


#if ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

	RunTimeCompiledCpp::shutdown();

#endif // ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP
}

void TowerDefenseGame::setDefaultSettingsAfterEngineInit()
{
	camera.m_eyePosition = V3_Set(0, -100, 20);
	camera.m_movementSpeed = VX5::getChunkSizeAtLoD(0) * 4.0f;
	//camera.m_strafingSpeed = camera.m_movementSpeed;
	camera.m_rotationSpeed = 0.5;
	camera.m_invertYaw = true;
	camera.m_invertPitch = true;



	m_settings.renderer.sceneAmbientColor = V3_Set(0.4945, 0.465, 0.5) * 0.3f;

	//
	//m_settings.sun_direction = -V3_UP;
	//m_settings.sun_light.direction = V3_Normalized(V3_Set(1,1,-1));
	m_settings.sun_light.direction = V3_Normalized(V3_Set(-0.4,+0.4,-1));
//m_settings.sun_light.direction = V3_Set(1,0,0);
	if(0)
		m_settings.sun_light.color = V3_Set(0.5,0.7,0.6)* 0.1f;	// night
	else
		m_settings.sun_light.color = V3_Set(0.7,0.7,0.7);	// day
	//m_settings.sun_light.color = V3_Set(0.3,0.3,0.3);	// day

//m_settings.sun_light.color = V3_Set(0,1,0);
	m_settings.sun_light.ambientColor = V3_Set(0.4945, 0.465, 0.5);// * 0.1f;
	//m_settings.sun_light.ambientColor = V3f::set(0.017, 0.02, 0.019);
	//m_settings.sun_light.ambientColor = V3f::set(0.006, 0.009, 0.015);
	//m_settings.sun_light.ambientColor = V3f::zero();





	//
	const F32 terrainRegionSize = VX5::getChunkSizeAtLoD(0) * 3;

	//
	{
		RrShadowSettings &shadowSettings = m_settings.renderer.shadows;
		shadowSettings.num_cascades = MAX_SHADOW_CASCADES;

		mxSTATIC_ASSERT( MAX_SHADOW_CASCADES <= RrOffsettedCascadeSettings::MAX_CASCADES );

		float scale = 1;//0.3f;
		for( int i = 0; i < MAX_SHADOW_CASCADES; i++ )
		{
			float lodScale = ( 1 << i );
			shadowSettings.cascade_radius[i] = terrainRegionSize * lodScale * scale;
		}
	}

	//
	{
		RrSettings_VXGI &vxgiSettings = m_settings.renderer.gi;
		vxgiSettings.num_cascades = 1;

		float scale = 2;
		for( int i = 0; i < RrOffsettedCascadeSettings::MAX_CASCADES; i++ )
		{
			float lodScale = ( 1 << i ) * 0.5f;
			vxgiSettings.cascade_radius[i] = terrainRegionSize * lodScale;
			vxgiSettings.cascade_max_delta_distance[i] = terrainRegionSize * lodScale * scale;
		}
	}
}

void TowerDefenseGame::saveSettingsToFiles()
{
	SON::SaveToFile(camera, PATH_TO_SAVEFILE);

	m_settings.voxels = _voxel_world._settings;
	SON::SaveToFile(m_settings, PATH_TO_SETTINGS);
}

void TowerDefenseGame::handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename )
{
#if ENABLE_HOT_RELOADING_OF_SCENE_SCRIPT
	if( action == efsw::Actions::Modified && filename == LUA_SCENE_SCRIPT_FILE_NAME )
	{
		AtomicExchange(&m_sceneScriptChanged, 1);
	}
#endif
}

ERet TowerDefenseGame::initializeResourceLoaders( AllocatorI & allocator )
{
	return ALL_OK;
}

void TowerDefenseGame::shutdownResourceLoaders( AllocatorI & allocator )
{

}



M44f operator > ( const M44f& lhs, const M44f& rhs )
{
	return lhs * rhs;
}


ERet MyEntryPoint()
{
	M44f	local_to_world, world_to_screen;
	M44f	t2 = local_to_world > world_to_screen;


	EngineSettings engineSettings;
	{
		String256 path_to_engine_config;
		mxDO(SON::GetPathToConfigFile("engine_config.son", path_to_engine_config));

		TempAllocator4096	temp_allocator( &MemoryHeaps::process() );
		mxDO(SON::LoadFromFile( path_to_engine_config.c_str(), engineSettings, temp_allocator ));
	}

	mxDO(Engine::initialize( engineSettings ));





	////
	//for( UINT depth = 0; depth < MORTON32_MAX_OCTREE_LEVELS; depth++ )
	//{
	//	const U32 octree_resolution_at_this_depth = 1 << depth;
	//	const U32 max_octree_bound_at_this_depth = octree_resolution_at_this_depth - 1;
	//	const Morton32 dilated_octree_max_bound_by_depth = Morton32_SpreadBits3( max_octree_bound_at_this_depth );
	//	DEVOUT("0x%X, // at octree depth %u", dilated_octree_max_bound_by_depth, depth );
	//}




	////
	//TbAssetBundle	ab;
	//ab.mount("R:/testbed/.Build/Assets/weapons.TbAssetBundle");

	//AssetKey	ak;
	//ak.id.d = NameID("models/weapons/hands/hand");
	//ak.type = NwMaterial::metaClass().GetTypeGUID();

	//AssetReader	ar;
	//ab.Open( ak, &ar );



//	Meshok::TriMesh	mesh(MemoryHeaps::temporary());
//mxDO(mesh.ImportFromFile("R:/gun_TBS_001C.obj"));
//




	//TbDevAssetBundle	ab(MemoryHeaps::global(), make_AssetID_from_raw_string("doom3_sound_shaders"));
	//ab.mount("R:/testbed/.Build/Assets/doom3_sound_shaders.TbDevAssetBundle");



#if 0

	FileReader	stream("R:/testbed/.Build/Assets/doom3_meshes.TbDevAssetBundle");
	stream.Seek(225280);

	//
	AssetKey	key;
	key.id = make_AssetID_from_raw_string("models/md5/monsters/trite/trite.md5mesh");
	key.type = NwMesh::metaClass().GetTypeGUID();

	NwMesh	mesh;
	Testbed::TbMeshLoader::loadFromStream( mesh, stream, AssetId_ToChars(key.id) );

	mesh._submesh_material_ids.clear();
#endif


/*

	//
	//VX5::offline__build_LUT__touched_external_faces_mask8__to__cell_type27();
	//VX5::offline__build_LUT__touched_internal_faces_mask8__to__chunk_neighbors_mask26();

	{
		U32 face_mask =
			BIT(CubeFace::NegX)
			;

		VX5::MooreNeighborsMask27 f = VX5::g_LUT_touched_internal_faces_mask8__to__touched_chunk_neighbors_mask27[ face_mask ];
		ptWARN(f.dbgToString().c_str());
		//FaceNegX
	}
	{
		U32 face_mask =
			BIT(CubeFace::NegX) | BIT(CubeFace::PosY) | BIT(CubeFace::NegZ)	// Corner2
			;

		VX5::MooreNeighborsMask27 f = VX5::g_LUT_touched_internal_faces_mask8__to__touched_chunk_neighbors_mask27[ face_mask ];
		ptWARN(f.dbgToString().c_str());
		//FaceNegX|FacePosY|FaceNegZ|EdgeNum1|EdgeNum4|EdgeNumB|Corner2
	}

	{
		U32 face_mask =
			BIT(CubeFace::PosX) | BIT(CubeFace::NegY) | BIT(CubeFace::NegZ)	// Corner1
			;

		VX5::MooreNeighborsMask27 f = VX5::g_LUT_touched_internal_faces_mask8__to__touched_chunk_neighbors_mask27[ face_mask ];
		ptWARN(f.dbgToString().c_str());
		//FacePosX|FaceNegY|FaceNegZ|EdgeNum0|EdgeNum7|EdgeNum9|Corner1
	}


	{
		U32 face_mask =
			BIT(CubeFace::PosX) | BIT(CubeFace::NegY)
			;

		VX5::MooreNeighborsMask27 f = VX5::g_LUT_touched_internal_faces_mask8__to__touched_chunk_neighbors_mask27[ face_mask ];
		ptWARN(f.dbgToString().c_str());
		//FacePosX|FaceNegY|EdgeNum9
	}



	//
	VX5::TCDC::generateTables(
		VX5::TCDC::g_packed_table
		, VX5::TCDC::g_case_table
		, nil//"R:/DMC_packed_tables.h",
		, nil//"R:/DMC_cell_cases.h"
		);

*/




	{
		TowerDefenseGame app( MemoryHeaps::global() );

		app.setDefaultSettingsAfterEngineInit();

		{
			TempAllocator4096	temp_allocator( &MemoryHeaps::process() );
			SON::LoadFromFile( PATH_TO_SETTINGS, app.m_settings, temp_allocator );
			SON::LoadFromFile( PATH_TO_SAVEFILE, app.camera, temp_allocator );
		}

		{
			app._game_state_manager.handlers.add( ActionBindingT(GameActions::rotate_pitch, mxBIND_MEMBER_FUNCTION(FlyingCamera,rotate_pitch,app.camera)) );
			app._game_state_manager.handlers.add( ActionBindingT(GameActions::rotate_yaw, mxBIND_MEMBER_FUNCTION(FlyingCamera,rotate_yaw,app.camera)) );

			app._game_state_manager.handlers.add( ActionBindingT(
				GameActions::dev_toggle_gui
				, mxBIND_MEMBER_FUNCTION(TowerDefenseGame, dev_toggle_GUI, app)
			) );
			app._game_state_manager.handlers.add( ActionBindingT(
				GameActions::dev_toggle_wireframe
				, mxBIND_MEMBER_FUNCTION(TowerDefenseGame, dev_toggle_wireframe, app)
			) );
			app._game_state_manager.handlers.add( ActionBindingT(
				GameActions::execute_action
				, mxBIND_MEMBER_FUNCTION(TowerDefenseGame, execute_action, app)
			) );
		}

		mxDO(app.Initialize(engineSettings));

		Engine::run( app );

		// save settings before shutting down, because shutdown() may fail in render doc (D3D Debug RunTime)
		app.saveSettingsToFiles();

		//
		{
			ClumpInfo	clump_info( MemoryHeaps::global() );
			clump_info.buildFromClump( app.m_runtime_clump );
			clump_info.saveToFile("R:/runtime_clump.son");
		}

		//
		app.Shutdown();
	}




	Engine::shutdown();

	return ALL_OK;
}



ERet ClumpInfo::buildFromClump( const NwClump& clump )
{
	const U32	num_object_lists = clump.numAliveObjectLists();

	mxDO(_instances_map.Initialize(
		NextPowerOfTwo( num_object_lists )
		, num_object_lists
		));

	//
	const NwObjectList *	current_object_list = clump._object_lists;

	while( current_object_list )
	{
		const NwObjectList *	next_object_list = current_object_list->_next;

		//
		const TbMetaClass& metaclass = current_object_list->getType();

		const U32	object_list_capacity = current_object_list->capacity();

		//
		const U32* existing_count = _instances_map.find( &metaclass );
		if( existing_count )
		{
			_instances_map.Set( &metaclass, object_list_capacity + *existing_count );
		}
		else
		{
			_instances_map.Set( &metaclass, object_list_capacity );
		}

		//
		current_object_list = next_object_list;
	}

	return ALL_OK;
}

ERet ClumpInfo::saveToFile( const char* filename ) const
{
	SON::NodeAllocator	node_allocator( MemoryHeaps::global() );

	//
	SON::Node *	root_object = SON::NewObject( node_allocator );
	chkRET_X_IF_NIL(root_object, ERR_UNKNOWN_ERROR);

	//
	const InstanceCountByClassID::PairsArray& pairs = _instances_map.GetPairs();

	for( U32 i = 0; i < pairs.num(); i++ )
	{
		const InstanceCountByClassID::Pair& pair = pairs[i];

		//
		SON::Node* number_node = SON::NewNumber( pair.value, node_allocator );
		SON::AddChild( root_object, pair.key->GetTypeName(), number_node );
	}

	//
	FileWriter	file_stream;
	mxDO(file_stream.Open( filename ));

	//
	mxDO(SON::WriteToStream( root_object, file_stream ));

	return ALL_OK;
}

//ERet loadFromFile( const char* filename );




int main(int /*_argc*/, char** /*_argv*/)
{
	NwSetupMemorySystem	setupMemory;
//	SetupScratchHeap		setupScratchHeap( MemoryHeaps::global(), 64 );

	//	TIncompleteType<sizeof(GL::ViewState)>();
	//	TIncompleteType<sizeof(NwClump)>();

	UnitTest_Quaternions();

	MyEntryPoint();

	return 0;
}

// Ambient occlusion (AO) is a technique used to calculate how exposed each point in a scene is to ambient lighting.
// The lighting at each point is a function of other geometry in the scene.
// For example, the interior of a building is more occluded and thus appears darker than the outside of the building that is more exposed. 

// SDF ambient occlusion
// https://www.shadertoy.com/view/Xds3zN
