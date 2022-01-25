#include "Scripting_PCH.h"
#pragma hdrstop
#include <Scripting/Lua/Lua_Helpers.h>
#include <Scripting/ScriptResources.h>


ERet Lua_GetErrorCode( int luaErr )
{
	switch( luaErr )
	{
	case LUA_ERRRUN :
		// A runtime error.
		return ERR_RUNTIME_ERROR;
	case LUA_ERRSYNTAX :
		return ERR_SYNTAX_ERROR;
	case LUA_ERRMEM :
		// Memory allocation error. For such errors, Lua does not call the error handler function.
		return ERR_OUT_OF_MEMORY;
	case LUA_ERRERR :
		// Error while running the error handler function.
		return ERR_WHILE_ERROR_HANDLER;
	}
	return ALL_OK;
}

// taken from: http://zeuxcg.org/2010/11/07/lua-callstack-with-c-debugger/
void Lua_PrintStack( lua_State* L )
{
    lua_Debug	entry;
    int			depth = 0;
 
    while( lua_getstack( L, depth, &entry ) )
    {
        int status = lua_getinfo( L, "Sln", &entry );
        mxASSERT(status); 
        DBGOUT("%s(%d): %s\n", entry.short_src, entry.currentline, entry.name ? entry.name : "?");
        depth++;
    }
}

ScriptVariant::Type GetValueType( int luaValType )
{
	switch( luaValType )
	{
	case LUA_TNIL:		return ScriptVariant::Nil;
	case LUA_TBOOLEAN:	return ScriptVariant::Boolean;
	case LUA_TLIGHTUSERDATA:	return ScriptVariant::Undefined;
	case LUA_TNUMBER:	return ScriptVariant::Number;
	case LUA_TSTRING:	return ScriptVariant::String;
	case LUA_TTABLE:	return ScriptVariant::LuaTable;
	case LUA_TFUNCTION:	return ScriptVariant::Function;
	case LUA_TUSERDATA:	return ScriptVariant::UserData;
	case LUA_TTHREAD:	return ScriptVariant::Undefined;
	mxNO_SWITCH_DEFAULT;
	}
	return ScriptVariant::Undefined;
}

void Lua_Push( lua_State* L, const ScriptVariant& _value )
{
	switch( _value.type )
	{
	case ScriptVariant::Nil:
		lua_pushnil( L );
		break;
	case ScriptVariant::Boolean:
		lua_pushboolean( L, _value.b );
		break;
	case ScriptVariant::Number:
		lua_pushnumber( L, _value.n );
		break;
	case ScriptVariant::String:
		lua_pushstring( L, _value.s );
		break;
	case ScriptVariant::Function:
		lua_pushcfunction( L, _value.f );
		break;
	case ScriptVariant::UserData:
		lua_pushlightuserdata( L, (void*)_value.p );
		break;
	mxNO_SWITCH_DEFAULT;
	}
}

const ScriptVariant Lua_Pop( lua_State* L )
{
	int type = lua_type( L, -1 );
	mxASSERT2(type != LUA_TNONE, "Lua stack is empty");

	int idx = -1;
	size_t len;

	ScriptVariant result;

	result.type = ScriptVariant::Undefined;

	switch( type )
	{
	case LUA_TNIL:
		result.type = ScriptVariant::Nil;
		result.U = 0;
		break;
	case LUA_TBOOLEAN:
		result.type = ScriptVariant::Boolean;
		result.b = (lua_toboolean( L, idx ) != 0);
		break;
	case LUA_TLIGHTUSERDATA:
		result.type = ScriptVariant::UserData;
		result.f = lua_tocfunction( L, idx );
		break;
	case LUA_TNUMBER:
		result.type = ScriptVariant::Number;
		result.n = lua_tonumber( L, idx );
		break;
	case LUA_TSTRING:
		result.type = ScriptVariant::String;
		result.s = lua_tolstring( L, idx, &len );
		mxUNUSED(len);
		break;
	case LUA_TTABLE:
		Unimplemented;
		break;
	case LUA_TFUNCTION:
		result.type = ScriptVariant::UserData;
		result.f = lua_tocfunction( L, idx );
		break;
	case LUA_TUSERDATA:
		Unimplemented;
		break;
	case LUA_TTHREAD:
		Unimplemented;
		break;
	mxNO_SWITCH_DEFAULT;
	}

	lua_pop( L, 1 );

	return result;
}

void Lua_EnsureGlobalTableExists( lua_State* L, const char* _table_name )
{
	lua_getglobal( L, _table_name );

	if( !lua_istable( L, -1 ) )
	{
		lua_pop( L, 1 ); // Pop the non-table.
		lua_newtable( L );
		lua_pushvalue( L, -1 );
		lua_setglobal( L, _table_name );
	}
}

void Lua_RegisterModule( lua_State* L, const LuaModule& module )
{
	LUA_DBG_CHECK_STACK(L);
	// Create a new table for module.
	lua_newtable( L );
	lua_pushvalue( L, -1 );
	lua_setglobal( L, module.name );	// __G["name"] = table

	// Register all the functions.
	for( int i = 0; i < module.numFunctions; i++ )
	{
		const luaL_Reg& f = module.functions[ i ];
		lua_pushcfunction( L, f.func );
		lua_setfield( L, -2, f.name );	// table["function_name"] = function
	}

	lua_pop( L, 1 );	// Pop the module table.
}

ERet AssignConVar( const AConsoleVariable* _convar, const ScriptVariant& _value )
{
	switch( _value.type )
	{
	case ScriptVariant::Type::Nil :
		//result.type = ScriptVariant::Nil;
		//result.U = 0;
		Unimplemented;
		break;
	case ScriptVariant::Type::Boolean :
		mxDO(AConsoleVariable::SetBool( _convar, c_cast(int)_value.b ));
		break;
	case ScriptVariant::Type::Function :
		//result.type = ScriptVariant::UserData;
		//result.f = lua_tocfunction( L, idx );
		Unimplemented;
		break;
	case ScriptVariant::Type::Number :
		mxDO(AConsoleVariable::SetInt( _convar, c_cast(int)_value.n ));
		break;
	case ScriptVariant::Type::String :
		//result.type = ScriptVariant::String;
		//result.s = lua_tolstring( L, idx, &len );
		//mxUNUSED(len);
		Unimplemented;
		break;
	case ScriptVariant::Type::LuaTable :
		Unimplemented;
		break;
	case ScriptVariant::Type::UserData :
		//result.type = ScriptVariant::UserData;
		//result.f = lua_tocfunction( L, idx );
		Unimplemented;
		break;
	case ScriptVariant::Type::LuaThread :
		Unimplemented;
		break;
	case ScriptVariant::Type::Undefined :
		Unimplemented;
		break;
	mxNO_SWITCH_DEFAULT;
	}
	return ALL_OK;
}


namespace MyLua
{
	ERet LoadChunk(
		lua_State* L

		, const char* chunk, size_t size

		/// the chunk name for debug information and error messages
		, const char* sourcefilename /* = NULL */
		)
	{
		LUA_TRY(luaL_loadbuffer(
			L
			, chunk, size
			, sourcefilename
			)
			, L
			);
		return ALL_OK;
	}

	ERet LoadChunk(
		const NwScript&	script
		, lua_State* L
		)
	{
		return LoadChunk(
			L
			, script.code.raw(), script.code.rawSize()
			, script.sourcefilename.c_str()
			);
	}

}//namespace MyLua
