#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>

/*
-----------------------------------------------------------------------------
	RTSGameState_DoYouReallyWannaExit
-----------------------------------------------------------------------------
*/
class RTSGameState_DoYouReallyWannaExit: public NwGameStateI
{
	bool is_window_open;

public:
	RTSGameState_DoYouReallyWannaExit();

	ERet Initialize(
		);

	void Shutdown();

	virtual EStateRunResult handleInput(
		NwGameI* game_app
		, const NwTime& game_time
		) override;

	virtual void drawUI(
		float delta_time_in_seconds
		, NwGameI* game_app
		) override;

	virtual bool pausesGame() const override
	{return true;}
};
