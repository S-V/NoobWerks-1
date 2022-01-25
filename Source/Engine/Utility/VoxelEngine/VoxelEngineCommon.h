// ChunkID.
#pragma once

#include <Rendering/Public/Core/Mesh.h>	// MeshBuffers
#include <Rendering/Public/Globals.h>
#include <Utility/Meshok/Volumes.h>	// MASK_POS_X, etc.

namespace VX
{



///
struct EditingSettings: CStruct
{
	/// By default, only the finest Level-of-Detail chunks can be edited.
	/// Set this flag to 'true' if you also want modify coarser LoDs (they will be rebuilt on-the-fly).
	bool	allow_editing_coarse_LoDs;	// (default = false)

public:
	mxDECLARE_CLASS(EditingSettings, CStruct);
	mxDECLARE_REFLECTION;
	EditingSettings();
};

/// disconnected, "floating" islands (for debris)
enum EIslandDetection
{
	IslandDetect_None,
	IslandDetect_VoxelBased,	// CCL
	IslandDetect_MeshBased,
};
mxDECLARE_ENUM( EIslandDetection, int, IslandDetectionT );

///
struct PhysicsSettings: CStruct
{
	IslandDetectionT	island_detection;

	/// remove small chunks instead of physicalizing them?
	float	remove_islands_less_than_this_volume;

	///
	bool	approximate_inertia_tensor_by_bounding_box;

public:
	mxDECLARE_CLASS(PhysicsSettings, CStruct);
	mxDECLARE_REFLECTION;
	PhysicsSettings();
};

}//namespace VX

// Driver.h
struct InputState;

class NwFlyingCamera;

namespace VX
{
	void Dbg_MoveSpectatorCamera( NwFlyingCamera & _camera, const InputState& input_state );
	void Dbg_ControlTerrainCamera( const NwFlyingCamera& _camera, NwView3D &_terrainCamera, const InputState& _inputState );
	/// for debugging LoD
	void Dbg_ControlTerrainCamera( V3f & _cameraPos, float _cameraSpeed, const InputState& _inputState );
}//namespace VX

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
