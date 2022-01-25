/*
=============================================================================
	File:	Driver.cpp
	Desc:	
=============================================================================
*/

#include <windowsx.h>	// GET_X_LPARAM, GET_Y_LPARAM

// SDL
#include <SDL2/include/SDL.h>
#include <SDL2/include/SDL_syswm.h>

#include <Base/Base.h>
#include <Core/Event.h>
#include <Core/Client.h>
#include <Engine/WindowsDriver.h>


#if 0
	#define DBG_MSG(...)\
		DBGOUT(__VA_ARGS__)
#else
	#define DBG_MSG(...)
#endif


mxDEFINE_CLASS(NwWindowSettings);
mxBEGIN_REFLECTION(NwWindowSettings)
	//mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(width),
	mxMEMBER_FIELD(height),
	mxMEMBER_FIELD(offset_x),
	mxMEMBER_FIELD(offset_y),

	mxMEMBER_FIELD(resizable),
	mxMEMBER_FIELD(fullscreen),
mxEND_REFLECTION;
NwWindowSettings::NwWindowSettings()
{
	this->SetDefaults();
}

void NwWindowSettings::SetDefaults()
{
	name = "Unnamed";

	width = ::GetSystemMetrics( SM_CXSCREEN );
	height = ::GetSystemMetrics( SM_CYSCREEN );
	offset_x = 0;
	offset_y = 0;

	resizable = false;
	fullscreen = true;
}

void NwWindowSettings::FixBadValues()
{
	const int system_res_x = ::GetSystemMetrics( SM_CXSCREEN );
	const int system_res_y = ::GetSystemMetrics( SM_CYSCREEN );

	width = Clamp(width, 640, system_res_x);
	height = Clamp(height, 480, system_res_y);
	offset_x = Clamp(offset_x, 0, system_res_x-1);
	offset_y = Clamp(offset_y, 0, system_res_y-1);
}


namespace WindowsDriver
{
	struct WindowsDriverData
	{
		// These are initially null and set later
		NwWindowedGameI *	game;
		NwGameLoopI *		game_loop;

		// SDL:
		SDL_Window *	main_window;
	};

	static TPtr< WindowsDriverData >	gs_data;

	Settings::Settings()
	{
	}

	ERet Initialize(
		const Settings& options
		)
	{
		gs_data.ConstructInPlace();

		gs_data->game = nil;
		gs_data->game_loop = nil;

		//
		SDL_Init(
			SDL_INIT_VIDEO
			);

		SDL_DisplayMode desktop;
		if(SDL_GetDesktopDisplayMode(0, &desktop) < 0) {
			ptERROR("SDL_GetDesktopDisplayMode() failed: %s", SDL_GetError());
			return ERR_UNKNOWN_ERROR;
		}

		int window_flags = 0;

		window_flags |= SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;
		if( options.window.resizable ) {
			window_flags |= SDL_WINDOW_RESIZABLE;
		}

		int windowX = (options.window.offset_x == -1) ? SDL_WINDOWPOS_UNDEFINED : options.window.offset_x;
		int windowY = (options.window.offset_y == -1) ? SDL_WINDOWPOS_UNDEFINED : options.window.offset_y;
		int windowWidth = options.window.width;
		int windowHeight = options.window.height;

		gs_data->main_window = SDL_CreateWindow(
			options.window.name.c_str(),
			windowX, windowY, windowWidth, windowHeight,
			window_flags
		);
		if( !gs_data->main_window ) {
			ptERROR("SDL_CreateWindow() failed: %s", SDL_GetError());
			return ERR_UNKNOWN_ERROR;
		}

		return ALL_OK;
	}

	void Shutdown()
	{
		SDL_Quit();

		gs_data->main_window = nil;

		gs_data->game = nil;
		gs_data->game_loop = nil;
	}

	void requestExit()
	{
		PostQuitMessage(0);
	}

	NwOpaqueWindow* getMainWindow()
	{
		return (NwOpaqueWindow*) gs_data->main_window;
	}

	void* GetNativeWindowHandle()
	{
		SDL_SysWMinfo wmInfo;
		SDL_VERSION( &wmInfo.version );

		mxENSURE(
			SDL_GetWindowWMInfo( gs_data->main_window, &wmInfo ) == SDL_TRUE
			, NULL
			, ""
		);

		return wmInfo.info.win.window;
	}

	UInt2 getWindowSize()
	{
		int width, height;
		SDL_GetWindowSize(
			gs_data->main_window
			, &width, &height
			);

		return UInt2( width, height );
	}

	void run(
		NwWindowedGameI* game
		, NwGameLoopI* game_loop
		)
	{
		mxASSERT_PTR(game);
		mxASSERT_PTR(game_loop);
		//
		//BROFILER_FRAME("MainLoop");	// should be placed into the Main Loop
		//mxPROFILE_SCOPE("MainLoop");

		gs_data->game = game;
		gs_data->game_loop = game_loop;

		// Main message loop

		while( !G_isExitRequested() )
		{
			//
			game_loop->update( game );
		}//

		gs_data->game = nil;
		gs_data->game_loop = nil;
	}

}//namespace WindowsDriver

/*
References:

Direct3D Game VS project templates
https://github.com/walbourn/directx-vs-templates

Optimize input latency for Universal Windows Platform (UWP) DirectX games [02/08/2017]
https://docs.microsoft.com/en-us/windows/uwp/gaming/optimize-performance-for-windows-store-direct3d-11-apps-with-coredispatcher

Multithreaded game-update loop:
http://tulrich.com/geekstuff/updatesnippets.cpp
(from http://tulrich.com/geekstuff/index.html)

Taking Advantage of High-Definition Mouse Movement
http://msdn.microsoft.com/en-us/library/windows/desktop/ee418864%28v=vs.85%29.aspx

Developing mouse controls (DirectX and C++)
http://msdn.microsoft.com/en-us/library/windows/apps/hh994925.aspx

Raw Input
http://msdn.microsoft.com/en-us/library/ms645536%28VS.85%29.aspx

Handling Multiple Mice with Raw Input
http://asawicki.info/news_1533_handling_multiple_mice_with_raw_input.html

Input for Modern PC Games
http://www.altdevblogaday.com/2011/04/25/input-for-modern-pc-games/

Properly handling keyboard input
http://molecularmusings.wordpress.com/2011/09/05/properly-handling-keyboard-input/

*/
