/*
=============================================================================
	File:	Client.h
	Desc:	
=============================================================================
*/
#pragma once

//@this is bad: TCallback<> is used for input action binding
#include <Base/Template/Delegate/Delegate.h>
#include <Base/Template/Containers/Array/TInplaceArray.h>

#include <Core/Assets/AssetManagement.h>

#include <Input/Input.h>


//TODO: remove harcoded game state IDs, use polymorphism!
namespace GameStates {
	enum EGameStateIDs {
#define DECLARE_GAME_STATE( name, description )		name,
#include <Core/GameStates.inl>
#undef DECLARE_GAME_STATE
		MAX_STATES,
		ALL = -1,
	};
	mxSTATIC_ASSERT2(MAX_STATES < 32, Must_Fit_Into_Int32);
}//namespace GameStates

mxDECLARE_ENUM( GameStates::EGameStateIDs, U8, GameStateID );
mxDECLARE_FLAGS( GameStates::EGameStateIDs, U32, GameStateF );

/*
=============================================================================
	INPUT ACTION MAPPING
	(inspired by CryENGINE's approach)
=============================================================================
*/
namespace GameActions {
	enum EGameActionIDs {
#define DECLARE_GAME_ACTION( name, description )		name,
#include <Core/GameActions.inl>
#undef DECLARE_GAME_ACTION
		MAX_ACTIONS
	};
	mxSTATIC_ASSERT2(MAX_ACTIONS < 256, Some_Code_Relies_On_It);
}//namespace GameActions

mxDECLARE_ENUM( GameActions::EGameActionIDs, U8, GameActionID );

typedef TCallback< void ( GameActionID, EInputState, float ) >	ActionHandlerT;
typedef TKeyValue< GameActionID, ActionHandlerT >				ActionBindingT;

/*
-----------------------------------------------------------------------------
	KeyBind
-----------------------------------------------------------------------------
*/
struct KeyBind: CStruct
{
	KeyCodeT		key;	//1 Name of the key (keyboard, mouse or joystick key) bind
	FInputStateT	when;	//1 bitmask (EInputAction)
	FKeyModifiersT	with;	//2 bitmask (FKeyModifiers) of keys that need to be held down for this key bind to activate
	GameActionID	action;	//1 Command to execute when this key bind is activated
	//GameStateID		new_state;	//1 for state transitions
	String			command;	//8|16 command text
public:
	mxDECLARE_CLASS(KeyBind,CStruct);
	mxDECLARE_REFLECTION;
	KeyBind();
};

/*
-----------------------------------------------------------------------------
	AxisBind
-----------------------------------------------------------------------------
*/
struct AxisBind: CStruct
{
	InputAxisT		axis;	//1 mouse or joystick axis
	GameActionID	action;	//1 Command to execute when this binding is activated
public:
	mxDECLARE_CLASS(AxisBind,CStruct);
	mxDECLARE_REFLECTION;
	AxisBind();
};

/*
-----------------------------------------------------------------------------
	InputContext
	e.g. menu, in-game, PDA, vehicle, helicopter, cut scene, spectator, edit
-----------------------------------------------------------------------------
*/
struct InputContext: CStruct
{
	String				name;
	GameStateID			state;
	TArray< KeyBind >	key_binds;
	TArray< AxisBind >	axis_binds;
	//GameStateF			state_mask;
public:
	mxDECLARE_CLASS(InputContext,CStruct);
	mxDECLARE_REFLECTION;
	InputContext();

	class StatesEqual {
		const GameStateID m_state;
	public:
		StatesEqual( const GameStateID _state ) : m_state(_state) {}
		bool operator() ( const InputContext& _other ) const
		{ return m_state == _other.state; }
	};
};

struct NwInputBindings: NwResource
{
	//TArray< ActionBindingT >	handlers;
	TArray< InputContext >		contexts;
public:
	mxDECLARE_CLASS(NwInputBindings,NwResource);
	mxDECLARE_REFLECTION;

	void Dbg_Print() const;
};

/*
-----------------------------------------------------------------------------
	AViewport
	interface to a platform-specific window implementation
-----------------------------------------------------------------------------
*/
class AViewport {
public:
	virtual void* VGetWindowHandle() = 0;

	virtual void VGetSize( int &_w, int &_h ) = 0;
	virtual bool VIsFullscreen() const = 0;

	virtual bool VIsKeyPressed( EKeyCode key ) const = 0;
	virtual void VGetMousePosition( int &x, int &y ) const = 0;

