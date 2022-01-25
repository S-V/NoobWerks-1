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

#if GAME_CFG_WITH_SOUND
#include <Sound/SoundSystem.h>
#endif

#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
#endif

#include <Engine/Model.h>


static const char* s_gib_models[] = {
	"arm",
	"chest",
	"gib5",
	"gib6",
	"leg1",
	"leg2",
	"smallchest",
};

MyGameGibs::MyGameGibs()
{
	next_item_to_remove_if_full = 0;
}

static
void UpdateGraphicsTransformFromPhysics2(
										const btRigidBody& bt_rigid_body
										, NwModel& render_model
										, ARenderWorld* render_world
										, const float gib_model_scale
										)
{
	const btTransform& bt_rb_xform = bt_rigid_body.getCenterOfMassTransform();
	const M44f	rigid_body_local_to_world_matrix = M44f_from_btTransform( bt_rb_xform );

	//
	const AABBf	aabb_in_model_space = render_model.mesh->local_aabb;

	//
	const M44f	gfx_local_to_world_matrix
		= M44f::createUniformScaling(gib_model_scale)
		* rigid_body_local_to_world_matrix
		;

	render_model.transform->local_to_world = gfx_local_to_world_matrix;

	//
	render_world->updateEntityAabbWorld(
		render_model.world_proxy_handle,
		AABBd::fromOther(aabb_in_model_space.transformed(gfx_local_to_world_matrix))
		);
}

void MyGameGibs::UpdateOncePerFrame(
	const NwTime& game_time
	//, GameParticleRenderer& game_particles
	, ARenderWorld* render_world
	//, StateDeltaForCollisionRayCasts &phys_delta_state
	)
{
	//
	GibItem* gib_items = alive_gib_items.raw();
	UINT num_alive_gib_items = alive_gib_items.num();

	//
	UINT gib_item_idx = 0;

	while( gib_item_idx < num_alive_gib_items )
	{
		GibItem &	gib_item = gib_items[ gib_item_idx ];

		//
		const F32 gib_item_life_new
			= gib_item.life
			+ game_time.real.delta_seconds * gib_item.inv_lifespan
			;

		if( gib_item_life_new < 1.0f )
		{
			// the gib_item is still alive
			++gib_item_idx;

			gib_item.life = gib_item_life_new;

			////
			//UpdateGraphicsTransformFromPhysics2(
			//	gib_item.rigid_body->bt_rb()
			//	, *gib_item.model
			//	, render_world
			//	, gib_item.model_scale
			//	);
		}
		else
		{
			// the gib_item is dead - remove it
			_RemoveResourcesUsedByOldGibItem(
				gib_item
				);

			TSwap( gib_item, gib_items[ --num_alive_gib_items ] );
		}
	}

	//
	alive_gib_items.setNum( num_alive_gib_items );
}

ERet MyGameGibs::SpawnGibs(
	const V3f& position_in_world
	, const V3f& model_center
	, const AABBf& model_aabb_world
	, const float model_scale

	, const int git_item_count
	, const float git_item_velocity
	)
{
	const V3f model_aabb_half_size = model_aabb_world.halfSize();

	//
	const float adjusted_gib_item_velocity = git_item_velocity * model_aabb_half_size.boxVolume();

	//
	idRandom	rnd;

	const U32 curr_time_msec = mxGetTimeInMilliseconds();

	//
	for(int i = 0; i < git_item_count; i++)
	{
		rnd.SetSeed( curr_time_msec ^ i );

		const int gib_mdl_idx = rnd.RandomInt(mxCOUNT_OF(s_gib_models));

		V3f rnd_delta_pos;
		rnd_delta_pos.x = rnd.RandomFloat(-model_aabb_half_size.x, model_aabb_half_size.x);
	
		rnd.SetSeed( curr_time_msec ^ i ^ gib_mdl_idx );
		rnd_delta_pos.y = rnd.RandomFloat(-model_aabb_half_size.y, model_aabb_half_size.y);

		rnd.SetSeed( curr_time_msec ^ i ^ gib_mdl_idx );
		rnd_delta_pos.z = rnd.RandomFloat(-model_aabb_half_size.z, model_aabb_half_size.z);

		//
		float ang_vel_1 = rnd.RandomFloat(-1, +1);

		rnd.SetSeed( curr_time_msec ^ i | gib_mdl_idx );
		float ang_vel_2 = rnd.RandomFloat(-1, +1);

		const V3f angular_velocity = {
			ang_vel_1,
			ang_vel_2,
			ang_vel_1,
		};

		SpawnSingleGibItem(
			position_in_world + rnd_delta_pos
			, MakeAssetID(s_gib_models[gib_mdl_idx])
			, (position_in_world - model_center).normalized() * git_item_velocity
			, angular_velocity * adjusted_gib_item_velocity
			, model_scale
			);
	}

	return ALL_OK;
}

