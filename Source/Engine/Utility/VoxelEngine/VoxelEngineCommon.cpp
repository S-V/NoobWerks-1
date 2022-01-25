#include "stdafx.h"
#pragma hdrstop

//#include <Input/Input.h>
#include <Engine/WindowsDriver.h>	// InputState

#include <VoxelEngine/VoxelEngineCommon.h>
#include <Utility/Camera/NwFlyingCamera.h>

namespace VX
{

mxDEFINE_CLASS(EditingSettings);
mxBEGIN_REFLECTION(EditingSettings)
	mxMEMBER_FIELD(allow_editing_coarse_LoDs),
mxEND_REFLECTION;
EditingSettings::EditingSettings()
{
	allow_editing_coarse_LoDs = false;
}

mxBEGIN_REFLECT_ENUM( IslandDetectionT )
	mxREFLECT_ENUM_ITEM( None, EIslandDetection::IslandDetect_None ),
	mxREFLECT_ENUM_ITEM( VoxelBased, EIslandDetection::IslandDetect_VoxelBased ),
	mxREFLECT_ENUM_ITEM( MeshBased, EIslandDetection::IslandDetect_MeshBased ),
mxEND_REFLECT_ENUM
mxDEFINE_CLASS(PhysicsSettings);
mxBEGIN_REFLECTION(PhysicsSettings)
	mxMEMBER_FIELD(island_detection),
	mxMEMBER_FIELD(approximate_inertia_tensor_by_bounding_box),
mxEND_REFLECTION;
PhysicsSettings::PhysicsSettings()
{
	island_detection = IslandDetect_None;
	remove_islands_less_than_this_volume = 1.0f;
	approximate_inertia_tensor_by_bounding_box = true;
}

void Dbg_MoveSpectatorCamera( NwFlyingCamera & _camera, const InputState& _inputState )
{
	//if( (_inputState.modifiers & KeyModifier_Ctrl) ) {
	//	if( _inputState.keyboard.held[KEY_R] ) {
	//		_camera.m_linearVelocity = CV3f(0);
	//		_camera.m_linAcceleration = CV3f(0);
	//	}
	//}
	if( _inputState.keyboard.held[KEY_Tab] ) {
		_camera.m_linearVelocity = CV3f(0);
		_camera.m_linAcceleration = CV3f(0);
	}

	// toggle hyperspeed
	setbit_cond(_camera.m_movementFlags,
		_inputState.keyboard.held[KEY_LShift] || _inputState.keyboard.held[KEY_RShift],
		NwFlyingCamera::ACCELERATE);

	if( !_inputState.window.has_focus ) {
		_camera.m_linearVelocity = CV3f(0);
	}
}
void Dbg_ControlTerrainCamera( const NwFlyingCamera& _camera, NwView3D &_terrainCamera, const InputState& _inputState )
{
	if( (_inputState.modifiers & KeyModifier_Ctrl) && _inputState.keyboard.held[KEY_C] ) {
		_terrainCamera.eye_pos = V3f::fromXYZ( _camera._eye_position );
	}
}
void Dbg_ControlTerrainCamera( V3f & _cameraPos, float _cameraSpeed, const InputState& _inputState )
{
	V3f clipmap_center_movement = V3_Zero();

	if( (_inputState.modifiers & KeyModifier_Ctrl) )
	{
		if( _inputState.keyboard.held[KEY_Up] ) {
			clipmap_center_movement.z += _cameraSpeed;
		}
		if( _inputState.keyboard.held[KEY_Down] ) {
			clipmap_center_movement.z -= _cameraSpeed;
		}
	}
	else
	{
		if( _inputState.keyboard.held[KEY_Up] ) {
			clipmap_center_movement.y += _cameraSpeed;
		}
		if( _inputState.keyboard.held[KEY_Down] ) {
			clipmap_center_movement.y -= _cameraSpeed;
		}
		if( _inputState.keyboard.held[KEY_Right] ) {
			clipmap_center_movement.x += _cameraSpeed;
		}
		if( _inputState.keyboard.held[KEY_Left] ) {
			clipmap_center_movement.x -= _cameraSpeed;
		}
		//if( _inputState.keyboard.held[KEY_U] ) {
		//	clipmap_center_movement.z += _cameraSpeed;
		//}
		//if( _inputState.keyboard.held[KEY_L] ) {
		//	clipmap_center_movement.z -= _cameraSpeed;
		//}
	}

	_cameraPos += clipmap_center_movement;
}

}//namespace VX

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
