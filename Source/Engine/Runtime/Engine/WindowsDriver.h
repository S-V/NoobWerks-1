/*
=============================================================================
	File:	Driver.h
	Desc:	Platform Abstraction Layer: Collect and forward OS input events
=============================================================================
*/
#pragma once

#include <Base/Template/Containers/BitSet/BitField.h>
#include <Core/Client.h>
#include <Core/Event.h>
#include <Input/Input.h>


/*
=============================================================================
	GAME INTERFACE
=============================================================================
*/

typedef U64 Microseconds64T;

///
class NwWindowedGameI: public NwAppI
{
public:

	// Basic game loop

public:	// Messages

	/// Game is becoming active window.
	virtual void onActivated() {};

	/// Game is becoming background window.
	virtual void onDeactivated() {};

	/// Game is becoming background window.
	virtual void onSuspending() {};

	/// Game is being power-resumed (or returning from minimize).
	virtual void onResuming() {};

	/// Game window is being resized.
	virtual void onWindowSizeChanged(int width, int height) {};

public:	// Properties

	/// Change to desired default window size (note minimum size is 320x200).
	virtual void getDefaultWindowSize(
		int& width, int& height
		) const {}

protected:
	virtual ~NwWindowedGameI() {}
};

///
class NwGameLoopI: NonCopyable
{
public:
	virtual void update(
		NwAppI* game
		) = 0;

protected:
	virtual ~NwGameLoopI() {}
};


/*
=============================================================================
	
	PLATFORM DRIVER

=============================================================================
*/

struct NwOpaqueWindow;


struct NwWindowSettings: CStruct
{
	String32	name;	// not serialized

	int	width;
	int	height;
	int	offset_x;
	int	offset_y;

	bool resizable;
	bool fullscreen;

public:
	mxDECLARE_CLASS(NwWindowSettings,CStruct);
	mxDECLARE_REFLECTION;

	NwWindowSettings();

	void SetDefaults();
	void FixBadValues();
};


namespace WindowsDriver
{
	struct Settings
	{
		NwWindowSettings	window;

	public:
		Settings();
	};

	ERet Initialize(
		const Settings& options = Settings()
		);

	void Shutdown();

	void run(
		NwWindowedGameI* game
		, NwGameLoopI* game_loop
		);

	void requestExit();

	NwOpaqueWindow* getMainWindow();

	/// returns HWND on Windows
	void* GetNativeWindowHandle();

	UInt2 getWindowSize();

}//namespace WindowsDriver
