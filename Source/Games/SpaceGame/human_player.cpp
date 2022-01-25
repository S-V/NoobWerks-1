//
#include "stdafx.h"
#pragma hdrstop

#include <bx/easing.h>

#include <Core/Util/Tweakable.h>
#include <Engine/Engine.h>	// NwTime
#include <Rendering/Public/Globals.h>
//
#include "experimental/game_experimental.h"
#include "experimental/rccpp.h"
#include "world/world.h"
#include "game_app.h"
#include "human_player.h"

/*
-------------------------------------------------------------------------------
	SgHumanPlayer
-------------------------------------------------------------------------------
*/
mxDEFINE_CLASS(SgHumanPlayer);
mxBEGIN_REFLECTION(SgHumanPlayer)
	mxMEMBER_FIELD( camera ),
	//mxMEMBER_FIELD( position_in_world_space ),
mxEND_REFLECTION;
SgHumanPlayer::SgHumanPlayer()
{
	//position_in_world_space = V3d::zero();
	//num_seconds_shift_is_held = 0;

	time_when_received_damage = 0;
	time_when_map_was_loaded = 0;
}

void SgHumanPlayer::UpdateOncePerFrame(
	const NwTime& game_time
	, const SgCheatSettings& game_cheat_settings
	)
{
	//
	camera.Update( game_time.real.delta_seconds );
	camera.UpdateVelocityAndPositionFromInputs( game_time.real.delta_seconds );

	if(!game_cheat_settings.enable_noclip)
	{
		// don't let the player look up or down more than 90 degrees
		camera.m_pitchAngle = clampf(
			camera.m_pitchAngle,
			DEG2RAD(-85),
			DEG2RAD(+88)
			);
	}
}

V3f SgHumanPlayer::GetEyePosition() const
{
	return V3f::fromXYZ(camera._eye_position);
}

void SgHumanPlayer::SetCameraPosition(
	const V3d new_position_in_world_space
	)
{
	camera._eye_position = new_position_in_world_space;
}

const V3f SgHumanPlayer::GetPhysicsBodyCenter() const
{
	UNDONE;
	return CV3f(0);
	//return phys_char_controller.getPos();
}

void SgHumanPlayer::RenderFirstPersonView(
	const NwCameraView& camera_view
	, GL::NwRenderContext & render_context
	, const NwTime& args
	, SpatialDatabaseI* spatial_database
	) const
{
	const SgAppSettings& game_settings = SgGameApp::Get().settings;

	const bool enable_player_collisions = !game_settings.cheats.enable_noclip;
	const bool draw_player_weapon = enable_player_collisions;

	////
	//_RenderFullScreenDamageIndicatorIfNeeded(
	//	render_context
	//	, args
	//	);

	//
	_FadeInUponLevelLoadingIfNeeded(
		render_context
		, args
		);
}

ERet SgHumanPlayer::_RenderFullScreenDamageIndicatorIfNeeded(
	GL::NwRenderContext & render_context
	, const NwTime& args
	) const
{
	//
	nwTWEAKABLE_CONST(int, PLAYER_DAMAGE_INDICATOR_DURATION_MSEC, 700);

	//
	const MillisecondsU32 curr_time_msec = mxGetTimeInMilliseconds();
	const MillisecondsU32 time_elapsed_since_received_damage_msec = curr_time_msec - time_when_received_damage;

	if(time_elapsed_since_received_damage_msec <= PLAYER_DAMAGE_INDICATOR_DURATION_MSEC)
	{
		const float01_t alpha
			= clampf(
			1.0f - float(time_elapsed_since_received_damage_msec) / float(PLAYER_DAMAGE_INDICATOR_DURATION_MSEC)
			, 0
			, 0.8f // don't draw it fully opaque
			)
			;

		//
		NwShaderEffect* technique;
		mxDO(Resources::Load(
			technique
			, MakeAssetID("player_indicate_damage")
			, &SgGameApp::Get().runtime_clump
			));

		//
		V4f	red_color = {1,0,0, alpha};
		technique->setUniform(
			mxHASH_STR("u_color")
			, &red_color
			);

		//
		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];
		const U32 draw_order = Rendering::Globals::
			.GetRenderPath()
			.getPassDrawingOrder( Rendering::ScenePasses::DebugTextures )
			;

		//
		GL::NwPushCommandsOnExitingScope	submitCommands(
			render_context,
			GL::buildSortKey(
			draw_order,
			pass0.default_program_handle,
			GL::SortKey(0)
			)
			);
		mxDO(technique->pushAllCommandsInto( render_context._command_buffer ));

		mxDO(push_FullScreenTriangle(
			render_context
			, pass0.render_state
			, technique
			, pass0.default_program_handle
			));
	}

	return ALL_OK;
}

ERet SgHumanPlayer::_FadeInUponLevelLoadingIfNeeded(
	GL::NwRenderContext & render_context
	, const NwTime& args
	) const
{
	//
	nwNON_TWEAKABLE_CONST(int, LEVEL_LOADED_FADE_IN_DURATION_MSEC, 2000);

	//
	const MillisecondsU32 curr_time_msec = mxGetTimeInMilliseconds();
	const MillisecondsU32 time_elapsed_since_level_was_loaded = curr_time_msec - time_when_map_was_loaded;

	if(time_elapsed_since_level_was_loaded <= LEVEL_LOADED_FADE_IN_DURATION_MSEC)
	{
		const float01_t alpha
			= clampf(
			1.0f - float(time_elapsed_since_level_was_loaded) / float(LEVEL_LOADED_FADE_IN_DURATION_MSEC)
			, 0
			, 1.0f
			)
			;

		//
		NwShaderEffect* technique;
		mxDO(Resources::Load(
			technique
			, MakeAssetID("player_indicate_damage")
			, &SgGameApp::Get().runtime_clump
			));

		//
		V4f	black_color = {0,0,0, alpha};
		technique->setUniform(
			mxHASH_STR("u_color")
			, &black_color
			);

		//
		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];
		const U32 draw_order = Rendering::Globals::
			.GetRenderPath()
			.getPassDrawingOrder( Rendering::ScenePasses::DebugTextures )
			;

		//
		GL::NwPushCommandsOnExitingScope	submitCommands(
			render_context,
			GL::buildSortKey(
			draw_order,
			pass0.default_program_handle,
			GL::SortKey(0)
			)
			);
		mxDO(technique->pushAllCommandsInto( render_context._command_buffer ));

		mxDO(push_FullScreenTriangle(
			render_context
			, pass0.render_state
			, technique
			, pass0.default_program_handle
			));
	}

	return ALL_OK;
}

void SgHumanPlayer::Dbg_RenderFirstPersonView(
	const NwCameraView& camera_view
	, SgRenderer& renderer
	) const
{
}

float SgHumanPlayer::calc_debug_camera_speed() const
{
	UNDONE;
	return 0;
	//num_seconds_shift_is_held
}

void SgHumanPlayer::PlaySound3D(
							   const AssetID& sound_id
							   )
{
	SgGameApp& game = SgGameApp::Get();

#if GAME_CFG_WITH_SOUND	
	
	game.sound.PlaySound3D(
		sound_id
		, this->GetEyePosition()
		);

#endif // #if GAME_CFG_WITH_SOUND
}

void SgHumanPlayer::RespawnAtPos(
								const V3f& spawn_pos
								)
{
	this->SetCameraPosition(
		V3d::fromXYZ(spawn_pos)
		);
}

void SgHumanPlayer::StopCameraMovement()
{
	// workaround for the bug with sticky movement keys
	camera.m_movementFlags = 0;
}
