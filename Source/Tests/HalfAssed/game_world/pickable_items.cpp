// medkits, ammo boxes, etc.
#include "stdafx.h"
#pragma hdrstop

#include <Base/Math/Random.h>
#include <Core/Util/Tweakable.h>
#include <Renderer/Core/MeshInstance.h>

#include "game_compile_config.h"
#include "game_rendering/game_renderer.h"
#include "game_rendering/particles/game_particles.h"
#include "game_world/gibs.h"
#include "FPSGame.h"
#include "gameplay_constants.h"

#if GAME_CFG_WITH_SOUND
#include <Sound/SoundSystem.h>
#endif

#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
#endif

#include <Engine/Model.h>


namespace
{
	//
	struct SItemDesc
	{
		const char*	model_name;
		float		model_scale;
	};
	const SItemDesc s_item_descs[Gameplay::EPickableItem::ITEM_COUNT] =
	{
		{ "medkit_small", 0.5f },	// ITEM_MEDKIT

		{ "plasmagun_world", 0.5f },	// ITEM_PLASMAGUN

		{ "rocket_large", 0.5f },	// ITEM_ROCKET_AMMO

		{ "bfg_case", 0.5f },	// ITEM_BFG_CASE
	};
}


namespace MyGameUtils
{
	ERet SpawnItem(
		const Gameplay::EPickableItem item_type
		, const V3f& pos_ws
		)
	{
		//
		FPSGame& game = FPSGame::Get();

		const SItemDesc& item_desc = s_item_descs[ item_type ];

		//
		NwModel*	new_model_inst;
		mxDO(NwModel_::Create(
			new_model_inst

			, MakeAssetID(item_desc.model_name)
			, game.runtime_clump

			, M44f::createUniformScaling(item_desc.model_scale)
			, *game.world.render_world
			
			, game.world.physics_world
			, PHYS_PICKABLE_ITEM

			, M44_RotationY(DEG2RAD(90))	// Doom'3
				* M44f::createTranslation(pos_ws)

			//, velocity_in_world
			//, angular_velocity
			));

		new_model_inst->rigid_body->bt_rb().setUserIndex2(
			item_type
			);

		return ALL_OK;
	}

	bool PickItemIfShould(
		int col_obj_item_type
	)
	{
		const Gameplay::EPickableItem item_type = (Gameplay::EPickableItem) col_obj_item_type;

		//
		FPSGame& game = FPSGame::Get();

		switch(item_type)
		{
		case Gameplay::ITEM_MEDKIT:
			if(game.player.HasFullHealth()) {
				return false;
			}
			game.player.comp_health._health = Gameplay::PLAYER_HEALTH;

			game.player.PlaySound3D(
				MakeAssetID("player_use_medkit")
				);
			return true;

		case Gameplay::ITEM_PLASMAGUN:
			return game.player.PickUp_Weapon_Plasmagun();

		case Gameplay::ITEM_ROCKET_AMMO:
			return game.player.PickUp_Item_RocketAmmo();

		case Gameplay::ITEM_BFG_CASE:
			return game.player.PickUp_Item_BFG_Case();

		default:
			mxUNREACHABLE;
			return false;
		}

		//game.pl

		return true;
	}

}//namespace MyGameUtils
