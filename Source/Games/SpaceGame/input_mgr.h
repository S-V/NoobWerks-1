#pragma once

#include "common.h"
#include <Input/Input.h>


enum EGameAction
{
	// Escape
	eGA_Exit,




	/// Dear ImGui windows
	eGA_Show_Debug_HUD,
	/// Dear ImGui console
	eGA_Show_Debug_Console,


	//
	eGA_DBG_Toggle_Wireframe,
	eGA_DBG_Toggle_Torchlight,
	//eGA_DBG_Toggle_Torchlight,

	eGA_DBG_Save_Game_State,
	eGA_DBG_Load_Game_State,


	//


	eGA_Voxel_CSG_Subtract,
	eGA_Voxel_PlaceBlock,



	eGA_PauseGame,



	///
	eGA_Show_Help,

	///
	eGA_HighlightVisibleShips,

	/// F2
	//eGA_Show_Construction_Center,


	/// F12 - should also try to hinder RenderDoc analysis
	eGA_Take_Screenshot,


	/// Looking
	eGA_Yaw,
	eGA_Pitch,




	/// Movement
	eGA_Move_Left,
	eGA_Move_Right,
	eGA_Move_Forward,
	eGA_Move_Backwards,

	// only in noclip mode
	eGA_Move_Up,
	eGA_Move_Down,

	eGA_Jump,



	/// Main Engine:
	eGA_Spaceship_AccelerateForward,
	eGA_Spaceship_Brake,

	// Reaction Control System Thrusters:
	eGA_Spaceship_YawLeft,
	eGA_Spaceship_YawRight,
	eGA_Spaceship_PitchUp,
	eGA_Spaceship_PitchDown,
	eGA_Spaceship_RollLeft,
	eGA_Spaceship_RollRight,

	//eGA_Spaceship_AccelerateBackwards,
	//eGA_Spaceship_AccelerateLeft,
	//eGA_Spaceship_AccelerateRight,

	//eGA_Spaceship_AccelerateUp,
	//eGA_Spaceship_AccelerateDown,

	eGA_Spaceship_FireCannon,

	//eGA_Spaceship_Debug_DecelerateToStop,

	//eGA_Spaceship_BoostThrust,




	// Weapons
	eGA_SelectPreviousWeapon,
	eGA_SelectNextWeapon,
	eGA_ReloadWeapon,

	eGA_SelectWeapon1,
	eGA_SelectWeapon2,
	eGA_SelectWeapon3,
	eGA_SelectWeapon4,
	eGA_SelectWeapon5,

	//
	eGA_DebugTerrainCamera_Move_Left,
	eGA_DebugTerrainCamera_Move_Right,
	eGA_DebugTerrainCamera_Move_Forward,
	eGA_DebugTerrainCamera_Move_Backwards,
	eGA_DebugTerrainCamera_Move_Up,
	eGA_DebugTerrainCamera_Move_Down,

	//
	eGA_Editor_StartOperation,
	eGA_Editor_ApplyOperation,
	eGA_Editor_UndoOperation,
};

///
class SgInputMgr
	: TSingleInstance< SgInputMgr >
{
public_internal:
	TPtr< NwInputSystemI >	_input_system;


	TPtr< gainput::InputMap >		_input_map;

	TPtr< gainput::InputDeviceKeyboard >	_keyboard;
	TPtr< gainput::InputDeviceMouse >		_mouse;

	AllocatorI& _allocator;

public:
	SgInputMgr(AllocatorI& allocator);

	ERet initialize();
	void shutdown();

	void UpdateOncePerFrame();

	bool GetBool(EGameAction game_action) const;

	bool wasButtonDown( EGameAction game_action ) const;
};


void FlyingCamera_handleInput_for_FirstPerson(
	struct NwFlyingCamera & camera
	, const NwTime& game_time
	, const gainput::InputMap& input_map
	, const gainput::InputState& keyboard_input_state
	, const SgCheatSettings& game_cheat_settings
	, const MyGameControlsSettings& controls_settings
	);
