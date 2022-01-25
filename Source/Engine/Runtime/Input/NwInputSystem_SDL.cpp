// SDL
#include <SDL2/include/SDL.h>
#include <SDL2/include/SDL_syswm.h>

// gainput
#include <gainput/gainput.h>

#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>
#include <Input/Input.h>



#if 0
	#define DBG_MSG(...)\
		DBGOUT(__VA_ARGS__)
#else
	#define DBG_MSG(...)
#endif



class NwInputSystem_SDL: public NwInputSystemI
{
	InputState				_input_state;

	// gainput
	gainput::InputManager	input_manager;

	// 
	//bool		pins_mouse_cursor_to_center;

	/// maps SDL virtual codes to our keycodes
	KeyCodeT	_sdl_scancode_to_my_keycode[ SDL_NUM_SCANCODES ];

	////
	//EKeyCode		gainput_key_to_my_keycode[ gainput::KeyCount_ ];
	//EMouseButton	gainput_mouse_button_to_mine[ gainput::MouseButtonCount_ ];

public_internal:
	AllocatorI	&	_allocator;

public:
	NwInputSystem_SDL( AllocatorI& allocator )
		: _allocator( allocator )
	{
		//
	}

	virtual ERet Initialize( NwOpaqueWindow* window ) override
	{
		DBGOUT("MAX_KEYS = %d", MAX_KEYS);

		mxASSERT_PTR(window);

		mxENSURE(
			SDL_WasInit(SDL_INIT_VIDEO),
			ERR_INVALID_FUNCTION_CALL,
			"Must init SDL first and create a window!"
			);

		mxZERO_OUT(_input_state);

		_initSDLScancodeToMyKeycodeTable();

		//
		SDL_Window* sdl_window = (SDL_Window*) window;

		SDL_GetWindowSize( sdl_window,
			&_input_state.window.width, &_input_state.window.height
			);

		//
		SDL_SetWindowsMessageHook( &My_SDL_WindowsMessageHook, this );

		//
		gainput::DeviceId id = input_manager
			.CreateDevice< gainput::InputDeviceKeyboard >(
				InputDevice_Keyboard,
				gainput::InputDevice::DV_STANDARD	// raw doesn't have text input enabled
			)
			;
		mxASSERT(id == InputDevice_Keyboard);

		//
		input_manager
			.CreateDevice< gainput::InputDeviceMouse >(
				InputDevice_Mouse,
				gainput::InputDevice::DV_STANDARD//DV_RAW
			)
			;


		return ALL_OK;
	}

	virtual void Shutdown() override
	{
		//
	}

	virtual void UpdateOncePerFrame() override
	{
		_input_state.resetOncePerFrame();

		//
		SDL_PumpEvents();

		//
		SDL_Event sdl_event;

		while( SDL_PollEvent( &sdl_event ) )
		{
			_HandleSDLEvent( sdl_event );
		}

		//
		input_manager.Update();
	}

public:

	virtual const InputState& getState() const override
	{
		return _input_state;
	}

	virtual gainput::InputManager& getInputManager() override
	{
		return input_manager;
	}

	virtual void UnpressAllButtons() override
	{
		_input_state.mouse.held_buttons_mask = 0;
		_input_state.keyboard.held.ClearAllBits();
	}

	virtual void setRelativeMouseMode( bool recenter_mouse_cursor ) override
	{
		SDL_SetRelativeMouseMode( recenter_mouse_cursor ? SDL_TRUE : SDL_FALSE );
	}

	virtual void setMouseCursorHidden( bool hide ) override
	{
		SDL_ShowCursor( hide ? SDL_DISABLE : SDL_ENABLE );
	}

private:

