#include <complex>

#include <Lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <Base/Base.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Template/Containers/BitSet/BitArray.h>
#include <Base/Text/String.h>
#include <Base/Math/ViewFrustum.h>

#include <bx/bx.h>	// uint8_t

//#include <libnoise/noise.h>
//#pragma comment( lib, "libnoise.lib" )

#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Graphics/private/d3d_common.h>
#include <Driver/Driver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/TxTSupport/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Renderer/Core/Material.h>
#include <Renderer/private/RenderSystem.h>
#include <Renderer/Shaders/__shader_globals_common.h>	// G_VoxelTerrain

#include <Utility/Meshok/SDF.h>
#include <Utility/Meshok/BSP.h>
#include <Utility/Meshok/BIH.h>
#include <Utility/Meshok/Noise.h>
#include <Utility/Meshok/Volumes.h>
#include <Utility/MeshLib/Simplification.h>
#include <Utility/Meshok/Octree.h>
#include <Utility/TxTSupport/TxTConfig.h>
#include <Utility/VoxelEngine/VoxelizedMesh.h>

#include <DirectXMath/DirectXMath.h>

#include "TowerDefenseGame.h"
#include <Voxels/Data/vx5_chunk_data.h>
#include <Voxels/Base/Math/vx5_Quadrics.h>
#include <Voxels/Common/vx5_debug.h>

#include <Scripting/Scripting.h>
#include <Scripting/FunctionBinding.h>
// Lua G(L) crap
#undef G

//const char* PATH_TO_LUA_SCRIPTS = "R:/testbed/Assets/scripts";

const char* LUA_SCENE_SCRIPT_FILE_NAME = "make_scene.lua";
const char* ALTERNATIVE_LUA_SCENE_SCRIPT_FILE = "R:/testbed/Assets/scripts/make_scene.lua";

void TowerDefenseGame::_setupLuaForBlobTreeCompiler( const V3f& world_center, VX5::Real world_radius )
{
	lua_State* L = Scripting::GetLuaState();

	//
	lua_pushinteger(L, VoxMat_None);
	lua_setglobal(L, "mat_air");

	lua_pushinteger(L, VoxMat_RockA);
	lua_setglobal(L, "mat_default");

	//
	lua_pushinteger(L, VoxMat_Ground);
	lua_setglobal(L, "mat_ground");

	lua_pushinteger(L, VoxMat_RockA);
	lua_setglobal(L, "mat_rocka");

	lua_pushinteger(L, VoxMat_RockB);
	lua_setglobal(L, "mat_rockb");

	//lua_pushinteger(L, VoxMat_RockC);
	//lua_setglobal(L, "mat_rockc");

	lua_pushinteger(L, VoxMat_Grass);
	lua_setglobal(L, "mat_grass");

	lua_pushinteger(L, VoxMat_GrassB);
	lua_setglobal(L, "mat_grassb");

	lua_pushinteger(L, VoxMat_Floor);
	lua_setglobal(L, "mat_floor");

	lua_pushinteger(L, VoxMat_Wall);
	lua_setglobal(L, "mat_wall");

	lua_pushinteger(L, VoxMat_Snow);
	lua_setglobal(L, "mat_snow");

	lua_pushinteger(L, VoxMat_Snow2);
	lua_setglobal(L, "mat_snow2");

	lua_pushinteger(L, VoxMat_Mirror);
	lua_setglobal(L, "mat_mirror");

	//
	lua_pushnumber(L, world_center.x);
	lua_setglobal(L, "WORLD_CENTER_X");

	lua_pushnumber(L, world_center.y);
	lua_setglobal(L, "WORLD_CENTER_Y");

	lua_pushnumber(L, world_center.z);
	lua_setglobal(L, "WORLD_CENTER_Z");

	//luabridge::LuaRef n = luabridge::getGlobal(L, "mat_default");
	//int num = n.cast<int>();

	lua_pushnumber(L, world_radius);
	lua_setglobal(L, "WORLD_RADIUS");

	m_blobTreeBuilder.bindToLua(L);
}

