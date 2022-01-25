#pragma once

#define DEBUG_USE_SERIAL_JOB_SYSTEM (1)




#define PHYSICS_FOR_DEMO_RELEASE	(0)

#define TB_TEST_USE_SOUND_ENGINE	(0)


/// for debugging PBR shaders
#define TB_TEST_LOAD_GLTF_MODEL			(0)


#define ENABLE_HOT_RELOADING_OF_SCENE_SCRIPT	(0)


#define TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE	(0)
#define TEST_VOXEL_TERRAIN_PROCEDURAL_TERRAIN_MODE	(0)

#define VOXEL_TERRAIN5_WITH_PHYSICS	(0)
#define VOXEL_TERRAIN5_CREATE_TERRAIN_COLLISION	(0)
#define VOXEL_TERRAIN5_WITH_PHYSICS_CHARACTER_CONTROLLER	(0)




const bool SHOW_MD5_ANIM = 0;
const bool SHOW_OZZ_ANIM = 0;
const bool SHOW_SKINNED_MESH = true;

const int MAX_LOD_FOR_COLLISION_PHYSICS = 8;






#pragma comment( lib, "bx.lib" )
#pragma comment( lib, "psapi.lib" )	// GetProcessMemoryInfo()

#pragma comment( lib, "ImGuizmo.lib" )

#include <optional-lite/optional.hpp>	// nonstd::optional<>



#include <Base/Memory/ScratchAllocator.h> // TempAllocator1024

#include <Renderer/renderer_settings.h>
#include <Renderer/scene/SkyModel.h>


//
#include <ProcGen/FastNoiseSupport.h>
#pragma comment( lib, "ProcGen.lib" )

//
#include <Utility/DemoFramework/DemoFramework.h>
#include <Utility/DemoFramework/Camera.h>
#include <Utility/Meshok/BSP.h>
#include <Utility/Meshok/BIH.h>
#include <Utility/Meshok/BVH.h>
#include <Utility/MeshLib/CubeMesh.h>
#include <Utility/Implicit/Evaluator.h>
#include <Utility/VoxelEngine/VoxelTerrainChunk.h>
#include <Utility/VoxelEngine/VoxelTerrainRenderer.h>

#include <Utility/GameFramework/TbGamePlayer.h>

#include <Utility/GUI/ImGui/ImGUI_Renderer.h>
#include <Utility/GUI/ImGui/ImGUI_EditorManipulatorGizmo.h>


#if TB_TEST_LOAD_GLTF_MODEL
	#include <Utility/MeshLib/MeshCompiler.h>
#endif // TB_TEST_LOAD_GLTF_MODEL


#include "TowerDefenseGame-Misc.h"



#include <Voxels/Scene/vx5_octree_world.h>
namespace VX5 {
	class BuildMesh;
}



//
// always needed for game state
#pragma comment( lib, "Physics.lib" )
#if VOXEL_TERRAIN5_WITH_PHYSICS
	#include <Physics/Bullet_Wrapper.h>
	#include <Physics/TbPhysicsWorld.h>
	#include <Physics/Collision/TbQuantizedBIH.h>

	#if VOXEL_TERRAIN5_WITH_PHYSICS_CHARACTER_CONTROLLER
	#include <Physics/Physics_CharacterController.h>
	#endif
#endif

//
#if TB_TEST_USE_SOUND_ENGINE
#include <Sound/SoundSystem.h>
#endif // TB_TEST_USE_SOUND_ENGINE

#include "TowerDefenseGame-game-utils.h"
#include "TowerDefenseGame-Misc.h"
#include "TowerDefenseGame-AI.h"


#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
#include <Planets/PlanetsCommon.h>
#endif // TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE



















struct tbPhysicsSystemFlags
{
	enum Enum
	{
		dbg_pause_physics_simulation = BIT(0),
	};
};
mxDECLARE_FLAGS( tbPhysicsSystemFlags::Enum, U32, tbPhysicsSystemFlagsT );

struct Demo_PhysicsSettings: CStruct
{
	tbPhysicsSystemFlagsT	flags;

	bool	enable_player_collisions;	//!< false if "noclip"
	bool	enable_player_gravity;	//!< is player affected by gravity?

	bool	dbg_draw_voxel_terrain_collisions;

