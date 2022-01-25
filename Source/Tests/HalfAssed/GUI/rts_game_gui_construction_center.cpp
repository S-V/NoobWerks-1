//
#include "stdafx.h"
#pragma hdrstop

#include <Core/Memory/MemoryHeaps.h>
#include <Engine/WindowsDriver.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>

#include "Localization/game_localization.h"

const char* GAME_WINDOW_TITLE__CONSTRUCTION_CENTER = "#ConstructionCenter";

struct media
{
    //struct nk_font *font_14;
    //struct nk_font *font_18;
    //struct nk_font *font_20;
    //struct nk_font *font_22;

    //struct nk_image unchecked;
    //struct nk_image checked;
    //struct nk_image rocket;
    //struct nk_image cloud;
    //struct nk_image pen;
    //struct nk_image play;
    //struct nk_image pause;
    //struct nk_image stop;
    //struct nk_image prev;
    //struct nk_image next;
    //struct nk_image tools;
    //struct nk_image dir;
    //struct nk_image copy;
    //struct nk_image convert;
    //struct nk_image del;
    //struct nk_image edit;
    //struct nk_image images[9];
    //struct nk_image menu[6];
};

/* ===============================================================
 *
 *                          BUTTON DEMO
 *
 * ===============================================================*/
static void
ui_header(struct nk_context *ctx, struct media *media, const char *title)
{
    //nk_style_set_font(ctx, &media->font_18->handle);
    nk_layout_row_dynamic(ctx, 20, 1);
    nk_label(ctx, title, NK_TEXT_LEFT);
}

static void
ui_widget(struct nk_context *ctx, struct media *media, float height)
{
    static const float ratio[] = {0.15f, 0.85f};
    //nk_style_set_font(ctx, &media->font_22->handle);
    nk_layout_row(ctx, NK_DYNAMIC, height, 2, ratio);
    nk_spacing(ctx, 1);
}

static void
ui_widget_centered(struct nk_context *ctx, struct media *media, float height)
{
    static const float ratio[] = {0.15f, 0.50f, 0.35f};
    //nk_style_set_font(ctx, &media->font_22->handle);
    nk_layout_row(ctx, NK_DYNAMIC, height, 3, ratio);
    nk_spacing(ctx, 1);
}

/* ===============================================================
 *
 *                          BASIC DEMO
 *
 * ===============================================================*/
static void
basic_demo(struct nk_context *ctx, struct media *media)
{
    static int image_active;
    static int check0 = 1;
    static int check1 = 0;
    static size_t prog = 80;
    static int selected_item = 0;
    static int selected_image = 3;
    static int selected_icon = 0;
    static const char *items[] = {"Item 0","item 1","item 2"};
    static int piemenu_active = 0;
    static struct nk_vec2 piemenu_pos;

    int i = 0;
    //nk_style_set_font(ctx, &media->font_20->handle);
    nk_begin(ctx, "Basic Demo", nk_rect(320, 50, 275, 610),
        NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_TITLE);

    /*------------------------------------------------
     *                  POPUP BUTTON
     *------------------------------------------------*/
    ui_header(ctx, media, "Popup & Scrollbar & Images");
    ui_widget(ctx, media, 35);
    //if (nk_button_image_label(ctx, media->dir, "Images", NK_TEXT_CENTERED))
    //    image_active = !image_active;

    /*------------------------------------------------
     *                  SELECTED IMAGE
     *------------------------------------------------*/
    ui_header(ctx, media, "Selected Image");
    ui_widget_centered(ctx, media, 100);
//    nk_image(ctx, media->images[selected_image]);

    /*------------------------------------------------
     *                  IMAGE POPUP
     *------------------------------------------------*/
    //if (image_active) {
    //    if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Image Popup", 0, nk_rect(265, 0, 320, 220))) {
    //        nk_layout_row_static(ctx, 82, 82, 3);
    //        for (i = 0; i < 9; ++i) {
    //            if (nk_button_image(ctx, media->images[i])) {
    //                selected_image = i;
    //                image_active = 0;
    //                nk_popup_close(ctx);
    //            }
    //        }
    //        nk_popup_end(ctx);
    //    }
    //}
    /*------------------------------------------------
     *                  COMBOBOX
     *------------------------------------------------*/
    ui_header(ctx, media, "Combo box");
    ui_widget(ctx, media, 40);
    if (nk_combo_begin_label(ctx, items[selected_item], nk_vec2(nk_widget_width(ctx), 200))) {
        nk_layout_row_dynamic(ctx, 35, 1);
        for (i = 0; i < 3; ++i)
            if (nk_combo_item_label(ctx, items[i], NK_TEXT_LEFT))
                selected_item = i;
        nk_combo_end(ctx);
    }

    //ui_widget(ctx, media, 40);
    //if (nk_combo_begin_image_label(ctx, items[selected_icon], media->images[selected_icon], nk_vec2(nk_widget_width(ctx), 200))) {
    //    nk_layout_row_dynamic(ctx, 35, 1);
    //    for (i = 0; i < 3; ++i)
    //        if (nk_combo_item_image_label(ctx, media->images[i], items[i], NK_TEXT_RIGHT))
    //            selected_icon = i;
    //    nk_combo_end(ctx);
    //}

    /*------------------------------------------------
     *                  CHECKBOX
     *------------------------------------------------*/
    ui_header(ctx, media, "Checkbox");
    ui_widget(ctx, media, 30);
    nk_checkbox_label(ctx, "Flag 1", &check0);
    ui_widget(ctx, media, 30);
    nk_checkbox_label(ctx, "Flag 2", &check1);

    /*------------------------------------------------
     *                  PROGRESSBAR
     *------------------------------------------------*/
    ui_header(ctx, media, "Progressbar");
    ui_widget(ctx, media, 35);
    nk_progress(ctx, &prog, 100, nk_true);

    /*------------------------------------------------
     *                  PIEMENU
     *------------------------------------------------*/
    if (nk_input_is_mouse_click_down_in_rect(&ctx->input, NK_BUTTON_RIGHT,
        nk_window_get_bounds(ctx),nk_true)){
        piemenu_pos = ctx->input.mouse.pos;
        piemenu_active = 1;
    }

    //if (piemenu_active) {
    //    int ret = ui_piemenu(ctx, piemenu_pos, 140, &media->menu[0], 6);
    //    if (ret == -2) piemenu_active = 0;
    //    if (ret != -1) {
    //        fprintf(stdout, "piemenu selected: %d\n", ret);
    //        piemenu_active = 0;
    //    }
    //}
    //nk_style_set_font(ctx, &media->font_14->handle);
    nk_end(ctx);
}

