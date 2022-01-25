#include "Scripting_PCH.h"
#pragma hdrstop


#include <Quirrel/include/squirrel.h>

// sqstd_seterrorhandlers
#include <Quirrel/include/sqstdaux.h>

//sqstd_dofile
#include <Quirrel/include/sqstdio.h>

#if 0
#pragma comment( lib, "Quirrel.lib" )
#endif



//#include <Wren/include/wren.h>
//#if 1
//#pragma comment( lib, "Wren.lib" )
//#endif

#include <Base/Template/Containers/Blob.h>

#include <Core/Memory.h>
#include <Core/ObjectModel/Clump.h>

//
#include <Scripting/Scripting.h>
#include <Scripting/Lua/LuaScriptingEngine.h>
//#include <Scripting/Lua/Lua_Helpers.h>
//#include <Scripting/FunctionBinding.h>
//#include <Scripting/ScriptBindings_Core.h>
//#include <Scripting/ScriptBindings_Input.h>
//#include <Scripting/ScriptBindings_Globals.h>


namespace Scripting
{
	enum {
		SCRIPT_ALIGN = 4,

		/// initial stack size
		INIT_STACK_SIZE = 1024
	};

	struct QuirrelContext
	{
		// Squirrel Virtual Machine
		HSQUIRRELVM		vm;
	};
	mxDECLARE_PRIVATE_DATA( QuirrelContext, gs_ctx );

	//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	#define me	mxGET_PRIVATE_DATA( QuirrelContext, gs_ctx )
	//-----------------------------------------------------------------------

#if 0
	// add the given function to "package.preload" so that we can easily "require" it.
	static int AddModuleToPreload( lua_State* L, const char* name, lua_CFunction loader )
	{
		lua_getglobal( L, "package" );
		lua_getfield( L, -1, "preload" );
		lua_pushcfunction( L, loader );
		lua_setfield( L, -2, name );
		lua_pop( L, 2 );
		return 0;
	}
#endif


#ifdef SQUNICODE 
#define scvprintf vwprintf 
#else 
#define scvprintf vprintf 
#endif 

	static void MySQ_printfunc(HSQUIRRELVM v, const SQChar *s, ...) 
	{ 
		va_list arglist; 
		va_start(arglist, s); 
		scvprintf(s, arglist); 
		va_end(arglist); 
	} 


	static void MySQ_CompileErrorCallback(
		HSQUIRRELVM /*v*/
		,const SQChar* desc
		,const SQChar* source
		,SQInteger line
		,SQInteger column
		)
	{
		DBGOUT("[Script] Error: %s", desc);
	}

	static SQInteger MySq_WriteFunc(
		SQUserPointer user_ptr
		, SQUserPointer bytes_to_write
		, SQInteger num_bytes_to_write
		)
	{
		NwBlobWriter&	bytecode_writer = *(NwBlobWriter*) user_ptr;

		const char* chars_to_write = (char*) bytes_to_write;

		ERet res = bytecode_writer.Write(chars_to_write, num_bytes_to_write);
		mxASSERT(res == ALL_OK);

		return res == ALL_OK ? num_bytes_to_write : 0;
	}


	/// The function is called every time the compiler needs a character;
	/// It has to return a character code if succeed or 0 if the source is finished.
	//typedef SQInteger (*SQREADFUNC)(SQUserPointer,SQUserPointer,SQInteger);
	static SQInteger MySq_ReadFunc( SQUserPointer user_ptr, SQUserPointer dst_buf, SQInteger num_bytes_to_read )
	{
		NwBlobReader&	bytecode_reader = *(NwBlobReader*) user_ptr;

		ERet res = bytecode_reader.Read(dst_buf, num_bytes_to_read);
		mxASSERT(res == ALL_OK);

		return res == ALL_OK ? num_bytes_to_read : 0;
	}



