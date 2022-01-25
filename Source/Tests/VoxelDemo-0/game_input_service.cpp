#include "stdafx.h"
#pragma hdrstop

#include <gainput/gainput.h>
#include <Engine/WindowsDriver.h>
#include <Utility/Camera/NwFlyingCamera.h>	// NwFlyingCamera

#include "app_settings.h"
#include "game_input_service.h"


ERet GameInputService::Initialize( AllocatorI& allocator )
{
	mxDO(NwInputSystemI::create(
		&_input_system._ptr
		, allocator
		));

	mxDO(_input_system->Initialize(
		WindowsDriver::getMainWindow()
		));

	//
	gainput::InputManager& input_manager = _input_system->getInputManager();

	void* mem = _input_map_storage.getMemoryForObject< gainput::InputMap >();
	_input_map = new( mem ) gainput::InputMap( input_manager );

	//
	_keyboard = checked_cast< gainput::InputDeviceKeyboard* >(
		input_manager.GetDevice( InputDevice_Keyboard )
		);
	_mouse = checked_cast< gainput::InputDeviceMouse* >(
		input_manager.GetDevice( InputDevice_Mouse )
		);

	//
	{
		const U32 keyboard_device_id = InputDevice_Keyboard;


		_input_map->MapBool(
			eGA_DBG_Toggle_SDF_Visualization
			, keyboard_device_id
			, gainput::KeyT
			);



		//
		_input_map->MapBool(
			eGA_Exit
			, keyboard_device_id
			, gainput::KeyEscape
			);


		


		//
		_input_map->MapBool(
			eGA_DBG_Toggle_Wireframe
			, keyboard_device_id
			, gainput::KeyF2
			);
		//
		_input_map->MapBool(
			eGA_DBG_Toggle_Torchlight
			, keyboard_device_id
			, gainput::KeyF3
			);
		_input_map->MapBool(
			eGA_DBG_Toggle_LOD_Octree_Display
			, keyboard_device_id
			, gainput::KeyE
			);

		//
		_input_map->MapBool(
			eGA_DBG_Clear_Debug_Lines
			, keyboard_device_id
			, gainput::KeyX
			);
		////
		//_input_map->MapBool(
		//	eGA_DBG_ShowChunkFineCells
		//	, keyboard_device_id
		//	, gainput::KeyG
		//	);



		//
		_input_map->MapBool(
			eGA_DBG_Save_Game_State
			, keyboard_device_id
			, gainput::KeyF5
			);
		//
		_input_map->MapBool(
			eGA_DBG_Load_Game_State
			, keyboard_device_id
			, gainput::KeyF9
			);



		//
		
		_input_map->MapBool(
			eGA_Voxel_CSG_Subtract
			, keyboard_device_id
			, gainput::KeyF
			);
		_input_map->MapBool(
			eGA_Voxel_CSG_Add
			, keyboard_device_id
			, gainput::KeyV
			);

		_input_map->MapBool(
			eGA_Voxel_PlaceBlock
			, keyboard_device_id
			, gainput::KeyU
			);




		// keyboard_input_state.GetBool( gainput::KeyMediaPlayPause ) ?
		_input_map->MapBool(
			eGA_PauseGame
			, keyboard_device_id
			, gainput::KeyBreak
			);



		////
		//_input_map->MapBool(
		//	eGA_Show_Help
		//	, keyboard_device_id
		//	, gainput::KeyF1
		//	);


		//
		_input_map->MapBool(
			eGA_Show_Debug_HUD
			, keyboard_device_id
			, gainput::KeyF1
			);

		//
		_input_map->MapBool(
			eGA_Show_Debug_Console
			, keyboard_device_id
			
			mxBUG("~ doesn't work, so we use Tab")
			, gainput::KeyTab
			//, gainput::KeyGrave
			);


		////
		//_input_map->MapBool(
		//	eGA_Show_Construction_Center
		//	, keyboard_device_id
		//	, gainput::KeyF2
		//	);

		//
		_input_map->MapBool(
			eGA_Take_Screenshot
			, keyboard_device_id
			, gainput::KeyF12
			);









		//
		_input_map->MapBool(
			eGA_Move_Left
			, keyboard_device_id
			, gainput::KeyA
			);
		//
		_input_map->MapBool(
			eGA_Move_Right
			, keyboard_device_id
			, gainput::KeyD
			);
		//
		_input_map->MapBool(
			eGA_Move_Forward
			, keyboard_device_id
			, gainput::KeyW
			);
		//
		_input_map->MapBool(
			eGA_Move_Backwards
			, keyboard_device_id
			, gainput::KeyS
			);
		//
		_input_map->MapBool(
			eGA_Move_Up
			, keyboard_device_id
			, gainput::KeySpace
			);
		//
		_input_map->MapBool(
			eGA_Move_Down
			, keyboard_device_id
			, gainput::KeyC
			);

		//
		_input_map->MapBool(
			eGA_Jump
			, keyboard_device_id
			, gainput::KeySpace
			);
	}
	
	// arrows
#if 0
	{
		//
		_input_map->MapBool(
			eGA_Move_Left
			, InputDevice_Keyboard
			, gainput::KeyLeft
			);
		//
		_input_map->MapBool(
			eGA_Move_Right
			, InputDevice_Keyboard
			, gainput::KeyRight
			);
		//
		_input_map->MapBool(
			eGA_Move_Forward
			, InputDevice_Keyboard
			, gainput::KeyUp
			);
		//
		_input_map->MapBool(
			eGA_Move_Backwards
			, InputDevice_Keyboard
			, gainput::KeyDown
			);
	}
#endif


	{
		//
		_input_map->MapBool(
			eGA_SelectPreviousWeapon
			, InputDevice_Keyboard
			, gainput::KeyBracketLeft
			);
		//
		_input_map->MapBool(
			eGA_SelectNextWeapon
			, InputDevice_Keyboard
			, gainput::KeyBracketRight
			);
		//
		_input_map->MapBool(
			eGA_ReloadWeapon
			, InputDevice_Keyboard
			, gainput::KeyR
			);

		//
		_input_map->MapBool( eGA_SelectWeapon1, InputDevice_Keyboard, gainput::Key1 );
		_input_map->MapBool( eGA_SelectWeapon2, InputDevice_Keyboard, gainput::Key2 );
		_input_map->MapBool( eGA_SelectWeapon3, InputDevice_Keyboard, gainput::Key3 );
		_input_map->MapBool( eGA_SelectWeapon4, InputDevice_Keyboard, gainput::Key4 );
		_input_map->MapBool( eGA_SelectWeapon5, InputDevice_Keyboard, gainput::Key5 );
	}


	//
	{
		_input_map->MapFloat(
			eGA_Yaw
			, InputDevice_Mouse
			, gainput::MouseAxisX
			);

		//

		_input_map->MapFloat(
			eGA_Pitch
			, InputDevice_Mouse
			, gainput::MouseAxisY
			);
	}

	{
		//
		_input_map->MapBool(
			eGA_DebugTerrainCamera_Move_Left
			, InputDevice_Keyboard
			, gainput::KeyLeft
			);
		//
		_input_map->MapBool(
			eGA_DebugTerrainCamera_Move_Right
			, InputDevice_Keyboard
			, gainput::KeyRight
			);
		//
		_input_map->MapBool(
			eGA_DebugTerrainCamera_Move_Forward
			, InputDevice_Keyboard
			, gainput::KeyUp
			);
		//
		_input_map->MapBool(
			eGA_DebugTerrainCamera_Move_Backwards
			, InputDevice_Keyboard
			, gainput::KeyDown
			);
		//
		_input_map->MapBool(
			eGA_DebugTerrainCamera_Move_Up
			, InputDevice_Keyboard
			, gainput::KeyPageUp
			);
		//
		_input_map->MapBool(
			eGA_DebugTerrainCamera_Move_Down
			, InputDevice_Keyboard
			, gainput::KeyPageDown
			);
	}

	{
		//
		_input_map->MapBool(
			eGA_Editor_StartOperation
			, InputDevice_Keyboard
			, gainput::KeySpace
			);
		//
		_input_map->MapBool(
			eGA_Editor_ApplyOperation
			, InputDevice_Keyboard
			, gainput::KeyReturn
			);
		//
		_input_map->MapBool(
			eGA_Editor_UndoOperation
			, InputDevice_Keyboard
			, gainput::KeyZ
			);
	}

	return ALL_OK;
}

