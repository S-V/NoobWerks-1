//
#include "stdafx.h"
#pragma hdrstop

#include <nativefiledialog/include/nfd.h>
#include <gainput/gainput.h>

#include <Core/Text/TextWriter.h>
#include <Engine/WindowsDriver.h>
#include <Utility/GUI/Nuklear/Nuklear_Support.h>
#include <Utility/GUI/ImGui/ImGUI_Renderer.h>

#include "game_states/game_states.h"
#include "FPSGame.h"


/*
-----------------------------------------------------------------------------
	MyGameState_GameOptionsMenu
-----------------------------------------------------------------------------
*/
MyGameState_GameOptionsMenu::MyGameState_GameOptionsMenu()
{
}

void MyGameState_GameOptionsMenu::onWillBecomeInactive()
{
	if(settings_did_changed)
	{
		settings_did_changed = false;
		
		FPSGame::Get().SaveUserDefinedInGameSettingsToFile();
	}
}

void MyGameState_GameOptionsMenu::tick(
								float delta_time_in_seconds
								, NwGameI* game_app
								)
{
	ImGui_Renderer::UpdateInputs();
}

EStateRunResult MyGameState_GameOptionsMenu::handleInput(
	NwGameI* game_app
	, const NwTime& game_time
)
{
	FPSGame* game = checked_cast< FPSGame* >( game_app );

	if(game->input.wasButtonDown( eGA_Exit ) || should_navigate_back)
	{
		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	return ContinueFurtherProcessing;
}

void MyGameState_GameOptionsMenu::drawUI(
	float delta_time_in_seconds
	, NwGameI* game_app
	)
{
	FPSGame* game = checked_cast< FPSGame* >( game_app );

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
		String128	window_title;
		StringWriter	str_wr(window_title);
		TextWriter		txt_wr(str_wr);

		for(int i = 0; i < 16; i++ ) {
			txt_wr.Emit("    ");
		}
		txt_wr.Emit("HALF-ASSED GAME OPTIONS");

		//
		if( nk_begin(
			ctx
			, window_title.c_str()
			, nk_rect(
				0,
				0,
				screen_width,
				screen_height
			)
			, NK_WINDOW_TITLE
			|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_NO_INPUT
			) )
		{
#if 0
			const int num_cols = 8;
			//
			nk_layout_space_begin(
				ctx
				, NK_DYNAMIC
				, screen_height * 0.1f	// height from top of the screen
				, 1	// widget count in row (or col count)
				);

			nk_layout_row_dynamic(
				ctx
				, 40	// item height
				, num_cols
				);

			nk_spacing(ctx, 1);
			if( nk_button_label( ctx, "BACK" ) )
			{
				should_navigate_back = true;
			}
			nk_spacing(ctx, num_cols-2);
#endif

#if 0
			nk_layout_row_static(ctx, 40, screen_width/8, 1);
			nk_spacing(ctx, 1);	// vertical spacing

			nk_layout_space_push(ctx, 40, screen_width/8, 1);
			if( nk_button_label( ctx, "BACK" ) )
			{
				should_navigate_back = true;
			}
#endif

			const float approx_window_title_height = 30;

			const float margin_x = screen_width/32;
			const float margin_y = screen_height/32;

			const float button_height = UI_BUTTON_HEIGHT;

			nk_layout_space_begin(ctx, NK_STATIC, UI_BUTTON_HEIGHT, 1);
			nk_layout_space_push(ctx, nk_rect(
				margin_x,
				margin_y,
				screen_width/8,	// button width
				button_height
				));
			if( nk_button_label( ctx, "BACK" ) )
			{
				should_navigate_back = true;
			}


			//
			NwViewport imgui_viewport;
			imgui_viewport.x = margin_x;
			imgui_viewport.y = approx_window_title_height + margin_y + button_height + margin_y;
			imgui_viewport.width = screen_width - 2*(margin_x);
			imgui_viewport.height = screen_height - (imgui_viewport.y + largest(margin_x, margin_y));

			_Draw_ImGui_Stuff(
				delta_time_in_seconds
				, game_app
				, imgui_viewport
				);

		}//draw transparent window
		// END transparent window
		nk_end(ctx);

		//
		ctx->style.window.fixed_background = prev_style_item;
	}
}

