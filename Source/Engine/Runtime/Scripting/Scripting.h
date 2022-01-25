// The scripting module interface.
#pragma once

#include <Scripting/ForwardDecls.h>


namespace Scripting
{
	//ERet Initialize();
	//void Shutdown();

	//lua_State* GetLuaState();

	//ERet ExecuteBuffer( const char* str, int len );

	//struct LuaConfig : AConfigFile
	//{
	//	virtual const char* FindString( const char* _key ) const override;
	//	virtual bool FindInteger( const char* _key, int &_value ) const override;
	//	virtual bool FindSingle( const char* _key, float &_value ) const override;
	//	virtual bool FindBoolean( const char* _key, bool &_value ) const override;
	//public:
	//	LuaConfig( lua_State* L ) : m_L(L) {}
	//private:
	//	lua_State* m_L;
	//};
}//namespace Scripting

/// scripting engine interface
class NwScriptingEngineI
{
public:
	static ERet Create(
		NwScriptingEngineI *&scripting_engine_
		, AllocatorI& allocator
		);
	static void Destroy(
		NwScriptingEngineI *scripting_engine
		);

public:
	virtual lua_State* GetLuaState() = 0;

	virtual void TickOncePerFrame(const NwTime& game_time) = 0;

public:
	/// Creates a new thread and adds it to the list
	/// (but does not run it straight away).

protected:
	virtual ~NwScriptingEngineI() {}
};
