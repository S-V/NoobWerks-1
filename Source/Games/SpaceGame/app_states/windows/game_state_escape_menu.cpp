//
#include "stdafx.h"
#pragma hdrstop

#include <nativefiledialog/include/nfd.h>
#include <gainput/gainput.h>

#include <Engine/WindowsDriver.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>

#include "app_states/app_states.h"
#include "game_app.h"


/*
-----------------------------------------------------------------------------
	MyGameState_EscapeMenu
-----------------------------------------------------------------------------
*/
MyGameState_EscapeMenu::MyGameState_EscapeMenu()
{
	should_navigate_back = false;
}

EStateRunResult MyGameState_EscapeMenu::handleInput(
	NwAppI* game_app
	, const NwTime& game_time
)
{
	SgGameApp* game = checked_cast< SgGameApp* >( game_app );

#if !GAME_CFG_RELEASE_BUILD
	if(game->input_mgr._input_system->getState().keyboard.held[KEY_Space])
	{
		G_RequestExit();
		return StopFurtherProcessing;
	}
#endif // !GAME_CFG_RELEASE_BUILD


	if(game->input_mgr.wasButtonDown( eGA_Exit ) || should_navigate_back)
	{
		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	if(should_navigate_to_options)
	{
		should_navigate_to_options = false;

		game->state_mgr.pushState(
			&game->states->game_options_menu
			);
		return StopFurtherProcessing;
	}

	return StopFurtherProcessing;
}

void MyGameState_EscapeMenu::DrawUI(
	const NwTime& game_time
	, NwAppI* game_app
	)
{
	SgGameApp* game = checked_cast< SgGameApp* >( game_app );

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
	{
		//
		nk_context* ctx = Nuklear_getContext();

		// BEGIN dark transparent window
		const nk_style_item prev_style_item = ctx->style.window.fixed_background;
		const struct nk_color gray = {0, 0, 0, 200};
		ctx->style.window.fixed_background = nk_style_item_color(gray);

		//
		if( nk_begin(
			ctx
			, "#EscapeMenu"
			, nk_rect(
				0,
				0,
				screen_width,
				screen_height
			)
			//, nk_rect(
			//message_box_margins.x,
			//message_box_margins.y,
			//absolute_message_box_size.x,
			//absolute_message_box_size.y
			//)
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
				if( nk_button_label( ctx, "BACK" ) )
				{
					should_navigate_back = true;
				}
				nk_spacing(ctx, UI_SPACING_COLS);

				//
				nk_spacing(ctx, UI_SPACING_COLS);
				if( nk_button_label( ctx, "RESTART" ) )
				{
					game->player_wants_to_start_again = true;

					should_navigate_back = true;
				}
				nk_spacing(ctx, UI_SPACING_COLS);


				//
				nk_spacing(ctx, UI_SPACING_COLS);
				if( nk_button_label( ctx, "OPTIONS" ) )
				{
					should_navigate_to_options = true;
				}
				nk_spacing(ctx, UI_SPACING_COLS);



				//
				nk_spacing(ctx, UI_SPACING_COLS);
				if( nk_button_label( ctx, "EXIT" ) )
				{
					G_RequestExit();
				}
				nk_spacing(ctx, UI_SPACING_COLS);



			}//draw button block
			//END button block

			nk_layout_space_end(ctx);




		}//draw transparent window
		// END transparent window
		nk_end(ctx);

		//
		ctx->style.window.fixed_background = prev_style_item;
	}
}