ERet TowerDefenseGame::rebuildBlobTreeFromLuaScript()
{
	ptPRINT("Rebuilding the scene tree...");

	AllocatorI & scratch = MemoryHeaps::temporary();
mxTODO("recycle old mem")
	//m_blobTreeBuilder._node_pool.Release();

	{
		NwBlob	script(scratch);

		if(mxFAILED(NwBlob_::loadBlobFromFile( script, LUA_SCENE_SCRIPT_FILE_NAME )))
		{
			mxDO(NwBlob_::loadBlobFromFile( script, ALTERNATIVE_LUA_SCENE_SCRIPT_FILE ));
		}

		Scripting::ExecuteBuffer(script.raw(), script.rawSize());
	}

	lua_State* L = Scripting::GetLuaState();

	/* the function name */
	lua_getglobal(L, "getRoot");

	/* the first argument */
	//lua_pushnumber(L, x);

	/* call the function with 0 arguments, return 1 result */
	lua_call(L, 0, 1);

	/* get the result */
	implicit::Node* root = (implicit::Node*) lua_touserdata(L, -1);
	lua_pop(L, 1);

	implicit::BlobTreeCompiler	compiler(scratch);
	mxTRY(compiler.compile( root, m_blobTree ));

	return ALL_OK;
}

void TowerDefenseGame::execute_action( GameActionID action, EInputState status, float value )
{
}

void FUN_dbgshow(void)
{
	TowerDefenseGame& app = TowerDefenseGame::Get();
	using namespace VX5;
	using namespace VX;

	const OctreeWorld& world = app._voxel_world;

	const int iLoD = 0;
	const VX::ChunkID chunk_id = world._octree.getContainingChunkId( app.getCameraPosition(), iLoD );
	const VX5::ChunkNode* chunk = (VX5::ChunkNode*) world._octree.findChunkByID( chunk_id );
	if(chunk) {
		VoxelTerrainChunk* terrChunk = (VoxelTerrainChunk*) chunk->mesh;
		//String256	adjacencyMaskStr;
		//Dbg_FlagsToString( &terrChunk->nodeAdjacency, GetTypeOf_CellVertexTypeF(), adjacencyMaskStr );
		LogStream(LL_Info) << "Dumping info for: Chunk " << chunk_id;
	}else{
		LogStream(LL_Warn) << "Chunk " << chunk_id << " is not loaded";
	}

#if DEBUG_USE_SERIAL_JOB_SYSTEM
const AABBf chunkBoundsNoMargin = AABBf::fromOther(world._octree.getNodeAABB( chunk_id ));
const AABBf chunkBoundsWithMargin = AABBf::fromOther(world.getBBoxWorldWithMargins( chunk_id ));
app.m_dbgView.addBox(chunkBoundsNoMargin, RGBAf::BLACK);
app.m_dbgView.addBox(chunkBoundsWithMargin, RGBAf::ORANGE);

const M44f	chunkMeshTransform = M44_BuildTS( chunkBoundsWithMargin.min_corner, chunkBoundsWithMargin.size().x );
DebugViewProxy	dbgViewProxy( &app.m_dbgView, chunkMeshTransform );

#else
ADebugView & dbgViewProxy = ADebugView::ms_dummy;
#endif

	AllocatorI & scratch = MemoryHeaps::temporary();

	ChunkData		voxelData( scratch );
	voxelData.loadFromDatabase( chunk_id, world.voxelDB );

	voxelData.Debug_Show_Voxels(dbgViewProxy);
}

static void dbg()
{
	//
}

