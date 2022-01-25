#pragma once

#include <Core/Client.h>

///
class NwGameStateManager: NonCopyable
{
	TFixedArray< NwGameStateI*, 8 >		_game_state_stack;

public:
	NwGameStateManager();
	~NwGameStateManager();

	ERet Initialize();
	void Shutdown();

	// returns the top-level state (must always be valid!)
	NwGameStateI* getTopState();

	void pushState( NwGameStateI* new_state );
	NwGameStateI* popState();

	void handleInput(
		NwAppI* game
		, const NwTime& game_time
		);

	void DrawScene(
		NwAppI* game
		, const NwTime& game_time
		);

	void DrawUI(
		const NwTime& game_time
		, NwAppI* game
		);

private:
	void _applyState( NwGameStateI* state );

#if 0
        // Creating and destroying the state machine
        void Init();
        void Cleanup();

        // Transit between states
        void ChangeState(CGameState* state);
        void PushState(CGameState* state);
        void PopState();

        // The three important actions within a game loop
        // (these will be handled by the top state in the stack)
        void HandleEvents();
        void Update();
        void Draw();
#endif
};
