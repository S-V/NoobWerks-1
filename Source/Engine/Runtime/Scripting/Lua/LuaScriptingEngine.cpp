#include "Scripting_PCH.h"
#pragma hdrstop

#include <Scripting/Lua/Lua_Helpers.h>

#include <Scripting/Scripting.h>
#include <Scripting/Lua/LuaScriptingEngine.h>


LuaScriptingEngine::LuaScriptingEngine(AllocatorI& allocator)
	: _allocator(allocator)
{
	_L = nil;
}

namespace
{

enum EConstants
{
	SCRIPT_ALIGN = 4,

	/// initial stack size
	INIT_STACK_SIZE = 1024
};

// See: lua_Alloc()
// ud - an opaque pointer passed to lua_newstate (via userdata)
// ptr - a pointer to the block being allocated/reallocated/freed
// osize - the original size of the block
// nsize - the new size of the block
//
// ptr is NULL if and only if osize is zero.
// When nsize is zero, the allocator must return NULL;
// if osize is not zero, it should free the block pointed to by ptr. When nsize is not zero,
// the allocator returns NULL if and only if it cannot fill the request.
// When nsize is not zero and osize is zero, the allocator should behave like malloc.
// When nsize and osize are not zero, the allocator behaves like realloc.
// Lua assumes that the allocator never fails when osize >= nsize.
//
static void* Script_Realloc( void *ud, void *ptr, size_t osize, size_t nsize )
{
	LuaScriptingEngine* lua_scripting_engine
		= static_cast<LuaScriptingEngine*>(ud);

	return lua_scripting_engine->LuaAlloc(ptr, osize, nsize);
}

static int ScriptErrorCallback( lua_State* L )
{
	size_t len;
	const char* msg = lua_tolstring( L, -1, &len );
	mxASSERT_PTR(msg);
	mxGetLog().PrintF( LL_Warn, "Lua: unprotected error: %s\n", msg );
	return 0;
}

}//namespace


//
//#if 0
//		{
//			Assets::AssetMetaType& callbacks = Assets::gs_assetTypes[AssetTypes::SCRIPT];
//			callbacks.loadData = &Script::Load;
//			callbacks.finalize = &Script::Online;
//			callbacks.bringOut = &Script::Offline;
//			callbacks.freeData = &Script::Destruct;
//		}
//#endif
//
//
//		const AConsoleVariable* cvar = AConsoleVariable::Head();
//		while( cvar )
//		{
//			cvar = cvar->Next();
//		}
ERet LuaScriptingEngine::Setup()
{
	// Create a Lua virtual machine.
	_L = lua_newstate( &Script_Realloc, this );

	if( !_L ) {
		ptERROR("Failed to initialize Lua.\n");
		return ERR_UNKNOWN_ERROR;
	}

	// Install panic function.
	lua_atpanic( _L, &ScriptErrorCallback );

	LUA_TRY(
		lua_gc(_L, LUA_GCCOLLECT, 0)
		, _L
		);	// Garbage collect to free as much as possible
	lua_setallocf( _L,  &Script_Realloc, this );

	// Open default libraries
	luaL_openlibs( _L );

#if 0
	// Stop the GC during Lua library (all std) initialization and function registrations.
	// C functions that are callable from a Lua script must be registered with Lua.
	LUA_TRY(_L, lua_gc( _L, LUA_GCSTOP, 0 ));
	{
		Script_RegisterGlobals( _L );

		ScriptModuleLoader_Core( _L );
#if 0
		ScriptModuleLoader_Input( L );
#endif
	}
	LUA_TRY(_L, lua_gc( _L, LUA_GCRESTART, 0 ));
#endif

	mxDO(_sched.Setup());

	return ALL_OK;
}

void LuaScriptingEngine::Close()
{
	_sched.Close();

	//
	lua_gc( _L, LUA_GCCOLLECT, 0 );

	lua_close( _L );

	_L = nil;
}

