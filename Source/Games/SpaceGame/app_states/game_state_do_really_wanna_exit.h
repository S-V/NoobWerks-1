#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>

/*
-----------------------------------------------------------------------------
	SgGameState_DoYouReallyWannaExit
-----------------------------------------------------------------------------
*/
class SgGameState_DoYouReallyWannaExit: public NwGameStateI
{
	bool is_window_open;

public:
	SgGameState_DoYouReallyWannaExit();

	ERet Initialize(
		);

	void Shutdown();

	virtual EStateRunResult handleInput(
		NwAppI* game_app
		, const NwTime& game_time
		) override;

	virtual void DrawUI(
		const NwTime& game_time
		, NwAppI* game_app
		) override;

	virtual bool pausesGame() const override
	{return true;}
};
