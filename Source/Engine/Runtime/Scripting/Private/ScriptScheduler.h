//
#pragma once

#include <Core/Assets/AssetManagement.h>

#include <Scripting/ForwardDecls.h>
#include <Base/Memory/FreeList/FreeList.h>
#include <Core/Client.h>

#include <Scripting/Lua/Lua_Helpers.h>


// represents compiled bytecode ready for execution
class NwScriptScheduler: NonCopyable
{
	struct ScheduledScript: TLinkedList<ScheduledScript>
	{
		const NwScript*	script;
	};

	FreeListAllocator		_scheduled_scripts_pool;

	/// scripts that run periodically
	ScheduledScript::Head	_scheduled_repetitive_scripts;

public:
	NwScriptScheduler();

	ERet Setup();
	void Close();

	void TickOncePerFrame(const NwTime& game_time);

	ERet AddPeriodicScript(
		const NwScript&	script
		, const char* entry_point

		/// 10 Hz by default
		, const UINT frequency_msec = 100
		);

private:
	bool ShouldExecuteScript(
		ScheduledScript& scheduled_script
		, const NwTime& game_time
		);
	ERet ExecuteScript(
		ScheduledScript& scheduled_script
		, const NwTime& game_time
		);
};