	ERet Initialize()
	{
		mxInitializeBase();
		mxINITIALIZE_PRIVATE_DATA( gs_ctx );

UNDONE;

#if 0
		// Create a virtual machine with the given initial stack size.
		me.vm = sq_open(INIT_STACK_SIZE);


		sqstd_seterrorhandlers(me.vm);

		//sets the print function
		sq_setprintfunc(
			me.vm
			, &MySQ_printfunc
			, NULL
			);


		sq_setcompilererrorhandler(
			me.vm,
			&MySQ_CompileErrorCallback
			);

		////
		//sq_compile(
		//	me.vm
		//	, &MySq_ReadFunc // SQLEXREADFUNC
		//	, f // SQUserPointer
		//	, "test" // filename
		//	, 1 // SQBool raiseerror
		//	);


		const char* saved_bytecode_filename = "R:/test.bc";
		
		//
		const char* src_txt = "function SayHi() { print(\"Hi\"); }";

		const SQRESULT compile_buffer_res = sq_compilebuffer(
			me.vm

			// SQChar* s (const) – a pointer to the buffer that has to be compiled.
			, src_txt
			// size (SQInteger) – size in characters of the buffer passed in the parameter ‘s’.
			, strlen(src_txt)

			// SQChar * sourcename (const) – the symbolic name of the program (used only for more meaningful runtime errors)
			, "test" // filename

			// raiseerror (SQBool) – if this value true the compiler error handler will be called in case of an error
			, 1 // SQBool raiseerror
			);
		// If sq_compile succeeds, the compiled script will be pushed
		// as Quirrel function in the stack.
		if(compile_buffer_res == SQ_OK)
		{
			//
			NwBlob	bytecode_blob(MemoryHeaps::temporary());
			NwBlobWriter	bytecode_writer(bytecode_blob);

			// serializes(writes) the closure on top of the stack,
			// the destination is user defined through a write callback.
			const SQRESULT write_closure_res = sq_writeclosure(
				me.vm
				
				// writef (SQWRITEFUNC) – pointer to a write function
				// that will be invoked by the vm during the serialization.
				, &MySq_WriteFunc //SQWRITEFUNC writef

				// up (SQUserPointer) – pointer that will be passed to each call to the write function
				, &bytecode_writer // SQUserPointer up
				);
			DBGOUT("Bytecode size: %d", bytecode_blob.rawSize());

			//
			mxDO(NwBlob_::saveBlobToFile(
				bytecode_blob
				, saved_bytecode_filename
				));

			//
			SQInteger top0 = sq_gettop(me.vm);


			//
			const SQRESULT call_res = sq_call(
				me.vm	// HSQUIRRELVM v
				, 0 //SQInteger params – number of parameters of the function
				, false // SQBool retval - if true the function will push the return value in the stack
				, true //SQBool invoke_err_handler – if true, if a runtime error occurs during the execution of the call, the vm will invoke the error handler.
				);
			DBGOUT("Call res: %d", call_res);

			SQInteger top1 = sq_gettop(me.vm);
			SQObjectType top_obj_type = sq_gettype(me.vm, top1);
			DBGOUT("top_obj_type = %d!", top_obj_type);
		}

		////
		//sq_settop(
		//	me.vm
		//	,1
		//	);






#if 1

		//push the root table(where the globals of the script will be stored)
		sq_pushroottable(me.vm);

		const SQRESULT dofile_res = sqstd_dofile(
			me.vm
			, saved_bytecode_filename//_SC("R:/test.bc")

			// if true the function will push the return value of the executed script in the stack.
			, 0 // retval

			// if true the compiler error handler will be called if a error occurs
			, 1 // printerror
			);


		int top = sq_gettop(me.vm); //saves the stack size before the call

		sq_pushroottable(me.vm); //pushes the global table
		sq_pushstring(me.vm,_SC("SayHi"),-1);
		if(SQ_SUCCEEDED(sq_get(me.vm,-2))) { //gets the field 'foo' from the global table
			sq_pushroottable(me.vm); //push the 'this' (in this case is the global table)
			//sq_pushinteger(v,n); 
			//sq_pushfloat(v,f);
			//sq_pushstring(v,s,-1);
			sq_call(
				me.vm
				,4	//SQInteger params
				,0	//SQBool retval
				,0	//SQBool invoke_err_handler
				); //calls the function 
		}
		sq_settop(me.vm,top); //restores the original stack size


#else



		////
		NwBlob	loaded_bytecode_blob(MemoryHeaps::temporary());
		mxDO(NwBlob_::loadBlobFromFile(loaded_bytecode_blob, saved_bytecode_filename));

		NwBlobReader	blob_reader(loaded_bytecode_blob);

		//
		const SQRESULT read_res = sq_readclosure(
			me.vm
			, &MySq_ReadFunc //SQREADFUNC readf
			, &blob_reader//SQUserPointer up
			);

		//
		const SQRESULT call_res2 = sq_call(
			me.vm	// HSQUIRRELVM v
			, 0 //SQInteger params – number of parameters of the function
			, false // SQBool retval - if true the function will push the return value in the stack
			, true //SQBool invoke_err_handler – if true, if a runtime error occurs during the execution of the call, the vm will invoke the error handler.
			);
		DBGOUT("Call res2: %d", call_res2);

#endif


		//
		//sq_compile(
		//	me.vm
		//	, &MySq_ReadFunc // SQLEXREADFUNC
		//	, f // SQUserPointer
		//	, "test" // filename
		//	, 1 // SQBool raiseerror
		//	);
		//	
		//saved_bytecode_filename


//		// Create a Lua virtual machine.
//		lua_State* L = lua_newstate( &Script_Realloc, NULL );
//		me.L = L;
//		if( !L ) {
//			ptERROR("Failed to initialize Lua.\n");
//			return ERR_UNKNOWN_ERROR;
//		}
//
//		// Install panic function.
//		lua_atpanic( L, &ScriptErrorCallback );
//
//		LUA_TRY(L, lua_gc(L, LUA_GCCOLLECT, 0));	// Garbage collect to free as much as possible
//		lua_setallocf( L,  &Script_Realloc, NULL );
//
//		// Open default libraries
//		luaL_openlibs( L );
//
//		// Stop the GC during Lua library (all std) initialization and function registrations.
//		// C functions that are callable from a Lua script must be registered with Lua.
//		LUA_TRY(L, lua_gc( L, LUA_GCSTOP, 0 ));
//		{
//			Script_RegisterGlobals( L );
//
//			ScriptModuleLoader_Core( L );
//#if 0
//			ScriptModuleLoader_Input( L );
//#endif
//		}
//		LUA_TRY(L, lua_gc( L, LUA_GCRESTART, 0 ));
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

#endif

		return ALL_OK;
	}