static void FUN_dbg(void)
{
	TowerDefenseGame& app = TowerDefenseGame::Get();
	using namespace VX5;
	using namespace VX;

	const OctreeWorld& world = app._voxel_world;


#if 0
	const int iLoD = 0;
	const VX::ChunkID chunk_id = world._octree.getContainingChunkId( app.camera.m_eyePosition, iLoD );

const AABBf chunkBoundsWithMargin = world.getBBoxWorldWithMargins( chunk_id );
const AABBf chunkBoundsNoMargin = world._octree.getBBoxWorld( chunk_id );

#if DEBUG_USE_SERIAL_JOB_SYSTEM
	DebugViewProxy	dbgViewProxy( &app.m_dbgView, g_MM_Identity );

	const M44f	chunkMeshTransform = M44_BuildTS( chunkBoundsWithMargin.mins, chunkBoundsWithMargin.size().x );
	DebugViewProxy	dbgViewProxyForChunkMesh( &app.m_dbgView, chunkMeshTransform );
	ADebugView &dbgViewProxyForContouring = dbgViewProxyForChunkMesh;
#else
	ADebugView & dbgViewProxy = ADebugView::ms_dummy;
	ADebugView &dbgViewProxyForContouring = ADebugView::ms_dummy;
#endif


dbgViewProxy.addBox(chunkBoundsNoMargin, RGBAf::BLACK);
//dbgViewProxy.addBox(chunkBoundsWithMargin, RGBAf::GREEN);
#endif



#if 0&DEBUG_USE_SERIAL_JOB_SYSTEM
	AllocatorI & scratchpad = MemoryHeaps::temporary();

	const SDF::Isosurface& s_terrain = GetTestIso(app);

	ChunkData		voxelData( scratchpad );
	ChunkGenStats	buildStats;
	voxelData.generateFromIsosurface( &s_terrain, chunkBoundsWithMargin, CHUNK_CELLS_DIM, scratchpad, &buildStats, dbgViewProxy );

	if(buildStats.num_active_cells)
	{
		Meshok::TriMesh	chunkMesh( scratchpad );	// 'fine' mesh
		voxelData.Contour( chunkMesh, true, scratchpad, dbgViewProxyForContouring );

		const UINT resolutionAtNextLoD = CHUNK_CELLS_DIM / 2u;

		ChunkData	coarseVoxelData( scratchpad );	//!< voxel data at the next LoD
		voxelData.generateFromIsosurface( &s_terrain, chunkBoundsWithMargin, resolutionAtNextLoD, scratchpad );

		Meshok::TriMesh	coarseMesh( scratchpad );	// 'coarse' mesh
		voxelData.Contour( coarseMesh, false, scratchpad );

		for( UINT iVertex = 0; iVertex < chunkMesh.vertices.num(); iVertex++ )
		{
			const Meshok::Vertex& fineVertex = chunkMesh.vertices[ iVertex ];
			const Morton32 fineCellAddress = fineVertex.tag;
			// find the coarse cell which corresponds to the fine cell.
			const Morton32 coarseCellAddress = (fineCellAddress >> 3u);

	Int4 fineAddr = Morton32_decodeCell(fineCellAddress);
	Int4 coarseAddr = Morton32_decodeCell(coarseCellAddress);

			const U32 coarseVertexIndex = Find_Cell_Vertex( coarseMesh, coarseCellAddress );

			//dst_vertex.miscFlags = GetVertexType( fineCellAddress );
			if( coarseVertexIndex != ~0 ) {
				V3f start = M44_TransformPoint(chunkMeshTransform, fineVertex.xyz);
				V3f end = M44_TransformPoint(chunkMeshTransform, coarseMesh.vertices[ coarseVertexIndex ].xyz);
				dbgViewProxy.addLine( start, end, RGBAf::YELLOW );
				dbgViewProxy.addPoint( end, RGBAf::ORANGE, 1.0f );
			} else {
				V3f start = M44_TransformPoint(chunkMeshTransform, fineVertex.xyz);
				dbgViewProxy.addPoint( start, RGBAf::BLACK, 1.0f );
			}
		}
	}
#endif
}