void GameInputService::Shutdown()
{
	_keyboard = nil;
	_mouse = nil;

	_input_map.Destruct();

	_input_system->Shutdown();
	NwInputSystemI::destroy( &_input_system._ptr );
}

void GameInputService::UpdateOncePerFrame()
{
	_input_system->UpdateOncePerFrame();
}

bool GameInputService::wasButtonDown( EGameAction game_action ) const
{
	return _input_map->GetBoolWasDown( game_action );
}


void FlyingCamera_handleInput_for_FirstPerson(
	NwFlyingCamera & camera
	, const NwTime& game_time
	, const gainput::InputMap& input_map
	, const gainput::InputState& keyboard_input_state
	//, const GameCheatsSettings& game_cheat_settings
	, const MyGameControlsSettings& controls_settings
	)
{
	//
	const bool is_CTRL_key_held = (
		keyboard_input_state.GetBool( gainput::KeyCtrlL )
		||
		keyboard_input_state.GetBool( gainput::KeyCtrlR )
		);

	const bool is_shift_key_held = (
		keyboard_input_state.GetBool( gainput::KeyShiftL )
		||
		keyboard_input_state.GetBool( gainput::KeyShiftR )
		);

	//
	const bool move_camera_with_keys
		= 1// game_cheat_settings.enable_noclip

		// When physics is enabled and noclip is disabled,
		// player movement is controlled via Physics Character Controller.
		|| !GAME_CFG_WITH_PHYSICS
		;

	{
		//
		setbit_cond(
			camera.m_movementFlags,
			input_map.GetBool( eGA_Move_Forward ),
			NwFlyingCamera::FORWARD
			);

		setbit_cond(
			camera.m_movementFlags,
			input_map.GetBool( eGA_Move_Left ),
			NwFlyingCamera::LEFT
			);

		setbit_cond(
			camera.m_movementFlags,
			input_map.GetBool( eGA_Move_Right ),
			NwFlyingCamera::RIGHT
			);

		setbit_cond(
			camera.m_movementFlags,
			input_map.GetBool( eGA_Move_Backwards ),
			NwFlyingCamera::BACKWARD
			);

		//
		//if(game_cheat_settings.enable_noclip)
		{
			setbit_cond(
				camera.m_movementFlags,
				input_map.GetBool( eGA_Move_Up ),
				NwFlyingCamera::UP
				);

			setbit_cond(
				camera.m_movementFlags,
				input_map.GetBool( eGA_Move_Down ),
				NwFlyingCamera::DOWN
				);
		}

		setbit_cond(
			camera.m_movementFlags,
			is_shift_key_held,
			NwFlyingCamera::ACCELERATE
			);
	}


	//
	//const float yaw_delta = input_map.GetFloatDelta( eGA_Yaw );
	//const float pitch_delta = input_map.GetFloatDelta( eGA_Pitch );

	const InputState& input_state = NwInputSystemI::Get().getState();
	float yaw_delta = input_state.mouse.delta_x * controls_settings.mouse_sensitivity;
	float pitch_delta = input_state.mouse.delta_y * controls_settings.mouse_sensitivity;

	//ptWARN("yaw_delta = %f, pitch_delta = %f", yaw_delta, pitch_delta);

	if(controls_settings.invert_mouse_x) {
		yaw_delta *= -1;
	}
	if(controls_settings.invert_mouse_y) {
		pitch_delta *= -1;
	}

	camera.Yaw( yaw_delta );
	camera.Pitch( pitch_delta );
}