	bool	dbg_draw_bullet_physics;

public:
	mxDECLARE_CLASS(Demo_PhysicsSettings, CStruct);
	mxDECLARE_REFLECTION;
	Demo_PhysicsSettings()
	{
		enable_player_collisions = false;
		enable_player_gravity = false;

		dbg_draw_voxel_terrain_collisions = false;

		dbg_draw_bullet_physics = false;
	}
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






struct DbgLabelledPoint
{
	V3f			position_in_world_space;
	RGBAi		color_rgba;
	String64	text_label;
};

struct DbgLabelledPoints
{
	DynamicArray< DbgLabelledPoint >	labelled_points;

public:
	DbgLabelledPoints( AllocatorI & storage )
		: labelled_points( storage )
	{
	}
};
	


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


///
struct DevVoxelTools: CStruct
{
	VX::SVoxelToolSettings_CSG_Subtract	small_geomod_sphere;	// KEY_F
	VX::SVoxelToolSettings_CSG_Add		small_add_sphere;	// KEY_V

	VX::SVoxelToolSettings_CSG_Subtract	large_geomod_sphere;	// KEY_G
	VX::SVoxelToolSettings_CSG_Add		large_add_sphere;	// KEY_U

	//
	VX::SVoxelToolSettings_CSG_Add		small_add_cube;	// KEY_B
	VX::SVoxelToolSettings_CSG_Subtract	small_geomod_cube;	// KEY_N

	//
	VX::SVoxelToolSettings_SmoothGeometry	smooth;	// KEY_Q
	VX::SVoxelToolSettings_Paint_Brush		paint_material;	// KEY_T

public:
	mxDECLARE_CLASS(DevVoxelTools, CStruct);
	mxDECLARE_REFLECTION;
	DevVoxelTools();

	void resetAllToDefaultSettings(
		const float chunk_length_units_in_world_space
		);
};







struct VoxelEditorTools: CStruct
{
	bool		place_mesh_tool_is_enabled;

	GizmoOperationT	gizmo_operation;

	//
	M44f		last_saved_gizmo_transform_of_mesh;

public:
	mxDECLARE_CLASS(VoxelEditorTools, CStruct);
	mxDECLARE_REFLECTION;
	VoxelEditorTools();

	void resetAllToDefaultSettings(
		const float chunk_length_units_in_world_space
		);
};






struct VX_Dbg_ChunkID: CStruct
{
	UInt3	coords;
	UINT	LoD_index;

public:
	mxDECLARE_CLASS(VX_Dbg_ChunkID, CStruct);
	mxDECLARE_REFLECTION;
	VX_Dbg_ChunkID();

	void setFromChunkID( const VX5::ChunkID& chunk_id )
	{
		coords = UInt3::fromXYZ( chunk_id.toCoords() );
		LoD_index = chunk_id.getLoD();
	}

	VX5::ChunkID asChunkId() const
	{
		return VX5::ChunkID( coords, LoD_index );
	}
};

/// In a restricted (2:1 (corner-)balanced) octree, a node can have up to seven coarser neighbors.
enum VX_Dbg_CoarseNeighborsMaskE
{
	VX_Neighbor0 = BIT(0),
	VX_Neighbor1 = BIT(1),
	VX_Neighbor2 = BIT(2),
	VX_Neighbor3 = BIT(3),

	VX_Neighbor4 = BIT(4),
	VX_Neighbor5 = BIT(5),
	VX_Neighbor6 = BIT(6),

	VX_AllNeighbors = 0x7F,
};
mxDECLARE_FLAGS( VX_Dbg_CoarseNeighborsMaskE, U8, VX_Dbg_CoarseNeighborsMaskT );

///
enum VX_Dbg_OctantsMaskE
{
	VX_Octant0 = BIT(0),
	VX_Octant1 = BIT(1),
	VX_Octant2 = BIT(2),
	VX_Octant3 = BIT(3),

	VX_Octant4 = BIT(4),
	VX_Octant5 = BIT(5),
	VX_Octant6 = BIT(6),
	VX_Octant7 = BIT(7),

	VX_AllOctants = 0xFF,
};
mxDECLARE_FLAGS( VX_Dbg_OctantsMaskE, U8, VX_Dbg_OctantsMaskT );



struct VoxelEngineDebugState: CStruct
{
	//VX_Dbg_ChunkID	debug_chunk_id;

