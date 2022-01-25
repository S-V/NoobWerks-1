#include "Scripting_PCH.h"
#pragma hdrstop

#include <Scripting/ScriptResources.h>
#include <Scripting/Private/ScriptScheduler.h>


NwScriptScheduler::NwScriptScheduler()
{
	_scheduled_repetitive_scripts = nil;
}

ERet NwScriptScheduler::Setup()
{
	mxDO(_scheduled_scripts_pool.Initialize(
		sizeof(ScheduledScript)
		, 64
		, &MemoryHeaps::scripting()
		));
	return ALL_OK;
}

void NwScriptScheduler::Close()
{
	_scheduled_scripts_pool.releaseMemory();
}

void NwScriptScheduler::TickOncePerFrame(const NwTime& game_time)
{
	ScheduledScript* curr_scheduled_script = _scheduled_repetitive_scripts;
	while(curr_scheduled_script)
	{
		ScheduledScript* next_scheduled_script = curr_scheduled_script->_next;

		if(ShouldExecuteScript(*curr_scheduled_script, game_time))
		{
			ExecuteScript(*curr_scheduled_script, game_time);
		}

		curr_scheduled_script = next_scheduled_script;
	}
}

ERet NwScriptScheduler::AddPeriodicScript(
	const NwScript&	script
	, const char* entry_point
	, const UINT frequency_msec
	)
{
	ScheduledScript*	new_scheduled_script;
	mxDO(nwCREATE_OBJECT(
		new_scheduled_script
		, MemoryHeaps::scripting() // _scheduled_scripts_pool
		, ScheduledScript
		));

	new_scheduled_script->script = &script;

	new_scheduled_script->PrependSelfToList(&_scheduled_repetitive_scripts);

	return ALL_OK;
}

bool NwScriptScheduler::ShouldExecuteScript(
	ScheduledScript& scheduled_script
	, const NwTime& game_time
	)
{
	return true;
}

ERet NwScriptScheduler::ExecuteScript(
									  ScheduledScript& scheduled_script
									  , const NwTime& game_time
									  )
{
	return ALL_OK;
}