void FUN_remesh_curr()
{
	TowerDefenseGame& app = TowerDefenseGame::Get();
	app._voxel_world.dbg_RemeshChunk( app.getCameraPosition() );
}

void FUN_seam()
{
	TowerDefenseGame& app = TowerDefenseGame::Get();

	VX::ChunkID	chunk_id;
	VX5::ChunkNode* chunk = (VX5::ChunkNode*) app._voxel_world._octree.findEnclosingChunk( app.getCameraPosition(), chunk_id );
	if( chunk ) {
		const VX5::SeamInfo seam = app._voxel_world.computeSeamInfo_AnyThread( chunk_id );
		//LogStream(LL_Info) << "Chunk " << chunk_id << ", octant: " << chunk_id.octantInParent() << ", neighbor mask: " << seam.coarseNeighbors << ", old mask: " << chunk->seam.coarseNeighbors;
	}
}

void dbg_check_ghost_cells_are_synced_with_neighbors()
{
	TowerDefenseGame& app = TowerDefenseGame::Get();
	VX5::dbg_check_ghost_cells_are_synced_with_neighbors( app.getCameraPosition(), app._voxel_world, app.m_dbgView );
}


void dbg_print_chunk_info()
{
	TowerDefenseGame& app = TowerDefenseGame::Get();
	VX5::dbg_print_chunk_info( app.getCameraPosition(), app._voxel_world );
}

void dbg_remesh_chunk()
{
	TowerDefenseGame& app = TowerDefenseGame::Get();
	VX5::dbg_remesh_chunk( app.getCameraPosition(), app._voxel_world );
}

void dbg_showSeamOctreeBounds()
{
	TowerDefenseGame& app = TowerDefenseGame::Get();
	VX5::dbg_showSeamOctreeBounds_for_Chunk_at_Point( app.getCameraPosition(), app._voxel_world );
}
void dbg_showChunkOctree()
{
	TowerDefenseGame& app = TowerDefenseGame::Get();
	VX5::dbg_showChunkOctree( app.getCameraPosition(), app._voxel_world );
}


void dbg_updateGhostCellsAndRemesh()
{
	TowerDefenseGame& app = TowerDefenseGame::Get();
	VX5::dbg_updateGhostCellsAndRemesh( app.getCameraPosition(), app._voxel_world );
}

void dbg_contourAndSaveChunk()
{
	TowerDefenseGame& app = TowerDefenseGame::Get();
	VX5::dbg_contourAndSaveChunk( app.getCameraPosition(), app._voxel_world, "R:/mesh.obj", MemoryHeaps::temporary(), app.m_dbgView );
}

void TowerDefenseGame::_bindFunctionsToLua()
{
	lua_State* L = Scripting::GetLuaState();
	lua_register( L, "dbgshow", TO_LUA_CALLBACK(FUN_dbgshow) );
	lua_register( L, "dbg", TO_LUA_CALLBACK(FUN_dbg) );
	//lua_register( L, "remesh", TO_LUA_CALLBACK(FUN_remesh) );
	lua_register( L, "cur", TO_LUA_CALLBACK(FUN_remesh_curr) );
	lua_register( L, "ssob", TO_LUA_CALLBACK(dbg_showSeamOctreeBounds) );
	lua_register( L, "so", TO_LUA_CALLBACK(dbg_showChunkOctree) );
	lua_register( L, "seam", TO_LUA_CALLBACK(FUN_seam) );
	lua_register( L, "cgc", TO_LUA_CALLBACK(dbg_check_ghost_cells_are_synced_with_neighbors) );
	lua_register( L, "inf", TO_LUA_CALLBACK(dbg_print_chunk_info) );
	lua_register( L, "rm", TO_LUA_CALLBACK(dbg_remesh_chunk) );

	lua_register( L, "rmsf", TO_LUA_CALLBACK(dbg_contourAndSaveChunk) );
}