NkFileBrowserI * bro = 0;

ERet RTS_Game_GUI_draw_Screen__construction_center(
	const InputState& input_state
	)
{
	const float screen_width = input_state.window.width;
	const float screen_height = input_state.window.height;

	//
	nk_context* ctx = Nuklear_getContext();

#if 0
	if( !bro ) {
		bro = NkFileBrowserI::create( MemoryHeaps::global() );
	}
	bro->drawUI();

	return ALL_OK;
#endif


	//GAME_WINDOW_TITLE__CONSTRUCTION_CENTER
	//nk_window_show(
	//	nk_ctx
	//	, GAME_WINDOW_TITLE__CONSTRUCTION_CENTER
	//	, NK_SHOWN
	//	);

	if( nk_begin_titled(
		ctx
		, GAME_WINDOW_TITLE__CONSTRUCTION_CENTER
		, nwLOCALIZE("Construction Center")
		, nk_rect( 0, 0, input_state.window.width, input_state.window.height )
		, NK_WINDOW_CLOSABLE|NK_WINDOW_TITLE|NK_WINDOW_NO_SCROLLBAR
		) )
	{
		// Menu Bar.
		{
			float spacing_x = ctx->style.window.spacing.x;

			/* output path directory selector in the menubar */
			ctx->style.window.spacing.x = 0;
			nk_menubar_begin(ctx);
			{
				nk_layout_row_dynamic(ctx, 25, 6);
				if( nk_button_label( ctx, "Load from File" ) ) {
					DBGOUT("Load from File");
				}
			}
			nk_menubar_end(ctx);
			ctx->style.window.spacing.x = spacing_x;
		}

		/* window layout */
		const struct nk_rect total_space = nk_window_get_content_region(ctx);

		static float ratio[] = {0.8f, NK_UNDEFINED};
		nk_layout_row(ctx, NK_DYNAMIC, total_space.h, 2, ratio);

		//
		nk_group_begin(ctx, "Editor View", 0);
		{
			nk_layout_row_dynamic(ctx, total_space.h, 1);

			//
			nk_color color = {0,1,0,1};
			nk_button_color(ctx, color);

			if (nk_button_label(ctx, "Editor button1")) {
				DBGOUT("button1 pressed\n");
			}
		}
		nk_group_end(ctx);

		//
		nk_group_begin(ctx, "Special", NK_WINDOW_NO_SCROLLBAR);
		{
			nk_layout_row_dynamic(ctx, 40, 1);
			if (nk_button_label(ctx, "home"))
				//file_browser_reload_directory_content(browser, browser->home)
			{}
			if (nk_button_label(ctx, "desktop"))
				//file_browser_reload_directory_content(browser, browser->desktop)
			{}
			if (nk_button_label(ctx, "computer"))
				//file_browser_reload_directory_content(browser, "/")
			{}
		}
		nk_group_end(ctx);






#if 0
		// first row with height 30 composed of 1 widget
		nk_layout_row_dynamic(
			ctx, screen_height * ((4.0f/5.0f) * 0.8f), 1
			);

		if( nk_group_begin_titled(
			ctx
			, nwNUKLEAR_GEN_ID
			, "Editor"
			, 0//NK_WINDOW_SCALABLE
			) )
		{
			// [... widgets ...]
			nk_layout_row_static(ctx, 30, 80, 1);

			if (nk_button_label(ctx, "button")) {
				DBGOUT("button0 pressed\n");
			}

			nk_group_end(ctx);
		}



		// second row uses 0 for height which will use auto layouting
		nk_layout_row_dynamic(
			ctx, 0, 1
			);

	//	nk_layout_row_dynamic(
	//		ctx, screen_height * (1.0f/5.0f), 1
	//		);

		if( nk_group_begin_titled(
			ctx
			, nwNUKLEAR_GEN_ID
			, "Editor"
			, 0
			) )
		{
			// [... widgets ...]

			nk_layout_row_static(ctx, 30, 80, 1);

			if (nk_button_label(ctx, "button1")) {
				DBGOUT("button1 pressed\n");
			}

			nk_group_end(ctx);
		}
#endif
	}

	nk_end(ctx);



	////
	//media	m;
	//basic_demo(ctx, &m);

	return ALL_OK;
}