	//
	struct LevelOfDetailStitchingState: CStruct
	{
		VX_Dbg_ChunkID	debug_chunk_id;

		// Coarse cell selection
		VX_Dbg_CoarseNeighborsMaskT	mask_to_show_bounds_only_for_specified_coarse_neighbors;
		bool	show_bounds_for_C0_continuity_only;
		bool	________________padding_for_ImGui_0;

		// Visualize stored coarse cells
		bool	show_coarse_cells_needed_for_C0_continuity;
		bool	show_coarse_cells_needed_for_C1_continuity;
		VX_Dbg_OctantsMaskT		top_level_octants_mask_in_seam_octree;
		bool	________________padding_for_ImGui_1;

	public:
		mxDECLARE_CLASS(LevelOfDetailStitchingState, CStruct);
		mxDECLARE_REFLECTION;
		LevelOfDetailStitchingState();
	};

	//
	struct GhostCellsVisualization: CStruct
	{
		bool show_cached_coarse_cells;
		bool show_ghost_cells;
		bool show_cells_overlapping_maximal_neighbors_mesh;
	public:
		mxDECLARE_CLASS(GhostCellsVisualization, CStruct);
		mxDECLARE_REFLECTION;
		GhostCellsVisualization();
	};

	LevelOfDetailStitchingState	LoD_stitching;
	GhostCellsVisualization		ghost_cells;

	CellVertexTypeF				show_ghost_cells_in_these_neighbors_mask;

public:
	mxDECLARE_CLASS(VoxelEngineDebugState, CStruct);
	mxDECLARE_REFLECTION;
	VoxelEngineDebugState();
};




struct DemoPlanetRenderingSettings: CStruct
{
	bool	enable_normal_mapping;
	bool	dbg_show_tangents;

public:
	mxDECLARE_CLASS(DemoPlanetRenderingSettings, CStruct);
	mxDECLARE_REFLECTION;
	DemoPlanetRenderingSettings();
};




class ImGui_DebugTextRenderer: public ADebugTextRenderer
{
public:
	///
	struct TextLabel
	{
		V3f			position;
		V3f			normal_for_culling;
		float		font_scale;
		RGBAf		color;
		String64	text;
	};
	TArray< TextLabel >	_text_labels;

public:
	ImGui_DebugTextRenderer();

