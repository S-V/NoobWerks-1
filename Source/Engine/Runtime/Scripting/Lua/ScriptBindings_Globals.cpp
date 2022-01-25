#include "Scripting_PCH.h"
#pragma hdrstop
#include <Core/Assets/AssetManagement.h>
#include <Core/ConsoleVariable.h>
#include <Scripting/Scripting.h>
#include <Scripting/Lua/Lua_Helpers.h>
#include <Scripting/Lua/ScriptBindings_Globals.h>

extern "C"
{

/* Variable arg function callable from a Lua script as:
*   LOG('A', 'gaggle', 'of', 5, 'args')
*/
static int W_LOG( lua_State* L )
{
	const int numArgs = lua_gettop( L );

	LogStream	log(LL_Info);
	for( int idx = 1; idx <= numArgs; idx++ )
	{
		if( !lua_isstring( L, idx ) ) {
			lua_pushstring(L, "LOG() error: argument must be a string or a number");
			lua_error(L);
			continue;
		}
		size_t l;
		const char* s = lua_tolstring( L, idx, &l );
		log.VWrite( s, l );
	}

	return 0;
}

// set console variable: setvar( 'g_count', 7 )
static int W_setvar( lua_State* L )
{
	const int numArgs = lua_gettop( L );
	mxASSERT(numArgs == 2);

	const char* name = lua_tostring( L, 1 );

	const AConsoleVariable* conVar = AConsoleVariable::FindCVar( name );
	if( conVar )
	{
		int type = lua_type( L, -1 );
		mxASSERT2(type != LUA_TNONE, "Lua stack is empty");

		const ScriptVariant value = Lua_Pop( L );

		AssignConVar( conVar, value );
	}
	else
	{
		ptERROR("Undefined variable: '%s'", name);
	}

	return 0;
}

}//extern "C"

void Script_RegisterGlobals( lua_State* L )
{
	lua_register( L, "LOG", &W_LOG );
	lua_register( L, "setvar", &W_setvar );
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