void LuaScriptingEngine::TickOncePerFrame(const NwTime& game_time)
{
	for( UINT i = 0; i < _coroutines.num(); i++ )
	{
		Coroutine& coroutine = _coroutines._data[i];
		if(_ShouldRunCoroutine(coroutine, game_time))
		{
			_RunCoroutine(coroutine, game_time);
		}
	}
}

ERet LuaScriptingEngine::StartCoroutine(
	const NwScript&	script
	, const char* entry_point
	)
{
	global_State *	global_Lua_state = _L->l_G;
	DBGOUT("Lua memory usage before: %d", global_Lua_state->totalbytes);

	//
	Coroutine	new_coroutine;

	// To start a coroutine, you first create a new thread.
	new_coroutine.L = lua_newthread(_L);
	new_coroutine.num_calls = 0;

	//
	mxDO(MyLua::LoadChunk(script, new_coroutine.L));

	LUA_TRY(lua_pcall(
		new_coroutine.L
		, 0	// int nargs
		, 0	// int nresults
		, 0	// int errfunc
		)
		, new_coroutine.L
		);

	lua_getglobal(new_coroutine.L, entry_point);

	//
	DBGOUT("Lua memory usage after: %d", global_Lua_state->totalbytes);

	mxDO(_coroutines.add(new_coroutine));

	return ALL_OK;
}

bool LuaScriptingEngine::_ShouldRunCoroutine(
	const Coroutine& coroutine
	, const NwTime& game_time
	) const
{
	return true;
}

ERet LuaScriptingEngine::_RunCoroutine(
	Coroutine & coroutine
	, const NwTime& game_time
	)
{
	int	nresults;
	const int resume_result = lua_resume(
		coroutine.L
		, nil	// lua_State *from
		, 0	// int narg
		, &nresults
		);
	++coroutine.num_calls;

	if( resume_result == LUA_YIELD )
	{
		//running

#if 0
		const int type_of_top_elem = lua_type(coroutine.L, -1);

		const int num_elems_on_stack = lua_gettop(coroutine.L);

		const char* s = lua_tostring(coroutine.L, -1);
		DBGOUT("[Script] Running: '%s' (%d calls)",
			s
			, coroutine.num_calls
			);

		if( 1 == lua_isnumber(coroutine.L, -1) )
		{
			lua_Number num = lua_tonumber(coroutine.L, -1);
			DBGOUT(
				"Got %f\n",
				num
				);
		}
#endif
	}
	else if( resume_result == 0 )
	{
		// finished.
		DBGOUT("[Script] Coroutine exited: %d calls",
			coroutine.num_calls
			);
		return ALL_OK;
	}
	else
	{
		// error
		const char* err_msg = lua_tostring(coroutine.L, -1);
		lua_pop(coroutine.L, 1);  // pop error message from the stack
		ptWARN("[SCRIPT] Lua error: %s\n", err_msg);
	}

	return ALL_OK;
}

void* LuaScriptingEngine::LuaAlloc( void *ptr, size_t osize, size_t nsize )
{
	void* result = NULL;

	if( osize && nsize ) {
		result = _Realloc(ptr, osize, nsize);
	} else if( nsize ) {
		//DBGOUT("NwScript:Alloc %u bytes\n", nsize);
		result = _allocator.Allocate( nsize, SCRIPT_ALIGN );
	} else if( osize ) {
		//DBGOUT("NwScript:Free %u bytes\n", osize);
		_allocator.Deallocate(ptr/*, osize*/);
	}

	return result;
}

void* LuaScriptingEngine::_Realloc( void* old_ptr, size_t old_size, size_t new_size )
{
	mxASSERT( old_size > 0 );
	mxASSERT( new_size > 0 );
	//mxASSERT( old_size != new_size );
	//DBGOUT("MyRealloc: old=%u, new=%u",old_size,new_size);
	void* new_ptr = old_ptr;
	if( new_size > old_size )
	{
		new_size = tbALIGN4(new_size);
		new_ptr = _allocator.Allocate(new_size, SCRIPT_ALIGN);
		if( old_ptr ) {
			memcpy(new_ptr, old_ptr, old_size);
			_allocator.Deallocate(old_ptr/*, old_size*/);
		}
	}
	return new_ptr;
}
