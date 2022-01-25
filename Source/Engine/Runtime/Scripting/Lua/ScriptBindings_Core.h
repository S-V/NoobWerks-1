// engine => script communication
#pragma once

struct lua_State;

extern "C"{
	int Wrapper_ReloadAssetsOfType( lua_State* L );
}//extern "C"

int ScriptModuleLoader_Core( lua_State* L );

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
