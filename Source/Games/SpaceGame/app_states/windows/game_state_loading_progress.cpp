//
#include "stdafx.h"
#pragma hdrstop

#include <nativefiledialog/include/nfd.h>
#include <gainput/gainput.h>

#include <Engine/WindowsDriver.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>

#include "app_states/windows/game_state_loading_progress.h"
#include "app_states/app_states.h"
#include "game_app.h"


/*
-----------------------------------------------------------------------------
	MyGameState_LoadingProgress
-----------------------------------------------------------------------------
*/
MyGameState_LoadingProgress::MyGameState_LoadingProgress()
{
	is_window_open = true;
}

ERet MyGameState_LoadingProgress::Initialize(
									   )
{
	return ALL_OK;
}

void MyGameState_LoadingProgress::Shutdown()
{
}

EStateRunResult MyGameState_LoadingProgress::handleInput(
	NwAppI* game_app
	, const NwTime& game_time
)
{
	return StopFurtherProcessing;
}

void MyGameState_LoadingProgress::DrawUI(
	const NwTime& game_time
	, NwAppI* game_app
	)
{
	SgGameApp& game = *checked_cast< SgGameApp* >( game_app );

	//
	const nk_size MAX_LOADING_PROGRESS_VALUE = 100;
	nk_size curr_loading_progress_percent = game.current_loading_progress * MAX_LOADING_PROGRESS_VALUE;

	//
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

		// BEGIN window
		const nk_style_item prev_style_item = ctx->style.window.fixed_background;
		const struct nk_color black_color = {0, 0, 0, 255};
		ctx->style.window.fixed_background = nk_style_item_color(black_color);

		//
		if( nk_begin(
			ctx
			, "HALF-ASSED"
			, nk_rect(
				0,
				0,
				screen_width,
				screen_height
			)
			, NK_WINDOW_TITLE|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_NO_INPUT
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
				const struct nk_color GREEN_COLOR = {0,255,0,255};

				String32	loading_progress_text;
				Str::Format(loading_progress_text, "LOADING: %d %%", (int)curr_loading_progress_percent);
				nk_label_colored( ctx, loading_progress_text.c_str(), NK_TEXT_CENTERED, GREEN_COLOR );

				nk_spacing(ctx, UI_SPACING_COLS);


				//
				enum {
					PROG_BAR_SPACING_COLS = 1,
					PROG_BAR_NUM_COLS = PROG_BAR_SPACING_COLS + 5 + PROG_BAR_SPACING_COLS,
				};

				nk_layout_row_dynamic(
					ctx
					, UI_BUTTON_HEIGHT	// item height
					, 1
					);

				nk_progress(
					ctx
					, &curr_loading_progress_percent
					, MAX_LOADING_PROGRESS_VALUE
					, NK_FIXED
					);
			}
			nk_layout_space_end(ctx);
		}

		// END window
		nk_end(ctx);

		//
		ctx->style.window.fixed_background = prev_style_item;
	}
}