	void Shutdown()
	{
UNDONE;

#if 0
		sq_close(me.vm);
		me.vm = nil;
		//lua_State* L = me.L;
		//lua_gc( L, LUA_GCCOLLECT, 0 );
		//lua_close( L );
		//me.L = nil;
		mxSHUTDOWN_PRIVATE_DATA( gs_ctx );
		mxShutdownBase();

#endif
	}

	//lua_State* GetLuaState()
	//{
	//	return me.L;
	//}

	//ERet ExecuteBuffer( const char* str, int len )
	//{
	//	lua_State *	L = me.L;
	//	LUA_DBG_CHECK_STACK(L);
	//	// Load script
	//	LUA_TRY(L, luaL_loadbuffer( L, str, len, NULL ));
	//	LUA_TRY(L, lua_pcall( L, 0, 0, 0 ));
	//	return ALL_OK;
	//}

	//const char* LuaConfig::FindString( const char* _key ) const
	//{
	//	lua_getglobal( m_L, _key );
	//	if( !lua_isstring( m_L, -1 ) )
	//	{
	//		return NULL;
	//	}
	//	return lua_tostring( m_L, -1 );
	//}
	//bool LuaConfig::FindInteger( const char* _key, int &_value ) const
	//{
	//	lua_getglobal( m_L, _key );
	//	if( !lua_isinteger( m_L, -1 ) )
	//	{
	//		return false;
	//	}
	//	_value = lua_tointeger( m_L, -1 );
	//	return true;
	//}
	//bool LuaConfig::FindSingle( const char* _key, float &_value ) const
	//{
	//	lua_getglobal( m_L, _key );
	//	if( !lua_isnumber( m_L, -1 ) )
	//	{
	//		return false;
	//	}
	//	_value = lua_tonumber( m_L, -1 );
	//	return true;
	//}
	//bool LuaConfig::FindBoolean( const char* _key, bool &_value ) const
	//{
	//	lua_getglobal( m_L, _key );
	//	if( !lua_isboolean( m_L, -1 ) )
	//	{
	//		return false;
	//	}
	//	_value = lua_toboolean( m_L, -1 );
	//	return true;
	//}
}//namespace Scripting


ERet NwScriptingEngineI::Create(
								NwScriptingEngineI *&scripting_engine_
								, AllocatorI& allocator
								)
{
	LuaScriptingEngine* lua_scripting_engine;
	mxDO(nwCREATE_OBJECT(
		lua_scripting_engine
		, allocator
		, LuaScriptingEngine
		, allocator
		));

	//
	TAutoDestroy<LuaScriptingEngine>	auto_destroy(
		lua_scripting_engine
		, allocator
		);

	//
	mxDO(lua_scripting_engine->Setup());

	auto_destroy.Disown();
	scripting_engine_ = lua_scripting_engine;
	return ALL_OK;
}

void NwScriptingEngineI::Destroy(
								 NwScriptingEngineI *scripting_engine
								 )
{
	LuaScriptingEngine* lua_scripting_engine
		= checked_cast<LuaScriptingEngine*>(scripting_engine);

	lua_scripting_engine->Close();

	nwDESTROY_OBJECT(
		lua_scripting_engine,
		lua_scripting_engine->_allocator
		);
}
