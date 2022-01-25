/*
=============================================================================
	File:	BuildConfig.h
	Desc:	Compilation options, preprocessor settings for compiling different versions.
	Note:	Always make sure this header gets included first.
=============================================================================
*/
#pragma once

#include "Language.h"

//----------------------------------------------
//	General.
//----------------------------------------------

// turn this off to disable many annoying warnings
// NOTE: turning on all compiler warnings can help find obscure bugs
#define MX_ENABLE_ALL_COMPILER_WARNINGS	(0)

//----------------------------------------------
//	Debug/Release.
//----------------------------------------------

#if defined(___DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
	/// Debug build.
	#define MX_DEBUG	(1)
	#define mxBUILD_MODE_STRING	"DEBUG"
#else
	#define MX_DEBUG	(0)
	#define mxBUILD_MODE_STRING	"RELEASE"
#endif

//----------------------------------------------
//	Development.
//	Don't forget to disable some of these settings
//	in the final build of the product!
//----------------------------------------------

//#define NO_DEVELOPER

#ifndef NO_DEVELOPER
/// Emit code for statistics, testing, in-game profiling and debugging, developer console, etc.
#define MX_DEVELOPER			(1)
#else
#define MX_DEVELOPER			(0)
#endif

// Profile and collect data for analysis.
#define MX_ENABLE_PROFILING			(0)	//Disabled our built-in profiler, using Brofiler.

#define MX_ENABLE_MEMORY_TRACKING	(0)

#define MX_ENABLE_REFLECTION		(1)

// prefer low-energy consuming CPU instructions (mobile platforms only)
#define MX_ENABLE_POWER_SAVING		(0)

// set when building the editor (and other tools)
// slows down a lot and wastes memory
#ifndef NO_EDITOR
#define MX_EDITOR	(MX_DEVELOPER)
#else
#define MX_EDITOR	(0)
#endif

//----------------------------------------------
//	Misc settings.
//----------------------------------------------

#define MX_USE_ASM	(0)
#define MX_USE_SSE	(1)


//----------------------------------------------
//	Engine versioning.
//----------------------------------------------

//
// These are used for preventing file version problems, DLL hell, etc.
//
#define mxENGINE_VERSION_MINOR	0
#define mxENGINE_VERSION_MAJOR	0

#define mxENGINE_VERSION_NUMBER	0
#define mxENGINE_VERSION_STRING	"0.0"

// If this is defined, the executable will work only with demo data.
#define MX_DEMO_BUILD			(1)

// returns a string reflecting the time when the Base project was built
// in the format "Mmm dd yyyy - hh:mm:ss"
extern const char* mxGetBaseBuildTimestamp();