	void _initSDLScancodeToMyKeycodeTable()
	{
		TSetStaticArray( _sdl_scancode_to_my_keycode, (KeyCodeT)KEY_Unknown );

		//
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_UNKNOWN] = KEY_Unknown;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_ESCAPE] = KEY_Escape;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_BACKSPACE] = KEY_Backspace;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_TAB] = KEY_Tab;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_RETURN] = KEY_Enter;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_SPACE] = KEY_Space;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_LSHIFT] = KEY_LShift;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_RSHIFT] = KEY_RShift;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_LCTRL] = KEY_LCtrl;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_RCTRL] = KEY_RCtrl;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_LALT] = KEY_LAlt;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_RALT] = KEY_RAlt;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_LGUI] = KEY_LWin;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_RGUI] = KEY_RWin;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_APPLICATION] = KEY_Apps;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_PAUSE] = KEY_Pause;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_CAPSLOCK] = KEY_Capslock;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_NUMLOCKCLEAR] = KEY_Numlock;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_SCROLLLOCK] = KEY_Scrolllock;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_PRINTSCREEN] = KEY_PrintScreen;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_PAGEUP] = KEY_PgUp;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_PAGEDOWN] = KEY_PgDn;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_HOME] = KEY_Home;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_END] = KEY_End;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_INSERT] = KEY_Insert;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_DELETE] = KEY_Delete;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_LEFT] = KEY_Left;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_UP] = KEY_Up;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_RIGHT] = KEY_Right;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_DOWN] = KEY_Down;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_0] = KEY_0;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_1] = KEY_1;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_2] = KEY_2;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_3] = KEY_3;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_4] = KEY_4;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_5] = KEY_5;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_6] = KEY_6;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_7] = KEY_7;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_8] = KEY_8;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_9] = KEY_9;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_A] = KEY_A;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_B] = KEY_B;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_C] = KEY_C;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_D] = KEY_D;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_E] = KEY_E;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F] = KEY_F;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_G] = KEY_G;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_H] = KEY_H;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_I] = KEY_I;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_J] = KEY_J;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_K] = KEY_K;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_L] = KEY_L;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_M] = KEY_M;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_N] = KEY_N;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_O] = KEY_O;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_P] = KEY_P;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_Q] = KEY_Q;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_R] = KEY_R;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_S] = KEY_S;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_T] = KEY_T;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_U] = KEY_U;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_V] = KEY_V;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_W] = KEY_W;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_X] = KEY_X;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_Y] = KEY_Y;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_Z] = KEY_Z;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_GRAVE] = KEY_Grave;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_MINUS] = KEY_Minus;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_EQUALS] = KEY_Equals;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_BACKSLASH] = KEY_Backslash;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_LEFTBRACKET] = KEY_LBracket;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_RIGHTBRACKET] = KEY_RBracket;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_SEMICOLON] = KEY_Semicolon;
//		_sdl_scancode_to_my_keycode[SDL_SCANCODE_QUOTE] = KEY_Apostrophe;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_COMMA] = KEY_Comma;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_PERIOD] = KEY_Period;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_SLASH] = KEY_Slash;