	virtual void drawText(
		const V3f& position,
		const RGBAf& color,
		const V3f* normal_for_culling,
		const float font_scale,	// 1 by default
		const char* fmt, ...
		) override;
};





//
struct TowerDefenseGame
	: Demos::DemoApp
	, public VX5::AClient
	, public VX5::AFrameCallback
	, public efsw::FileWatchListener
	, public TGlobal< TowerDefenseGame >
{
	struct AppSettings: CStruct
	{
bool 	pbr_test_use_deferred;//!< testing & debugging


		//Engine settings
		int	window_offset_x;
		int	window_offset_y;
		RrRuntimeSettings	renderer;
		RrDirectionalLight	sun_light;

		Demo_PhysicsSettings	physics;

		//=== NPC AI

		AI_Settings			ai;


		//=== World Rendering
		bool	dbg_draw_game_world;

		DemoSkyRenderModeT		skyRenderingMode;
		String64				skyboxAssetId;
		AssetID					skydomeAssetId;

		DemoPlanetRenderingSettings	planet;


		//=== Voxel Terrain settings:

		bool	voxels_draw_debug_data;
		VX::DebugView::Options	voxels_debug_viz;

		VX5::RunTimeEngineSettings		voxels;
	
		VX::SVoxelToolSettings_All	voxel_tool;
		float	phys_max_raycast_distance_for_voxel_tool;

		DevVoxelTools			dev_voxel_tools;
		VoxelEngineDebugState	voxel_engine_debug_state;
		VoxelEditorTools		voxel_editor_tools;


		//

		FastNoiseSettings		fast_noise;
		float					fast_noise_result_scale;
		float					fast_noise_frequency_mul;
		//float					fast_noise_result_offset;


		//

		bool	showGUI;	// draw debug HUD/UI?
		bool	draw_fake_camera;	//!< draw 3rd person camera?
		bool	drawGizmo;
		bool	drawCrosshair;

		//bool	drawSky;
		DebugTorchLightParams	player_torchlight_params;
		DebugTorchLightParams	terrain_roi_torchlight_params;

		/// 3rd person view for testing & debugging
		NwView3D	fake_camera;
		float	fake_camera_speed;
		bool	use_fake_camera;	//!< Center the clipmap around the fake camera?

		//float	paint_brush_radius;
		//float	csg_add_sphere_radius;
		//float	csg_subtract_sphere_radius;
		float	csg_diff_sphere_radius;	//!< in chunk units

		float AO_step_size;
		int AO_steps;

		bool calc_vertex_pos_using_QEF;

		int vertexProjSteps;
		float vertexProjLambda;

		bool dbg_viz_SDF;

		bool 	dev_showSceneSettings;//!< debugging
		bool 	dev_showGraphicsDebugger;//!< debugging
		bool 	dev_showRendererSettings;//!< debugging
		bool 	dev_showVoxelEngineSettings;//!< debugging
		bool 	dev_showVoxelEngineDebugMenu;//!< debugging
		bool 	dev_showTweakables;//!< debugging
		bool 	dev_showPerfStats;//!< debugging
		bool 	dev_showMemoryUsage;//!< debugging


		NwWeaponTable	weapon_def;

	public:
		mxDECLARE_CLASS(AppSettings, CStruct);
		mxDECLARE_REFLECTION;
		AppSettings();
	};


	AppSettings			m_settings;

	// Rendering
	TPtr< TbRenderSystemI >		_render_system;
	TPtr< ARenderWorld >		_render_world;
	AxisArrowGeometry	m_gizmo_renderer;
	//SkyModel_ClearSky	m_skyModel_ClearSky;
	Rendering::SkyBoxGeometry	skyBoxGeometry;

	nonstd::optional<CameraMatrices>	_last_camera_matrices;

	ImGui_DebugTextRenderer		_imgui_text_renderer;


	// Voxels
	VX5::OctreeWorld	_voxel_world;

	VoxelTerrainRenderer	_voxel_terrain_renderer;

#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
	Planets::PlanetProcGenData	_planet_procgen_data;
#endif // TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE

	M33f	_test_rotation_matrix;

	FastNoise				_fast_noise;

	//
	EditorManipulator		_editor_manipulator;
	TPtr< RrMeshInstance >	_voxel_editor_model;
	RawTriMesh				_stamped_mesh;







	V3f	_last_pos_where_voxel_tool_raycast_hit_world;
	bool _voxel_tool_raycast_hit_anything;


	//TPtr<NwMaterial>	_fallback_material;

	VX::DebugView		m_dbgView;

	DbgLabelledPoints	_dbg_pts;




	// always needed for GameState
	Physics::TbPhysicsWorld			_physics_world;

#if VOXEL_TERRAIN5_WITH_PHYSICS

#if VOXEL_TERRAIN5_WITH_PHYSICS_CHARACTER_CONTROLLER
	Physics::CharacterController3Step	_player_controller;
#endif

	TbBullePhysicsDebugDrawer		_physics_debug_drawer;

#endif // VOXEL_TERRAIN5_WITH_PHYSICS




	SDF::HeightMap	m_heightMapSampler;

	implicit::BlobTreeSoA	m_blobTree;
	implicit::BlobTreeBuilder	m_blobTreeBuilder;

#if ENABLE_HOT_RELOADING_OF_SCENE_SCRIPT
	efsw::FileWatcher 		m_luaScriptWatcher;
	//std::atomic<bool>
	AtomicInt				m_sceneScriptChanged;
#endif // ENABLE_HOT_RELOADING_OF_SCENE_SCRIPT


	// Scene management
	NwClump			m_runtime_clump;
	FlyingCamera	camera;

	//
	TMovingAverageN< 128 >	m_FPS_counter;


#if TB_TEST_USE_SOUND_ENGINE
	NwSoundSystemI	_sound_engine;
#endif // TB_TEST_USE_SOUND_ENGINE

	TDG_WorldState	_worldState;


	TbGamePlayer	_game_player;

	TPtr< RrMeshInstance >	_test_render_model;


#if TB_TEST_LOAD_GLTF_MODEL
	TPtr< NwMesh >	gltf_mesh;
	TArray< TRefPtr< TcMaterial > > gltf_submesh_materials;
#endif // TB_TEST_LOAD_GLTF_MODEL


public:
	TowerDefenseGame( AllocatorI & allocator );

	virtual ERet Initialize( const EngineSettings& settings ) override;
	virtual void Shutdown() override;
	virtual void Tick( const InputState& inputState ) override;
	virtual ERet Draw( const InputState& inputState ) override;

public:	// INI/Config/Settings

	void setDefaultSettingsAfterEngineInit();

	/// should be called after changing the settings
	void saveSettingsToFiles();

public:
	ERet rebuildBlobTreeFromLuaScript();

	// Input handlers
	void dev_toggle_GUI( GameActionID action, EInputState status, float value )
	{
		m_settings.showGUI ^= 1;
	}
	void dev_toggle_wireframe( GameActionID action, EInputState status, float value )
	{
		m_settings.renderer.drawModelWireframes ^= 1;
	}
	void execute_action( GameActionID action, EInputState status, float value );

	// VX5::AClient
	virtual void VX_onCameraPositionChanged(
		const UInt3 previous_chunk_coord
		, const UInt3 current_chunk_coord
		) override;

	virtual ERet VX_CompileMesh_AnyThread(
		const VX5::ChunkMeshingOutput& chunk_meshing_output
		, const VX::ChunkID& chunk_id
		, NwBlob &dst_data_blob_
	) override;

	virtual ERet VX_loadMesh(
		const VX5::AClient::CreateMeshArgs& _args
		, void *&mesh_
	) override;

	/// Destroys an existing mesh.
	virtual void VX_unloadMesh(
		const VX5::ChunkID& chunk_id,
		void* mesh
	) override;

	virtual ERet VX_generateChunk_AnyThread(
		const VX5::ChunkID& chunk_id
		, const CubeMLd& region_bounds_world
		, VX5::ChunkInfo &metadata_
		, VX5::AChunkDB * DB_voxels
		, AllocatorI & scratchpad
	) override;

	// VX5::AFrameCallback
	virtual void createDebris( const VX5::Debris& debris ) override;


	const V3d getCameraPosition() const
	{
		return V3d::fromXYZ(this->camera.m_eyePosition);
	}

public:
	/// Handles the action file action
	/// @param watchid The watch id for the directory
	/// @param dir The directory
	/// @param filename The filename that was accessed (not full path)
	/// @param action Action that was performed
	/// @param oldFilename The name of the file or directory moved
	virtual void handleFileAction(
		efsw::WatchID watchid,
		const std::string& dir,
		const std::string& filename,
		efsw::Action action,
		std::string oldFilename = ""
		) override;

private:

	ERet initializeResourceLoaders( AllocatorI & allocator );
	void shutdownResourceLoaders( AllocatorI & allocator );

	void _setupLuaForBlobTreeCompiler( const V3f& world_center, VX5::Real world_radius );
	void _bindFunctionsToLua();

	ERet initialize_Renderer( const EngineSettings& engineSettings );
	void shutdown_Renderer();

	void _processInput( const InputState& input_state );

private:
	ERet _generateChunk_for_semiprocedural_Moon(
		const VX5::ChunkID& chunk_id
		, const CubeMLd& region_bounds_world
		, VX5::ChunkInfo &metadata_
		, VX5::AChunkDB * DB_voxels
		, AllocatorI & scratchpad
	);

	ERet _generateChunk_from_Noise_in_App_settings(
		const VX5::ChunkID& chunk_id
		, const CubeMLd& region_bounds_world
		, VX5::ChunkInfo &metadata_
		, VX5::AChunkDB * DB_voxels
		, AllocatorI & scratchpad
	);

	ERet _generateChunk_from_SDF_in_Lua_file(
		const VX5::ChunkID& chunk_id
		, const CubeMLd& region_bounds_world
		, VX5::ChunkInfo &metadata_
		, VX5::AChunkDB * DB_voxels
		, AllocatorI & scratchpad
	);

private:

	ERet _createDebris( const VX5::Debris& debris );

	void _drawSky();

	void _drawCurrentVoxelTool( const VX::SVoxelToolSettings_All& voxel_tool_settings
		, const CameraMatrices& camera_matrices
		, GL::GfxRenderContext & render_context
		, NwClump & storage_clump );

	void _drawVoxelTool_AddShape( const VX::SVoxelToolSettings_CSG_Add& voxel_editor_tool_additive_brush
		, const CameraMatrices& camera_matrices
		, GL::GfxRenderContext & render_context
		, NwClump & storage_clump );

	void _drawVoxelTool_SubtractShape( const VX::SVoxelToolSettings_CSG_Subtract& voxel_tool_subtract_shape
		, const CameraMatrices& camera_matrices
		, GL::GfxRenderContext & render_context
		, NwClump & storage_clump );

	void _drawImGui( const InputState& input_state, const CameraMatrices& camera_matrices );
	void _drawImGui_DebugHUD( const InputState& input_state, const CameraMatrices& camera_matrices );

	void _ImGui_draw_VoxelToolsMenu(
		const InputState& input_state, const CameraMatrices& camera_matrices
		);
	void _ImGui_drawVoxelEngineDebugSettings(
		VoxelEngineDebugState & voxel_engine_debug_state
		, const InputState& input_state, const CameraMatrices& camera_matrices
		);

	void _applyEditingOnLeftMouseButtonClick( const VX::SVoxelToolSettings_All& voxel_tool_settings
		, const InputState& input_state );

private:
	void _gi_debugDraw();

	ERet _gi_bakeAlbedoDensityVolumeTexture( const CameraMatrices& camera_matrices );

private:
	ERet loadOrCreateModel(
		TRefPtr< VX::VoxelizedMesh > &model
		, const char* _sourceMesh
		);
};



void dbg_showChunkOctree();
void dbg_showStitching();
void dbg_showSeamOctreeBounds();
void dbg_remesh_chunk();
void dbg_updateGhostCellsAndRemesh();






inline
UINT calculate_LoD_needed_to_accomodate_planet_of_diameter(
	const double planet_size,
	const double chunk_size_at_most_detailed_LoD
	)
{
	UINT current_LoD = VX5::ChunkID::MIN_LOD;
	double current_world_size = chunk_size_at_most_detailed_LoD;

	while( planet_size > current_world_size )
	{
		++current_LoD;
		current_world_size = ( 1 << current_LoD ) * chunk_size_at_most_detailed_LoD;
	}

	return current_LoD;
}







/// used for saving capacity to file to minimize reallocations on next launch
class ClumpInfo: NonCopyable
{
	typedef THashMap< const TbMetaClass*, U32 >	 InstanceCountByClassID;

