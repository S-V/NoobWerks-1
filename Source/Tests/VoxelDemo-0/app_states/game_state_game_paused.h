#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>

/*
-----------------------------------------------------------------------------
	RTSGameState_GamePaused
-----------------------------------------------------------------------------
*/
class RTSGameState_GamePaused: public NwGameStateI
{
	bool is_window_open;

public:
	RTSGameState_GamePaused();

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

	virtual bool hidesMouseCursor() const
	{return true;}

	virtual bool allowsFirstPersonCameraToReceiveInput() const
	{return false;}
};