void MyGameState_GameOptionsMenu::_Draw_ImGui_Stuff(
	float delta_time_in_seconds
	, NwGameI* game_app
	, const NwViewport viewport
	)
{
	FPSGame& game = FPSGame::Get();
	MyGameUserSettings& user_settings = game.user_settings;
	InGameSettings& ingame_settings = game.user_settings.ingame;

	//
	ImGui::SetNextWindowPos(ImVec2(viewport.x, viewport.y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(viewport.width, viewport.height), ImGuiCond_Always);
	//ImGui::setnextWindowT

	if(ImGui::Begin("OPTIONS", nil,
		ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove
		|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse
		|ImGuiWindowFlags_NoCollapse
		|ImGuiWindowFlags_NoSavedSettings
		))
	{
		const float bottom_padding_size = 200;

		enum ESubmenu
		{
			Submenu_Controls,
			Submenu_Audio,
			Submenu_Video,
			Submenu_Gameplay,

			Submenu_Count
		};
		const float submenu_height = (viewport.height - bottom_padding_size) / Submenu_Count;

		//
		ImGui::LabelText("", "CONTROLS");
		ImGui::BeginChild(mxHASH_STR("CONTROLS"), ImVec2(0, submenu_height), true);
		{
			settings_did_changed |= ImGui::Checkbox(
				"Invert Mouse Y",
				&ingame_settings.controls.invert_mouse_y
				);
			if(ImGui::IsItemHovered())
			{
				 ImGui::BeginTooltip();
				 ImGui::TextUnformatted("Reverses mouse up-down axis.");
				 ImGui::EndTooltip();
			}

			//
			settings_did_changed |= ImGui::Checkbox(
				"Invert Mouse X",
				&ingame_settings.controls.invert_mouse_x
				);
			if(ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted("Reverses horizontal mouse axis.");
				ImGui::EndTooltip();
			}

			//
			settings_did_changed |= ImGui::SliderFloat(
				"Mouse Sensitivity",
				&ingame_settings.controls.mouse_sensitivity,
				0.1f,
				10.0f
				);
		}
		ImGui::EndChild();


#if GAME_CFG_WITH_SOUND
		//
		ImGui::LabelText("", "AUDIO");
		ImGui::BeginChild(mxHASH_STR("AUDIO"), ImVec2(0, submenu_height), true);
		{
			if(ImGui::SliderInt("Sound Effects Volume",
				&ingame_settings.sound.effects_volume,
				0,
				100
				))
			{
				ingame_settings.sound.FixBadSettings();
				game.sound.SetVolume(
					SND_CHANNEL_MAIN,
					ingame_settings.sound.effects_volume / 100.0f
					);
				settings_did_changed = true;
			}

			if(ImGui::SliderInt("Music Volume",
				&ingame_settings.sound.music_volume,
				0,
				100
				))
			{
				ingame_settings.sound.FixBadSettings();
				game.sound.SetVolume(
					SND_CHANNEL_MUSIC,
					ingame_settings.sound.music_volume / 100.0f
					);
				settings_did_changed = true;
			}
		}
		ImGui::EndChild();
#endif // GAME_CFG_WITH_SOUND

		//
		ImGui::LabelText("", "VIDEO");
		ImGui::BeginChild(mxHASH_STR("VIDEO"), ImVec2(0, submenu_height), true);
		{
			settings_did_changed |= ImGui::Checkbox(
				"Launch in Fullscreen Mode (Restart required)",
				&user_settings.window.fullscreen
				);
			if(ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Forces the game to start in fullscreen mode.");
			}
		}
		ImGui::EndChild();

		//
		ImGui::LabelText("", "GAMEPLAY");
		ImGui::BeginChild(mxHASH_STR("GAMEPLAY"), ImVec2(0, submenu_height), true);
		{
			settings_did_changed |= ImGui::Checkbox(
				"Destructible Environment?",
				&ingame_settings.gameplay.destructible_environment
			);
			if(ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Allows to destroy the environment with explosives.");
			}

			//
			settings_did_changed |= ImGui::Checkbox(
				"Destructible Floor?",
				&ingame_settings.gameplay.destructible_floor
			);
			if(ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(
					"Allows to blow holes in the floor."
					);
			}

			//
			if(ingame_settings.gameplay.destructible_floor)
			{
				settings_did_changed |= ImGui::Checkbox(
					"Shallow Craters?",
					&ingame_settings.gameplay.shallow_craters
					);
				if(ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(
						"Limits the depth of holes that the player can blow in the floor.\nThis is used to reduce the change of the player/NPCs getting stuck in holes."
						);
				}
			}
		}
		ImGui::EndChild();

		//
		{
			if(ImGui::Button("SAVE"))
			{
				DBGOUT("Saving settings...");

				settings_did_changed = false;
				FPSGame::Get().SaveUserDefinedInGameSettingsToFile();
			}
			ImGui::SameLine(100);
			//
			if(ImGui::Button("RESET TO DEFAULTS"))
			{
				settings_did_changed = true;
				ingame_settings = InGameSettings();
			}
		}
	}

	ImGui::End();
}
