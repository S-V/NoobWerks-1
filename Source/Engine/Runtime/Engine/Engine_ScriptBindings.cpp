#include <Base/Base.h>
#pragma hdrstop

#include <Core/Memory.h>

#include <Scripting/Scripting.h>
#include <Scripting/Lua/FunctionBinding.h>
#include <Scripting/Lua/LuaBridgeUtil.h>

#include <Engine/Engine_ScriptBindings.h>
#include <Engine/Engine.h>
#include <Engine/WindowsDriver.h>

namespace Engine
{

mxREFACTOR("move");
static void memstat()
{
	//Heaps::DumpStats();
}
static void API_sizeof( const char* _class_name )
{
	const TbMetaClass* metaclass = TypeRegistry::FindClassByName( _class_name );
	if( !metaclass ) {
		ptWARN("No class named '%s'", _class_name);
		return;
	}
	ptPRINT("%d", metaclass->GetInstanceSize());
}

//static void Log(const char* foo)
//{
//}

ERet ExposeToLua()
{
	//lua_State* L = g_engine.scripting->GetInternalState();

	//luabridge::getGlobalNamespace(L)
	//	.addFunction("log", Log)
	//	;

	//UNDONE;
	//lua_State* L = Scripting::GetLuaState();
	//lua_register( L, "memstat", TO_LUA_CALLBACK(memstat) );
	//lua_register( L, "sizeof", TO_LUA_CALLBACK(API_sizeof) );
	//lua_register( L, "exit", TO_LUA_CALLBACK(G_RequestExit) );
	return ALL_OK;
}

}//namespace Engine
