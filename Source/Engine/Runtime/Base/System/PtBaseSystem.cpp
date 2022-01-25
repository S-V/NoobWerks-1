/*
=============================================================================
	File:	Core.cpp
	Desc:	Base system, init/cleanup.
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <process.h>	// _cexit

#include <Base/Object/Reflection/TypeRegistry.h>

namespace
{

static NiftyCounter	g_iBaseSystemReferenceCount;

static FCallback *	g_pExitHandler = nil;
static void *		g_pExitHandlerArg = nil;

}//anonymous namespace

/*
================================
	mxBaseSystemIsInitialized
================================
*/
bool mxBaseSystemIsInitialized() {
	return (g_iBaseSystemReferenceCount.GetCount() > 0);
}

/*
================================
	mxInitializeBase
================================
*/
ERet mxInitializeBase()
{
	if( g_iBaseSystemReferenceCount.IncRef() )
	{
		// Initialize platform-specific services.
		mxPlatform_Init();

		TypeRegistry::Initialize();
	}
	return ALL_OK;
}

/*
================================
	mxShutdownBase
================================
*/
bool mxShutdownBase()
{
	if( g_iBaseSystemReferenceCount.DecRef() )
	{
		if( g_pExitHandler ) {
			g_pExitHandler( g_pExitHandlerArg );
		}

		// Destroy the type system.
		TypeRegistry::Destroy();

#if MX_ENABLE_PROFILING
		PtProfileManager::CleanupMemory();
#endif

		mxPlatform_Shutdown();

		return true;
	}

	return false;
}

/*
================================
	ForceExit_BaseSystem
================================
*/
void mxForceExit( int exitCode )
{
	ptPRINT( "Program ended at your request.\n" );

	//@todo: call user callback to clean up resources

	while( !g_iBaseSystemReferenceCount.DecRef() )
		;
	mxShutdownBase();

	// shutdown the C runtime, this cleans up static objects but doesn't shut 
	// down the process
	::_cexit();

	//::exit( exitCode );
	::ExitProcess( exitCode );
}

/*
================================
	mxSetExitHandler
================================
*/
void mxSetExitHandler( FCallback* pFunc, void* pArg )
{
	g_pExitHandler = pFunc;
	g_pExitHandlerArg = pArg;
}

/*
================================
	mxSetExitHandler
================================
*/
void mxGetExitHandler( FCallback **pFunc, void **pArg )
{
	mxASSERT_PTR(pFunc);
	mxASSERT_PTR(pArg);
	*pFunc = g_pExitHandler;
	*pArg = g_pExitHandlerArg;
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