	InstanceCountByClassID	_instances_map;

public:
	ClumpInfo( AllocatorI & allocator )
		: _instances_map( allocator )
	{
		//
	}

	ERet buildFromClump( const NwClump& clump );

	ERet saveToFile( const char* filename ) const;
	ERet loadFromFile( const char* filename );
};











#if TEST_VOXEL_TERRAIN_PROCEDURAL_PLANET_MODE
	typedef Vertex_Static VoxelTerrainStandardVertexT;
#else
	typedef Vertex_DLOD VoxelTerrainStandardVertexT;
#endif

typedef DrawVertex	VoxelTerrainTexturedVertexT;





#pragma pack (push,1)

/// for voxel terrain meshes
struct ChunkDataHeader_d
{
	U32		num_meshes;
	U32		pad32;
	AABBf	aabb_local;		//24 bounding box of normalized (in seam space), lies within [0..1]
	// ...followed by meshes.
};
mxSTATIC_ASSERT(sizeof(ChunkDataHeader_d) == 32);

///
struct ChunkMeshHeader_d
{
	U16		is_textured;
	U16		num_textured_submeshes;
	U32		num_vertices;	//4 total number of vertices
	U32		num_indices;	//4 total number of indices
	U32		total_size;
	// ...followed by vertices and indices.
};
mxSTATIC_ASSERT(sizeof(ChunkMeshHeader_d) == 16);

///
struct SubmeshHeader_d
{
	U32	start_index;	//!< Offset of the first index
	U32	index_count;	//!< Number of indices
	U32	texture_id;
	U16	pad[2];
};
mxSTATIC_ASSERT(sizeof(SubmeshHeader_d) == 16);

#pragma pack (pop)



// must be >= 3 for testing seams ("stitching")
const U32 NUM_WORLD_LODS = 2;
//Moon LoD 8 - large pixels in 4096x2048
