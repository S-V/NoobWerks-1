#pragma once

#include <Base/Template/MovingAverage.h>
#include <Core/Tasking/TinyCoroutines.h>
//
#include <Utility/Camera/NwFlyingCamera.h>
#include <Utility/Camera/NwOrbitingCamera.h>
//
#include "compile_config.h"

//
#if GAME_CFG_WITH_PHYSICS
//#include <Physics/Physics_CharacterController.h>
#include <Physics/Bullet/DynamicCharacterController.h>
#endif // GAME_CFG_WITH_PHYSICS

//
#include "common.h"

// CoTaskI
#include "experimental/game_experimental.h"

class SgHumanPlayer;



struct PlayerMovementFlags
{
	enum Enum {
		FORWARD		= BIT(0),
		BACKWARD	= BIT(1),
		LEFT		= BIT(2),
		RIGHT		= BIT(3),
		JUMP		= BIT(4),
		//DOWN		= BIT(5),
		//ACCELERATE	= BIT(6),	//!< useful for debugging planetary LoD
		//RESET		= BIT(7),	//!< reset position and orientation
		////STOP_NOW	= BIT(7),	//!< 		
	};
};
mxDECLARE_ENUM( PlayerMovementFlags::Enum, U32, PlayerMovementFlagsT );


/*
-------------------------------------------------------------------------------
	SgHumanPlayer
-------------------------------------------------------------------------------
*/
class SgHumanPlayer: CStruct, NwNonCopyable
{
public:
	NwFlyingCamera	camera;

	//
	//float	num_seconds_shift_is_held;

	// for player damage indicator
	MillisecondsU32	time_when_received_damage;

	// for fading in after loading the level
	MillisecondsU32	time_when_map_was_loaded;

public:
	mxDECLARE_CLASS(SgHumanPlayer, CStruct);
	mxDECLARE_REFLECTION;
	SgHumanPlayer();

	void UpdateOncePerFrame(
		const NwTime& args
		, const SgCheatSettings& game_cheat_settings
		);

	/// corrected for eye height
	V3f GetEyePosition() const;

	void SetCameraPosition(
		const V3d new_position_in_world_space
		);

	const V3f GetPhysicsBodyCenter() const;

	void RespawnAtPos(
		const V3f& spawn_pos
		);

	void StopCameraMovement();

public:
	void RenderFirstPersonView(
		const NwCameraView& camera_view
		, GL::NwRenderContext & render_context
		, const NwTime& args
		, SpatialDatabaseI* spatial_database
		) const;

	void Dbg_RenderFirstPersonView(
		const NwCameraView& camera_view
		, SgRenderer& renderer
		) const;

private:
	ERet _RenderFullScreenDamageIndicatorIfNeeded(
		GL::NwRenderContext & render_context
		, const NwTime& args
		) const;
	
	ERet _FadeInUponLevelLoadingIfNeeded(
		GL::NwRenderContext & render_context
		, const NwTime& args
		) const;

public:
	void PlaySound3D(
		const AssetID& sound_id
		);

public:
	float calc_debug_camera_speed(
		) const;
};