ERet MyGameGibs::SpawnSingleGibItem(
	const V3f& position_in_world
	, const AssetID& model_def
	, const V3f& velocity_in_world
	, const V3f& angular_velocity
	, const float model_scale
	)
{
	nwTWEAKABLE_CONST( SecondsF, GIBS_LIFESPAN_SECONDS, 4 );

	// so that gibs don't disappear at the same time
	const int ix = always_cast<int>(position_in_world.x);
	const int iy = always_cast<int>(position_in_world.y);
	const int iz = always_cast<int>(position_in_world.z);
	idRandom	rnd(
		mxGetTimeInMilliseconds() ^ Hash32Bits_0(ix ^ iy ^ iz) ^ model_def.d.size()
		);

	const SecondsF adjusted_lifespan = GIBS_LIFESPAN_SECONDS + rnd.RandomFloat(0, 3);

	//
	FPSGame& game = FPSGame::Get();

	NwModel*	new_model_inst;
	mxDO(NwModel_::Create(
		new_model_inst

		, model_def
		, game.runtime_clump

		, M44f::createUniformScaling(model_scale)
		, *game.world.render_world

		, game.world.physics_world
		, PHYS_OBJ_GIB_ITEM

		, M44_buildTranslationMatrix(position_in_world)

		, NwModel_::COL_REPR::COL_BOX

		, velocity_in_world
		, angular_velocity
		));

	//
	GibItem	new_gib_item;
	{
		new_gib_item.pos = position_in_world;
		//new_gib_item.velocity = direction_in_world * velocity_meters_per_second;
		new_gib_item.life = 0;
		new_gib_item.inv_lifespan = 1.0f / adjusted_lifespan;

		new_gib_item.model_scale = model_scale;

		new_gib_item.rigid_body = new_model_inst->rigid_body;
		new_gib_item.model = new_model_inst;
	}

	//
	if(alive_gib_items.isFull())
	{
		const U32 next_item_to_remove = next_item_to_remove_if_full++ % MAX_GIBS;

		_RemoveResourcesUsedByOldGibItem(
			alive_gib_items._data[ next_item_to_remove ]
		);

		alive_gib_items._data[ next_item_to_remove ] = new_gib_item;
	}
	else
	{
		GibItem	overwritten_gib_item;
		const bool has_overwritten_old_gib_item = alive_gib_items.AddWithWraparoundOnOverflowEx(
			new_gib_item
			, &overwritten_gib_item
			);
		if(has_overwritten_old_gib_item)
		{
			_RemoveResourcesUsedByOldGibItem(overwritten_gib_item);
		}
	}

	return ALL_OK;
}

void MyGameGibs::_RemoveResourcesUsedByOldGibItem(
	const MyGameGibs::GibItem& old_gib_item
	)
{
	FPSGame& game = FPSGame::Get();

	NwModel_::Delete(
		((MyGameGibs::GibItem&)old_gib_item).model
		, game.runtime_clump
		, *game.world.render_world
		, game.world.physics_world
		);
}

void MyGameGibs::PlayMeatSplatSound(
							   const V3f& position_in_world
		)
{

#if GAME_CFG_WITH_SOUND

	FPSGame& game = FPSGame::Get();

	game.sound.PlaySound3D(
		MakeAssetID("gib_item_splat"),
		position_in_world
		);

#endif // GAME_CFG_WITH_SOUND

}
