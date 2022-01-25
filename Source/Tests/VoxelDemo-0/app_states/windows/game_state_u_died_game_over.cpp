//
#include "stdafx.h"
#pragma hdrstop

#include <nativefiledialog/include/nfd.h>
#include <gainput/gainput.h>

#include <Engine/WindowsDriver.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>

#include "app_states/windows/game_state_u_died_game_over.h"
#include "app_states/game_states.h"
#include "app.h"


/*
-----------------------------------------------------------------------------
	MyGameState_UDiedGameOver
-----------------------------------------------------------------------------
*/
MyGameState_UDiedGameOver::MyGameState_UDiedGameOver()
{
	is_window_open = true;
}

ERet MyGameState_UDiedGameOver::Initialize(
									   )
{
	return ALL_OK;
}

void MyGameState_UDiedGameOver::Shutdown()
{
}

EStateRunResult MyGameState_UDiedGameOver::handleInput(
	NwAppI* app
	, const NwTime& game_time
)
{
	MyApp* game = checked_cast< MyApp* >( app );

//#if !GAME_CFG_RELEASE_BUILD
//	if(
//		game->player.comp_health.IsAlive()
//		||
//		game->input.wasButtonDown( eGA_Exit )
//		)
//	{
//		game->state_mgr.popState();
//		return StopFurtherProcessing;
//	}
//#endif // !GAME_CFG_RELEASE_BUILD

	//
	const gainput::InputState& keyboard_input_state =
		*game->input._keyboard->GetInputState()
		;

	if(
		keyboard_input_state.GetBool( gainput::KeyReturn )
		||
		keyboard_input_state.GetBool( gainput::KeyKpEnter )
		||
		keyboard_input_state.GetBool( gainput::KeySpace )
		||
		keyboard_input_state.GetBool( gainput::KeyY )
		)
	{
		G_RequestExit();
		return StopFurtherProcessing;
	}

	return ContinueFurtherProcessing;
}

void MyGameState_UDiedGameOver::DrawUI(
	const NwTime& game_time
	, NwAppI* app
	)
{
	MyApp* game = checked_cast< MyApp* >( app );

	const UInt2 window_size = WindowsDriver::getWindowSize();
	const float screen_width = window_size.x;
	const float screen_height = window_size.y;

	//
	const V2f relative_message_box_size = V2f(0.2);
	const V2f absolute_message_box_size = V2f(
		screen_width * relative_message_box_size.x,
		screen_height * relative_message_box_size.y
		);
	const V2f message_box_margins = V2f(
		screen_width - absolute_message_box_size.x,
		screen_height - absolute_message_box_size.y
		) * 0.5f;

	//
	const char* WINDOW_ID = "UDiedGameOver";

	{
		//
		nk_context* ctx = Nuklear_getContext();

		// BEGIN dark transparent window
		const nk_style_item prev_style_item = ctx->style.window.fixed_background;
		const struct nk_color gray = {0, 0, 0, 0x7F};
		ctx->style.window.fixed_background = nk_style_item_color(gray);

		//
		if( nk_begin(
			ctx
			, WINDOW_ID
			, nk_rect(
				0,
				0,
				screen_width,
				screen_height
			)
			, NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_NO_INPUT
			) )
		{
			//
			nk_layout_space_begin(
				ctx
				, NK_DYNAMIC
				, screen_height * 0.3f	// height from top of the screen
				, 1	// widget count in row (or col count)
				);
			{
				nk_layout_row_dynamic(
					ctx
					, UI_BUTTON_HEIGHT	// item height
					, UI_NUM_ITEMS_IN_ROW // == 5: 3 widges in a row (2*padding + button + 2*padding) == num of cols
					);

				//
				nk_spacing(ctx, UI_SPACING_COLS);
				const struct nk_color RED_COLOR = {255,0,0,255};
				nk_label_colored( ctx, "GAME OVER - YOU DIED!", NK_TEXT_CENTERED, RED_COLOR );
				nk_spacing(ctx, UI_SPACING_COLS);

				//
				nk_spacing(ctx, UI_SPACING_COLS);
				if( nk_button_label( ctx, "EXIT" ) ) {
					G_RequestExit();
				}
				nk_spacing(ctx, UI_SPACING_COLS);
			}
			nk_layout_space_end(ctx);
		}

		// END transparent window
		nk_end(ctx);

		//
		ctx->style.window.fixed_background = prev_style_item;
	}
}