//		_sdl_scancode_to_my_keycode[SDL_SCANCODE_PLUS] = KEY_Plus;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_MINUS] = KEY_Minus;
//		_sdl_scancode_to_my_keycode[SDL_SCANCODE_ASTERISK] = KEY_Asterisk;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_EQUALS] = KEY_Equals;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_KP_0] = KEY_Numpad0;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_KP_1] = KEY_Numpad1;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_KP_2] = KEY_Numpad2;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_KP_3] = KEY_Numpad3;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_KP_4] = KEY_Numpad4;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_KP_5] = KEY_Numpad5;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_KP_6] = KEY_Numpad6;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_KP_7] = KEY_Numpad7;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_KP_8] = KEY_Numpad8;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_KP_9] = KEY_Numpad9;

		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F1] = KEY_F1;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F2] = KEY_F2;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F3] = KEY_F3;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F4] = KEY_F4;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F5] = KEY_F5;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F6] = KEY_F6;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F7] = KEY_F7;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F8] = KEY_F8;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F9] = KEY_F9;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F10] = KEY_F10;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F11] = KEY_F11;
		_sdl_scancode_to_my_keycode[SDL_SCANCODE_F12] = KEY_F12;
	}

	const EKeyCode _translateToMyKeycode( const SDL_Keysym& keysym ) const
	{
		return _sdl_scancode_to_my_keycode[ keysym.scancode ];
	}

	static void SDLCALL My_SDL_WindowsMessageHook(
		void *userdata,
		void *hWnd,
		unsigned int message,
		Uint64 wParam,
		Sint64 lParam
		)
	{
		NwInputSystem_SDL* input_system_sdl = (NwInputSystem_SDL*) userdata;

		MSG msg;
		mxZERO_OUT(msg);

		msg.hwnd = (HWND) hWnd;
		msg.message = message;
		msg.wParam = (WPARAM) wParam;
		msg.lParam = lParam;
		msg.time = mxGetTimeInMilliseconds();
		msg.pt.x = input_system_sdl->_input_state.mouse.x;
		msg.pt.y = input_system_sdl->_input_state.mouse.y;

		input_system_sdl->input_manager.HandleMessage( msg );
	}

	void _HandleSDLEvent( const SDL_Event& sdl_event )
	{
		//SDL_GetWindowFlags()
		//
		switch( sdl_event.type )
		{
		case SDL_QUIT:
			G_RequestExit();
			break;

			//case SDL_TEXTINPUT:
			//	{
			//		uchar buf[SDL_TEXTINPUTEVENT_TEXT_SIZE+1];
			//		size_t len = decodeutf8(buf, sizeof(buf)-1, (const uchar *)sdl_event.text.text, strlen(sdl_event.text.text));
			//		if(len > 0) { buf[len] = '\0'; processtextinput((const char *)buf, len); }
			//		break;
			//	}
		case SDL_TEXTINPUT:
			if( _input_state.window.has_focus )
			{
				const SDL_TextInputEvent& text_input_event = sdl_event.text;
				//DBGOUT("Text: %s", text_input_event.text);

				//new_input_event.type = SysEvent_Text;
				//new_input_event.text.character = textInput.text[0];

				strncpy(
					_input_state.input_text,
					text_input_event.text,
					mxCOUNT_OF(_input_state.input_text)
					);
			}
			break;
			//case SDL_TEXTEDITING:
			//	/*
			//	Update the composition text.
			//	Update the cursor position.
			//	Update the selection length (if any).
			//	*/
			//	composition = sdl_event.edit.text;
			//	cursor = sdl_event.edit.start;
			//	selection_len = sdl_event.edit.length;
			//	break;

		case SDL_KEYDOWN:
			if( _input_state.window.has_focus )
			{
				const SDL_KeyboardEvent& sdl_keyboard_event = sdl_event.key;
				const EKeyCode my_keycode = _translateToMyKeycode( sdl_keyboard_event.keysym );

				_input_state.keyboard.held.SetBit( my_keycode );

				_input_state.modifiers = getKeyModifiersFromSDL( sdl_keyboard_event.keysym.mod );
			}
			break;

		case SDL_KEYUP:
			if( _input_state.window.has_focus )
			{
				const SDL_KeyboardEvent& sdl_keyboard_event = sdl_event.key;
				const EKeyCode my_keycode = _translateToMyKeycode( sdl_keyboard_event.keysym );

				_input_state.keyboard.held.ClearBit( my_keycode );

				//
				//const Uint8* sdl_keyboard_state = SDL_GetKeyboardState();
				//const FKeyModifiersT my_key_modifiers = getKeyModifiersFromSDLKeyboardState( sdl_keyboard_state );
				_input_state.modifiers = getKeyModifiersFromSDL( sdl_keyboard_event.keysym.mod );
			}
			break;

		case SDL_WINDOWEVENT:
			switch( sdl_event.window.event )
			{
			case SDL_WINDOWEVENT_CLOSE:
				G_RequestExit();
				break;

			case SDL_WINDOWEVENT_FOCUS_GAINED:
				DBG_MSG("SDL_WINDOWEVENT_FOCUS_GAINED");
				_input_state.window.has_focus = true;
				break;
			case SDL_WINDOWEVENT_ENTER:
				DBG_MSG("SDL_WINDOWEVENT_ENTER");
				_input_state.window.has_focus = true;
				break;

			case SDL_WINDOWEVENT_LEAVE:
				DBG_MSG("SDL_WINDOWEVENT_LEAVE");
			case SDL_WINDOWEVENT_FOCUS_LOST:
				DBG_MSG("SDL_WINDOWEVENT_FOCUS_LOST");
				_input_state.window.has_focus = false;
				break;

			case SDL_WINDOWEVENT_MINIMIZED:
				DBG_MSG("SDL_WINDOWEVENT_MINIMIZED");
				_input_state.window.has_focus = false;
				break;

			case SDL_WINDOWEVENT_MAXIMIZED:
				DBG_MSG("SDL_WINDOWEVENT_MAXIMIZED");
			case SDL_WINDOWEVENT_RESTORED:
				DBG_MSG("SDL_WINDOWEVENT_RESTORED");
				_input_state.window.has_focus = false;
				break;

			case SDL_WINDOWEVENT_RESIZED:
				DBG_MSG("SDL_WINDOWEVENT_RESIZED");
				_input_state.window.width = sdl_event.window.data1;
				_input_state.window.height = sdl_event.window.data2;
				break;

			case SDL_WINDOWEVENT_SIZE_CHANGED:
				DBG_MSG("SDL_WINDOWEVENT_SIZE_CHANGED");
				_input_state.window.width = sdl_event.window.data1;
				_input_state.window.height = sdl_event.window.data2;
				break;
			}

			if(!_input_state.window.has_focus)
			{
				_input_state.mouse.held_buttons_mask = 0;
				_input_state.keyboard.held.ClearAllBits();
				_input_state.modifiers.raw_value = 0;

				gainput::InputDevice* keyboard_device
					= input_manager.GetDevice(InputDevice_Keyboard)
					;
				keyboard_device->ResetButtonsState();
			}
			break;

		case SDL_MOUSEMOTION:
			if( _input_state.window.has_focus )
			{
				const SDL_MouseMotionEvent& sdl_mouse_motion_event = sdl_event.motion;
				const bool mouse_moved = (sdl_mouse_motion_event.xrel != 0) || (sdl_mouse_motion_event.yrel != 0);
				if( mouse_moved )
				{
					_input_state.mouse.x = sdl_event.motion.x;
					_input_state.mouse.y = sdl_event.motion.y;
					_input_state.mouse.delta_x = sdl_event.motion.xrel;
					_input_state.mouse.delta_y = sdl_event.motion.yrel;
				}
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			if( _input_state.window.has_focus )
			{
				const EKeyCode mouse_key_code = getMouseButtonKeyCodeFromSDL( sdl_event.button.button );
				const EMouseButton mouse_button = getMouseButtonFromKeycode( mouse_key_code );

				_input_state.mouse.x = sdl_event.button.x;
				_input_state.mouse.y = sdl_event.button.y;
				_input_state.mouse.held_buttons_mask |= BIT(mouse_button);

				_input_state.keyboard.held.SetBit( mouse_key_code );
			}
			break;

		case SDL_MOUSEBUTTONUP:
			if( _input_state.window.has_focus )
			{
				const EKeyCode mouse_key_code = getMouseButtonKeyCodeFromSDL( sdl_event.button.button );
				const EMouseButton mouse_button = getMouseButtonFromKeycode( mouse_key_code );

				_input_state.mouse.x = sdl_event.button.x;
				_input_state.mouse.y = sdl_event.button.y;
				_input_state.mouse.held_buttons_mask &= ~BIT(mouse_button);

				_input_state.keyboard.held.ClearBit( mouse_key_code );
			}
			break;

		case SDL_MOUSEWHEEL:
			if( _input_state.window.has_focus )
			{
				const int value = sdl_event.wheel.y;
				const EKeyCode key = (value > 0) ? KEY_MWHEELUP : KEY_MWHEELDOWN;
				const int iterations = abs( value );

				//_input_state.mouse.wheel_scroll += (value > 0) ? +1 : -1;
				_input_state.mouse.wheel_scroll = value;
			}
			break;

		}//switch( SDL_Event.type )
	}

	static FKeyModifiersT getKeyModifiersFromSDL( Uint16 sdl_key_mod )
	{
		FKeyModifiersT modifiers(0);

		if( sdl_key_mod & KMOD_LSHIFT ) {
			modifiers |= KeyModifier_LShift;
		}
		if( sdl_key_mod & KMOD_RSHIFT ) {
			modifiers |= KeyModifier_RShift;
		}
		if( sdl_key_mod & KMOD_LCTRL ) {
			modifiers |= KeyModifier_LCtrl;
		}
		if( sdl_key_mod & KMOD_RCTRL ) {
			modifiers |= KeyModifier_RCtrl;
		}
		if( sdl_key_mod & KMOD_LALT ) {
			modifiers |= KeyModifier_LAlt;
		}
		if( sdl_key_mod & KMOD_RALT ) {
			modifiers |= KeyModifier_RAlt;
		}
		if( sdl_key_mod & KMOD_LGUI ) {
			modifiers |= KeyModifier_LWin;
		}
		if( sdl_key_mod & KMOD_RGUI ) {
			modifiers |= KeyModifier_RWin;
		}
		if( sdl_key_mod & KMOD_NUM ) {
			modifiers |= KeyModifier_NumLock;
		}
		if( sdl_key_mod & KMOD_CAPS ) {
			modifiers |= KeyModifier_CapsLock;
		}
		if( sdl_key_mod & KMOD_MODE ) {
			//the AltGr key is down
		}
		
		return modifiers;
	}

	static FKeyModifiersT getKeyModifiersFromSDLKeyboardState( const Uint8* keyboardState )
	{
		FKeyModifiersT modifiers(0);

		if( keyboardState[ KMOD_LSHIFT ] ) {
			modifiers |= KeyModifier_LShift;
		}
		if( keyboardState[ KMOD_RSHIFT ] ) {
			modifiers |= KeyModifier_RShift;
		}
		if( keyboardState[ KMOD_LCTRL ] ) {
			modifiers |= KeyModifier_LCtrl;
		}
		if( keyboardState[ KMOD_RCTRL ] ) {
			modifiers |= KeyModifier_RCtrl;
		}
		if( keyboardState[ KMOD_LALT ] ) {
			modifiers |= KeyModifier_LAlt;
		}
		if( keyboardState[ KMOD_RALT ] ) {
			modifiers |= KeyModifier_RAlt;
		}
		if( keyboardState[ KMOD_LGUI ] ) {
			modifiers |= KeyModifier_LWin;
		}
		if( keyboardState[ KMOD_RGUI ] ) {
			modifiers |= KeyModifier_RWin;
		}
		if( keyboardState[ KMOD_NUM ] ) {
			modifiers |= KeyModifier_NumLock;
		}
		if( keyboardState[ KMOD_CAPS ] ) {
			modifiers |= KeyModifier_CapsLock;
		}
		if( keyboardState[ KMOD_MODE ] ) {
			//the AltGr key is down
		}

		return modifiers;
	}

	static EKeyCode getMouseButtonKeyCodeFromSDL( Uint8 sdl_mouse_button )
	{
		switch( sdl_mouse_button )
		{
		case SDL_BUTTON_LEFT :		return KEY_MOUSE0;
		case SDL_BUTTON_MIDDLE :	return KEY_MOUSE2;
		case SDL_BUTTON_RIGHT :		return KEY_MOUSE1;
		case SDL_BUTTON_X1 :		return KEY_MOUSE3;
		case SDL_BUTTON_X2 :		return KEY_MOUSE4;
			mxNO_SWITCH_DEFAULT;
		}
		return KEY_Unknown;
	}

};//class NwInputSystem_SDL


ERet NwInputSystemI::create(
	NwInputSystemI **o_
	, AllocatorI& allocator
	)
{
	*o_ = allocator.new_< NwInputSystem_SDL >( allocator );
	return ALL_OK;
}

void NwInputSystemI::destroy(
	NwInputSystemI **o
	)
{
	NwInputSystem_SDL* input_system = checked_cast< NwInputSystem_SDL* >( *o );
	input_system->_allocator.delete_( input_system );
	*o = nil;
}
