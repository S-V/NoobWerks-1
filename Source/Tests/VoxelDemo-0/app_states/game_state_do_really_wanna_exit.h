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
		NwAppI* app
		, const NwTime& game_time
		) override;

	virtual void DrawUI(
		const NwTime& game_time
		, NwAppI* app
		) override;

	virtual bool pausesGame() const override
	{return true;}
};
