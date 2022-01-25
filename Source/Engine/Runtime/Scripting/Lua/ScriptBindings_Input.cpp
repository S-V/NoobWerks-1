#include "Scripting_PCH.h"
#pragma hdrstop
#include <Input/Input.h>
#include <Scripting/Scripting.h>
#include <Scripting/Lua/Lua_Helpers.h>
#include <Scripting/Lua/ScriptBindings_Input.h>


extern "C"
{

// void bind( String key_name, String action_name );
int Wrapper_SetKeyBinding( lua_State* L )
{
	//// get number of arguments
	//int n = lua_gettop(L);
	//mxASSERT(n==2);
	const char* keyname = lua_tostring(L, 1);
	const char* binding = lua_tostring(L, 2);
	const EKeyCode keycode = FindKeyByName(keyname);
	if(keycode == KEY_Unknown) {
		ptERROR("Unknown key: %s", keyname);
	} else {
		UNDONE;
//		InputSystem::BindKey(keycode, binding);
	}
	// return the number of results
	return 0;
}

}//extern "C"

static const luaL_Reg gs_Input_functions[] =
{
	{ "Bind", &Wrapper_SetKeyBinding },
};

int ScriptModuleLoader_Input( lua_State* L )
{
	LuaModule	module;
	{
		module.name = "Input";
		module.functions = gs_Input_functions;
		module.numFunctions = mxCOUNT_OF(gs_Input_functions);
	}
	Lua_RegisterModule( L, module );

	return 1;
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
