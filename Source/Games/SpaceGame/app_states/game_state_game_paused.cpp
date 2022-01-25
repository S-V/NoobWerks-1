//
#include "stdafx.h"
#pragma hdrstop

#include <nativefiledialog/include/nfd.h>
#include <gainput/gainput.h>

#include <Engine/WindowsDriver.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>

#include "app_states/game_state_game_paused.h"
#include "game_app.h"


/*
-----------------------------------------------------------------------------
	SgGameState_GamePaused
-----------------------------------------------------------------------------
*/
SgGameState_GamePaused::SgGameState_GamePaused()
{
	is_window_open = true;
}

ERet SgGameState_GamePaused::Initialize(
									   )
{
	return ALL_OK;
}

void SgGameState_GamePaused::Shutdown()
{
}

EStateRunResult SgGameState_GamePaused::handleInput(
	NwAppI* game_app
	, const NwTime& game_time
)
{
	SgGameApp* game = checked_cast< SgGameApp* >( game_app );

	if( game->input_mgr.wasButtonDown( eGA_Exit ) )
	{
		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	//
	if(game->input_mgr.wasButtonDown( eGA_PauseGame ))
	{
		game->is_paused = false;

		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	return ContinueFurtherProcessing;
}

void SgGameState_GamePaused::DrawUI(
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
	const char* WINDOW_ID = "DoYouReallyWannaExit";

	//if( is_window_open )
	{
		//
		nk_context* ctx = Nuklear_getContext();


		// BEGIN transparent window
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
			//, nk_rect(
			//message_box_margins.x,
			//message_box_margins.y,
			//absolute_message_box_size.x,
			//absolute_message_box_size.y
			//)
			, NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_NO_INPUT
			) )
		{
			//nk_layout_space_begin(ctx, NK_STATIC, absolute_message_box_size.y, 2);

			//nk_layout_space_push(ctx,
			//	nk_rect(
			//	0,
			//	0,
			//	absolute_message_box_size.x,
			//	absolute_message_box_size.y
			//	)
			//);

			//nk_layout_row_static(
			//	ctx,
			//	absolute_message_box_size.y,
			//	absolute_message_box_size.x * 0.5f,
			//	2
			//	);
//nk_group_begin_titled(ctx,"")



#if 0

			nk_layout_space_begin(ctx, NK_STATIC, 200, INT_MAX);
			nk_layout_space_push(ctx, nk_rect(
				message_box_margins.x,
				message_box_margins.y,
				absolute_message_box_size.x,
				absolute_message_box_size.y
			));



			nk_label(ctx, nwLOCALIZE("Do you really want to exit?"), NK_TEXT_CENTERED);
			if( nk_button_label( ctx, "Yes" ) ) {
				G_RequestExit();
			}
			if( nk_button_label( ctx, "No" ) ) {
				//is_window_open = false;
				game->state_mgr.popState();
			}

			nk_layout_space_end(ctx);
#endif


			////
			//if( nk_begin(
			//	ctx
			//	, "fsfsfs"
			//	, nk_rect(
			//	message_box_margins.x,
			//	message_box_margins.y,
			//	absolute_message_box_size.x,
			//	absolute_message_box_size.y
			//	), NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_NO_INPUT
			//	) )
			//{
			//nk_label(ctx, nwLOCALIZE("Do you really want to exit?"), NK_TEXT_CENTERED);
			//if( nk_button_label( ctx, "Yes" ) ) {
			//	G_RequestExit();
			//}
			//if( nk_button_label( ctx, "No" ) ) {
			//	//is_window_open = false;
			//	game->state_mgr.popState();
			//}
			//}
			//nk_end(ctx);



		}

		// END transparent window
		nk_end(ctx);

		//
		ctx->style.window.fixed_background = prev_style_item;
	}

	//is_window_open = nk_window_is_closed(ctx, WINDOW_ID) == 0;
}
