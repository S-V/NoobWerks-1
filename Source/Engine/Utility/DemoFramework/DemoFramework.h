#pragma once

#include <Core/Event.h>
#include <Core/ObjectModel/Clump.h>
#include <Rendering/Public/Globals.h>
#include <Engine/Engine.h>
#include <Utility/MeshLib/TriMesh.h>
#include <Utility/GameFramework/NwGameStateManager.h>
#include <Developer/EditorSupport/DevAssetFolder.h>

struct VoxelTerrainMaterials;

namespace Demos
{

///
struct DemoApp
{
	DevAssetFolder	_asset_folder;

protected:
	DemoApp();
	virtual ~DemoApp();

public:
	ERet Initialize( const NEngine::LaunchConfig & engine_settings );
	void Shutdown();

	void Tick();

	// Input bindings
	virtual void attack1( GameActionID action, EInputState status, float value );
	virtual void attack2( GameActionID action, EInputState status, float value );
	virtual void attack3( GameActionID action, EInputState status, float value );
	virtual void take_screenshot( GameActionID action, EInputState status, float value );
	virtual void dev_toggle_console( GameActionID action, EInputState status, float value );
	virtual void dev_toggle_menu( GameActionID action, EInputState status, float value );
	virtual void dev_reset_state( GameActionID action, EInputState status, float value );
	virtual void dev_reload_shaders( GameActionID action, EInputState status, float value );
	virtual void dev_toggle_raytrace( GameActionID action, EInputState status, float value );
	virtual void dev_save_state( GameActionID action, EInputState status, float value );
	virtual void dev_load_state( GameActionID action, EInputState status, float value );
	virtual void dev_do_action1( GameActionID action, EInputState status, float value );
	virtual void dev_do_action2( GameActionID action, EInputState status, float value );
	virtual void dev_do_action3( GameActionID action, EInputState status, float value );
	virtual void dev_do_action4( GameActionID action, EInputState status, float value );
	virtual void request_exit( GameActionID action, EInputState status, float value );

	/// this should happen even if we don't have focus
	void dev_ReloadChangedAssetsIfNeeded();
};

}//namespace Demos

namespace Utilities
{

/// preallocate arrays of meshes, etc.
ERet preallocateObjectLists( NwClump & scene_data );

//ERet CreatePlane(
//	MeshInstance *&_model,
//	NwClump * _storage
//);

}//namespace Utilities

namespace Meshok
{

ERet ComputeVertexNormals( const TriMesh& mesh, DrawVertex *vertices, AllocatorI & scratch );

}//namespace Meshok
