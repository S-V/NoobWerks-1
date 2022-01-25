#include "Scripting_PCH.h"
#pragma hdrstop
#include <Core/Assets/AssetManagement.h>
#include <Scripting/Scripting.h>
#include <Scripting/Lua/Lua_Helpers.h>
#include <Scripting/Lua/ScriptBindings_Core.h>


extern "C"
{

// void ReloadAssetsOfType( String asset_type );
int Wrapper_ReloadAssetsOfType( lua_State* L )
{
	const ScriptVariant var = Lua_Pop( L );
	const TbMetaClass* metaClass = TypeRegistry::FindClassByName( var.ToStringPtr() );
	if( metaClass ) {
		UNDONE;
//		Assets::ReloadAssetsOfType( *metaClass );
	} else {
		ptWARN("Couldn't find type named '%s'\n", var.ToStringPtr());
	}
	return 0;
}

}//extern "C"

static const luaL_Reg gs_Core_functions[] =
{
	{ "ReloadAssetsOfType", &Wrapper_ReloadAssetsOfType },	
};

int ScriptModuleLoader_Core( lua_State* L )
{
	LuaModule	module;
	{
		module.name = "Core";
		module.functions = gs_Core_functions;
		module.numFunctions = mxCOUNT_OF(gs_Core_functions);
	}
	Lua_RegisterModule( L, module );

	return 1;
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