	virtual float VGetTabletPressure() const { return 0.0f; }

protected:
	virtual ~AViewport() {}
};

/*
-----------------------------------------------------------------------------
	game/editor/whatever render viewports classes can inherit
	from this abstract interface to receive messages from the window
	(this is used for e.g. GUI window -> renderer viewport communication)
-----------------------------------------------------------------------------
*/
class AViewportClient: public AObject {
public:
	// user input event handling
	virtual void VOnInputEvent( AViewport* _window, const SInputEvent& _event )
	{}

	//// The mouse cursor entered the area of the window
	//virtual void VOnMouseEntered( AViewport* _window )
	//{}
	//// The mouse cursor left the area of the window
	//virtual void VOnMouseLeft( AViewport* _window )
	//{}

	// 'The application will come to foreground.'
	virtual void VOnFocusGained( AViewport* _window )
	{}
	// 'The application will come to background.'
	virtual void VOnFocusLost( AViewport* _window )
	{}

	// this function gets called when the parent window's size changes and this viewport should be resized
	virtual void VOnResize( AViewport* _window, int _width, int _height, bool _fullScreen )
	{}

	// the containing widget is about to be closed; shutdown everything (but keep the object around).
	virtual void VOnClose( AViewport* _window )
	{}

	// the containing viewport widget requested redraw
	virtual void VOnRePaint( AViewport* _window )
	{}

	virtual ECursorShape VGetCursorShape( AViewport* _window, int _x, int _y )
	{
		return CursorShape_Arrow;
	}

protected:
	AViewportClient()
	{}
	virtual ~AViewportClient()
	{}
};






struct NwCameraView;

/// passed to functions that depend on game time
struct NwTime
{
	/// Real time, always updated (e.g. can be used for rendering in-game GUI).
	struct
	{
		/// The (clamped) time elapsed since the last frame, in seconds.
		SecondsF	delta_seconds;

		/// The time elapsed since the application started, in milliseconds.
		U32			total_msec;	// MillisecondsU32

		/// The time elapsed since the application started, in seconds.
		SecondsD	total_elapsed;

		/// always increasing
		U32			frame_number;
	} real;

	/// Local game time (affected by slo-mo, etc).
	/// Should be used for updating the game world (e.g. entity animations).
	/// Not updated if the game is paused.
	struct
	{
		//
	} game;
};

///
class NwAppI: NonCopyable
{
public:
	virtual void RunFrame(
		const NwTime& game_time
		) {};

protected:
	virtual ~NwAppI() {}
};




enum EStateRunResult
{
	ContinueFurtherProcessing,

	/// e.g. if the user wants to quit the game
	StopFurtherProcessing,
};

//
// e.g. 'Menu', 'MainGame', 'Vehicle'
//
class NwGameStateI
	: public DbgNamedObject<>
	, NonCopyable
{
public:

	/// called before transitioning to another state
	virtual void onWillBecomeActive() {}
	virtual void onWillBecomeInactive() {}

	///
	virtual void Update(
		const NwTime& game_time
		, NwAppI* game
		)
	{};

	/// states rendered from top to bottom
	virtual EStateRunResult DrawScene(
		NwAppI* game
		, const NwTime& game_time
		)
	{
		// by default, allow previous states to render
		return ContinueFurtherProcessing;
	};

public:	// UI

	/// only the topmost state renders its UI
	virtual void DrawUI(
		const NwTime& game_time
		, NwAppI* game
		)
	{};


public:	// Debug HUD

	virtual bool mustDrawDebugHUD() const
	{return false;}

	virtual void drawDebugHUD(
		NwAppI* game
		, const NwTime& game_time
		)
	{}

public:
	virtual bool pausesGame() const
	{return false;}

public:	// Input Event Handling

	virtual EStateRunResult handleInput(
		NwAppI* game
		, const NwTime& game_time
		)
	{
		return StopFurtherProcessing;
	}


	virtual bool pinsMouseCursorToCenter() const
	{return false;}


	virtual bool hidesMouseCursor() const
	{return false;}

public:	// Game-Specific (should get rid of those)

	/// true to rotate the player's camera with mouse
	virtual bool allowsFirstPersonCameraToReceiveInput() const
	{return true;}

	/// allow to fire weapons, etc.?
	virtual bool AllowPlayerToReceiveInput() const
	{return true;}

	virtual bool ShouldRenderPlayerWeapon() const
	{return true;}

protected:
	virtual ~NwGameStateI() {}
};
