//
#pragma once

#include <Scripting/ForwardDecls.h>
#include <Scripting/Private/ScriptScheduler.h>


class LuaScriptingEngine: public NwScriptingEngineI
{
public:
	lua_State*	_L;

	NwScriptScheduler	_sched;

	//
	struct Coroutine
	{
		lua_State *	L;
		U32			num_calls;
	};
	TFixedArray< Coroutine,4 >	_coroutines;

	//
	AllocatorI& _allocator;

public:
	LuaScriptingEngine(AllocatorI& allocator);

	ERet Setup();
	void Close();

	virtual lua_State* GetLuaState() override
	{
		return _L;
	}

	virtual void TickOncePerFrame(const NwTime& game_time) override;

public:
	ERet StartCoroutine(
		const NwScript&	script
		, const char* entry_point
		);

public:
	bool _ShouldRunCoroutine(
		const Coroutine& coroutine
		, const NwTime& game_time
		) const;

	ERet _RunCoroutine(
		Coroutine & coroutine
		, const NwTime& game_time
		);

public:
	void* LuaAlloc( void *ptr, size_t osize, size_t nsize );
	void* _Realloc( void* old_ptr, size_t old_size, size_t new_size );
};
