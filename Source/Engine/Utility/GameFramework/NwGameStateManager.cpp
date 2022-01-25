#include <Base/Base.h>
#pragma hdrstop
#include <typeinfo.h>	// typeid()

#include <ImGui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>

#include <Base/Template/Algorithm/Search.h>	// LinearSearch
#include <Engine/WindowsDriver.h>
#include <Utility/GameFramework/NwGameStateManager.h>
#include <Utility/GameFramework/NwGameStateManager.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>
#include <Utility/GUI/ImGui/ImGUI_Renderer.h>


NwGameStateManager::NwGameStateManager()
{
	Arrays::setAll( _game_state_stack, nil );
}

NwGameStateManager::~NwGameStateManager()
{}

ERet NwGameStateManager::Initialize()
{
	return ALL_OK;
}

void NwGameStateManager::Shutdown()
{
	//mxASSERT(_game_state_stack.IsEmpty());
}

NwGameStateI* NwGameStateManager::getTopState()
{
	NwGameStateI* state = _game_state_stack.getLast();
	return state;
}

void NwGameStateManager::pushState( NwGameStateI* new_state )
{
	if(mxSUCCEDED(_game_state_stack.add( new_state )))
	{
		//DBGOUT("pushState(%s): mask=%d", typeid(*state).name(), getTopState()->GetStateMask());
		_applyState( new_state );
	}
}

NwGameStateI* NwGameStateManager::popState()
{
	NwGameStateI* old_state = _game_state_stack.getLast();
	old_state->onWillBecomeInactive();
	_game_state_stack.Pop();
	//DBGOUT("pushState(%s): mask=%d",  typeid(*state).name(), getTopState()->GetStateMask());

	NwGameStateI* new_state = _game_state_stack.getLast();
	_applyState( new_state );

	return old_state;
}

void NwGameStateManager::_applyState( NwGameStateI* state )
{
	state->onWillBecomeActive();

	//
	NwInputSystemI& input_system = NwInputSystemI::Get();

	input_system.setRelativeMouseMode(
		state->pinsMouseCursorToCenter()
		);

	input_system.setMouseCursorHidden(
		state->hidesMouseCursor()
		);
}

void NwGameStateManager::handleInput(
	NwAppI* game
	, const NwTime& game_time
	)
{
	mxASSERT(!_game_state_stack.IsEmpty());

	for( int i = _game_state_stack.num() - 1;
		i >= 0;
		i-- )
	{
		NwGameStateI* game_state = _game_state_stack[i];

		const EStateRunResult result = game_state->handleInput(
			game
			, game_time
			);

		if( result == StopFurtherProcessing ) {
			break;
		}
	}
}

void NwGameStateManager::DrawScene(
	NwAppI* game
	, const NwTime& game_time
	)
{
	for( int i = _game_state_stack.num() - 1; i >= 0; i-- )
	{
		NwGameStateI* game_state = _game_state_stack[i];

		const EStateRunResult result = game_state->DrawScene(
			game
			, game_time
			);

		if( result == StopFurtherProcessing ) {
			break;
		}
	}
}

void NwGameStateManager::DrawUI(
	const NwTime& game_time
	, NwAppI* game
	)
{
	this->getTopState()->DrawUI(
		game_time
		, game
		);
}
