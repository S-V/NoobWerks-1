#include "stdafx.h"
#pragma hdrstop

#include <Base/Math/Random.h>
#include <Core/Util/Tweakable.h>
#include <Voxels/public/vx5.h>
#include <Engine/Model.h>

#include "game_compile_config.h"
#include "game_rendering/game_renderer.h"
#include "game_rendering/particles/game_particles.h"
#include "game_world/game_projectiles.h"
#include "FPSGame.h"
#include "gameplay_constants.h"

#if GAME_CFG_WITH_SOUND
#include <Sound/SoundSystem.h>
#endif

#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
#endif

#include "game_world/explosions.h"


static const V3f	PLASMA_LIGHT_COLOR_START = CV3f(0.3f, 0.60f, 0.82f) * 2.0f;	// HOT PLASMA
static const V3f	PLASMA_LIGHT_COLOR_END = CV3f(1.0f, 0.60f, 0.3f);	// OLD & COLD PLASMA

struct PlasmaColor
{
	float	min_life;
	V3f		color_rgb;
};
static const PlasmaColor s_plasma_colors[] = {
	// HOT NEW PLASMA
	{
		0,
		{ 0.6f, 1.2f, 1.64f }
	},

	// YELLOWISH PLASMA
	{
		0.1f,
		{ 1.4f * 1.5f, 1.20f * 1.5f, 0.88f * 1.5f }
	},

	// REDDISH PLASMA
	{
		0.5f,
		{ 0.9f, 0.20f, 0.1f }
	},

	// OLD & COLD PLASMA
	{
		0.6f,
		{ 0.8f * 0.5f, 0.60f * 0.5f, 0.3f * 0.5f }
	}
};

static
V3f GetPlasmaBoltColor(const float01_t plasma_bolt_life)
{
#if 0
	return V3_Lerp01(
		PLASMA_LIGHT_COLOR_START,
		PLASMA_LIGHT_COLOR_END,
		plasma_bolt_life
		);
#else

	for( int i = 0; i < mxCOUNT_OF(s_plasma_colors) - 1; i++ )
	{
		const PlasmaColor& plasma_color = s_plasma_colors[i];
		const PlasmaColor& plasma_color_next = s_plasma_colors[i+1];

		if( plasma_bolt_life < plasma_color_next.min_life )
		{
			const float lerp_factor
				= (plasma_bolt_life - plasma_color.min_life)
				/ (plasma_color_next.min_life - plasma_color.min_life)
				;

			return V3_Lerp01(
				plasma_color.color_rgb,
				plasma_color_next.color_rgb,
				lerp_factor
				);
		}
	}

	return s_plasma_colors[ mxCOUNT_OF(s_plasma_colors) - 1 ].color_rgb;

#endif
}




ERet GameProjectiles::Initialize(
	const NwEngineSettings& engine_settings
	, const RrRuntimeSettings& renderer_settings
	, NwClump* scene_clump
	)
{
	return ALL_OK;
}

void GameProjectiles::Shutdown()
{

}

void GameProjectiles::AdvanceParticlesOncePerFrame(
	const NwTime& game_time
	, GameParticleRenderer& game_particles
	, ARenderWorld* render_world
	, StateDeltaForCollisionRayCasts &phys_delta_state
	)
{
	_AdvanceRocketsIfAny(
		game_time
		, game_particles
		, render_world
		, phys_delta_state
		);
	_AdvancePlasmaBoltsIfAny(
		game_time
		, game_particles
		, render_world
		, phys_delta_state
		);
}

ERet GameProjectiles::_AdvanceRocketsIfAny(
	const NwTime& game_time
	, GameParticleRenderer& game_particles
	, ARenderWorld* render_world
	, StateDeltaForCollisionRayCasts &phys_delta_state
	)
{
	if(flying_rockets.isEmpty())
	{
		return ALL_OK;
	}

	const SecondsF rocket_life_span_seconds = 5.0f;
	const F32 rocket_lifespan_inv = 1.0f / rocket_life_span_seconds;

	//static float	smoke_trail_puffs_per_second = 50.0f;
	//HOT_VAR(smoke_trail_puffs_per_second);

	static float	smoke_trail_puffs_per_meter = 0.5f;
	HOT_VAR(smoke_trail_puffs_per_meter);

	//
	const V3f	rocket_light_color = CV3f(1.0f, 0.8, 0.4) * 2.0f;
	const F32	rocket_light_radius = 4.0f;

	//
	FlyingRocket* rockets = flying_rockets.raw();
	UINT num_alive_rockets = flying_rockets.num();

	//
	mxDO(phys_delta_state.rockets.reserve(num_alive_rockets));

	//
	UINT rocket_idx = 0;

	while( rocket_idx < num_alive_rockets )
	{
		FlyingRocket &	rocket = rockets[ rocket_idx ];

		//
		const V3f old_rocket_position = rocket.pos;
		

		const F32 rocket_life = rocket.life;
		const F32 rocket_life_new = rocket_life + game_time.real.delta_seconds * rocket_lifespan_inv;

		if( rocket_life_new < 1.0f )
		{
			//// squared velocity falloff
			//const F32 rocket_life_sq = rocket_life * rocket_life;

			//// Slow down by 50% as we age
			//V3f rocket_velocity_new = rocket.velocity * ( 1.0f - 0.5f * rocket_life_sq );

			// add some to the up direction, because of buoyancy
			//rocket_velocity_new += V3_UP * explosions_settings.rocket_buoyancy_vel;

			const V3f curr_rocket_position = rocket.model->rigid_body->GetCenterWorld();
			
			//old_rocket_position + rocket.velocity * game_time.real.delta_seconds;

#if GAME_CFG_WITH_PHYSICS
			{
				StateDeltaForCollisionRayCasts::DeltaPos& delta_pos =
					AllocateUnsafe_NoOverflowCheck(phys_delta_state.rockets)
					;
				delta_pos.old_pos = old_rocket_position;
				delta_pos.item_index = rocket_idx;
				delta_pos.new_pos = curr_rocket_position;
			}
#endif // GAME_CFG_WITH_PHYSICS

			rocket.pos = curr_rocket_position;
			rocket.life = rocket_life_new;

			//
			{
				TbDynamicPointLight	rocket_light;
				rocket_light.position = old_rocket_position - rocket.velocity.normalized();	// 1 m behind
				rocket_light.radius = rocket_light_radius;
				rocket_light.color = rocket_light_color;

				render_world->pushDynamicPointLight(rocket_light);
			}

			// Emit smoke trail puffs.
#if 0
			const float prev_life_time_seconds = rocket_life * rocket_life_span_seconds;
			const float next_life_time_seconds = rocket_life_new * rocket_life_span_seconds;

			const int num_smoke_puffs_emitted_prev = mxFloatToInt( prev_life_time_seconds * smoke_trail_puffs_per_second );
			const int num_smoke_puffs_emitted_next = mxFloatToInt( next_life_time_seconds * smoke_trail_puffs_per_second );
			const int num_smoke_puffs_to_emit = num_smoke_puffs_emitted_next - num_smoke_puffs_emitted_prev;
			if(num_smoke_puffs_to_emit > 0)
			{
				const V3f rocket_position_delta = rocket_position_new - rocket_position;

				float	position_delta_length;
				const V3f rocket_flying_direction = V3_Normalized(rocket_position_delta, position_delta_length);

				const float	smoke_puff_position_step_delta = position_delta_length / num_smoke_puffs_to_emit;

				for( int i = 0; i < num_smoke_puffs_to_emit; i++ )
				{
					const V3f smoke_puff_pos = rocket_position + rocket_flying_direction * (smoke_puff_position_step_delta * i);

					game_particles.CreateSmokePuff_RocketTrail(
						smoke_puff_pos
						);
				}
			}

#else

			//
			idRandom	smoke_puff_pos_random(
				rocket_idx ^ always_cast<int>(rocket_life)
				);

			const float prev_life_time_seconds = rocket_life * rocket_life_span_seconds;
			const float next_life_time_seconds = rocket_life_new * rocket_life_span_seconds;

			const float rocket_velocity_mag = rocket.velocity.length();
			const float prev_distance_travelled = prev_life_time_seconds * rocket_velocity_mag;
			const float next_distance_travelled = next_life_time_seconds * rocket_velocity_mag;

			const int num_smoke_puffs_emitted_prev = mxFloatToInt( prev_distance_travelled * smoke_trail_puffs_per_meter )
				+ smoke_puff_pos_random.RandomInt(2);	// skip some puffs
			const int num_smoke_puffs_emitted_next = mxFloatToInt( next_distance_travelled * smoke_trail_puffs_per_meter );

			const int num_smoke_puffs_to_emit = num_smoke_puffs_emitted_next - num_smoke_puffs_emitted_prev;
			if(num_smoke_puffs_to_emit > 0)
			{
				const V3f rocket_position_delta = curr_rocket_position - old_rocket_position;

				float	position_delta_length;
				const V3f rocket_flying_direction = V3_Normalized(rocket_position_delta, position_delta_length);

				const float	smoke_puff_position_step_delta = position_delta_length / num_smoke_puffs_to_emit;

				for( int i = 0; i < num_smoke_puffs_to_emit; i++ )
				{
					const V3f smoke_puff_pos
						= old_rocket_position
						+ rocket_flying_direction * (smoke_puff_position_step_delta * i)
						;

#if 0
					game_particles.CreateSmokePuff_RocketTrail(
						smoke_puff_pos
						);
#endif
				}
			}

#endif

			// the rocket is still alive
			++rocket_idx;
		}
		else
		{
			// the rocket is dead - remove it
			{
				// so that explosion craters are different
				const int ix = always_cast<int>(rocket.hit_pos.x);
				const int iy = always_cast<int>(rocket.hit_pos.y);
				const int iz = always_cast<int>(rocket.hit_pos.z);

				MyGameUtils::Explode_Rocket(
					rocket.hit_pos
					, rocket.hit_normal
					, num_alive_rockets ^ Hash32Bits_0(ix ^ iy ^ iz)
					);

				//
				FPSGame::Get().world.RequestDeleteModel(
					rocket.model
					);
			}

			//
			TSwap( rocket, rockets[ --num_alive_rockets ] );
		}//if dead
	}//for each rocket

	//
	flying_rockets.setNum( num_alive_rockets );

	return ALL_OK;
}

ERet GameProjectiles::_AdvancePlasmaBoltsIfAny(
	const NwTime& game_time
	, GameParticleRenderer& game_particles
	, ARenderWorld* render_world
	, StateDeltaForCollisionRayCasts &phys_delta_state
	)
{
	if(plasma_bolts.isEmpty())
	{
		return ALL_OK;
	}

	const SecondsF plasma_life_span_seconds = 7.0f;
	const F32 plasma_lifespan_inv = 1.0f / plasma_life_span_seconds;

	//
	const F32	plasma_light_radius = 2.0f;

	////
	static float PLASMA_BOLT_MIN_SIZE = 0.05f;	// 5 cm radius
	HOT_VAR(PLASMA_BOLT_MIN_SIZE);

	static float PLASMA_BOLT_MAX_SIZE = 3.0f;	// 3 meters
	HOT_VAR(PLASMA_BOLT_MAX_SIZE);

	static float PLASMA_FALL_DOWN_SPEED = 1.0f;	// per second
	HOT_VAR(PLASMA_FALL_DOWN_SPEED);

	//
	PlasmaBolt* plasma_bolts_ptr = plasma_bolts.raw();
	UINT num_plasma_bolts = plasma_bolts.num();

	//
	mxDO(phys_delta_state.plasma_bolts.reserve(num_plasma_bolts));

	//
	UINT plasma_idx = 0;

	while( plasma_idx < num_plasma_bolts )
	{
		PlasmaBolt &	plasma_bolt = plasma_bolts_ptr[ plasma_idx ];

		const F32 plasma_life = plasma_bolt.life;
		const F32 plasma_life_new = plasma_life + game_time.real.delta_seconds * plasma_lifespan_inv;

		if( plasma_life_new < 1.0f )
		{
			// squared velocity falloff
			const F32 plasma_life_sq = plasma_life * plasma_life;

			// Slow down by 1% as we age
			const V3f plasma_velocity = plasma_bolt.velocity;
			V3f plasma_velocity_new = plasma_velocity * ( 1.0f - 0.01f * plasma_life_sq );

			// add some to the down direction, because of gravity
			plasma_velocity_new -= V3_UP * PLASMA_FALL_DOWN_SPEED * game_time.real.delta_seconds;

			//
			const V3f plasma_position = plasma_bolt.pos;
			const V3f plasma_position_new = plasma_position + plasma_velocity * game_time.real.delta_seconds;

#if GAME_CFG_WITH_PHYSICS
			{
				StateDeltaForCollisionRayCasts::DeltaPos& delta_pos =
					AllocateUnsafe_NoOverflowCheck(phys_delta_state.plasma_bolts)
					;
				delta_pos.old_pos = plasma_position;
				delta_pos.item_index = plasma_idx;
				delta_pos.new_pos = plasma_position_new;
			}
#endif // GAME_CFG_WITH_PHYSICS

			const F32 particle_size_new = PLASMA_BOLT_MIN_SIZE
				+ (PLASMA_BOLT_MAX_SIZE - PLASMA_BOLT_MIN_SIZE) * plasma_life_new
				;

			//
			plasma_bolt.pos = plasma_position_new;
			plasma_bolt.life = plasma_life_new;
			plasma_bolt.velocity = plasma_velocity_new;
			plasma_bolt.size = particle_size_new;

			//
			const V3f	plasma_light_color = GetPlasmaBoltColor(plasma_life_new);

			//
			{
				TbDynamicPointLight	plasma_light;
				plasma_light.position = plasma_position;
				plasma_light.radius = plasma_light_radius;
				plasma_light.color = plasma_light_color;

				render_world->pushDynamicPointLight(plasma_light);
			}

			// the plasma_bolt is still alive
			++plasma_idx;
		}
		else
		{
			// the plasma_bolt is dead - remove it
			TSwap( plasma_bolt, plasma_bolts_ptr[ --num_plasma_bolts ] );
		}
	}

	//
	plasma_bolts.setNum( num_plasma_bolts );

	return ALL_OK;
}

void GameProjectiles::CollideWithWorld(
									   const StateDeltaForCollisionRayCasts& phys_delta_state
									   , NwPhysicsWorld& physics_world
									   , NwSoundSystemI& sound_world
									   , GameParticleRenderer& game_particles
									   , NwDecalsSystem& decals
									   , VX5::WorldI* voxel_world
									   , VX5::NoiseBrushI* crater_noise_brush
									   )
{
	_CollideRockets(
		phys_delta_state
		, physics_world
		, sound_world
		, game_particles
		, voxel_world
		, crater_noise_brush
		);

	_CollidePlasmaBolts(
		phys_delta_state
		, physics_world
		, sound_world
		, game_particles
		, decals
		, voxel_world
		);
}

void GameProjectiles::_CollideRockets(
									  const StateDeltaForCollisionRayCasts& phys_delta_state
									  , NwPhysicsWorld& physics_world
									  , NwSoundSystemI& sound_world
									  , GameParticleRenderer& game_particles
									  , VX5::WorldI* voxel_world
									  , VX5::NoiseBrushI* crater_noise_brush
									  )
{
	if(phys_delta_state.rockets.isEmpty())
	{
		return;
	}

	//
	nwTWEAKABLE_CONST(float, ROCKET_DIRECT_DAMAGE, 1001);

#if GAME_CFG_WITH_PHYSICS

	for(UINT i = 0; i < phys_delta_state.rockets._count; i++)
	{
		const StateDeltaForCollisionRayCasts::DeltaPos& delta_pos = phys_delta_state.rockets._data[i];
		FlyingRocket& rocket = flying_rockets[ delta_pos.item_index ];

		// Skip "newborn" rockets.
		// 1) avoids exploding into player's face;
		// 2) somewhat mimics safety system in real life.
		if(rocket.life < 0.1f) {
			continue;
		}

		//
		const btVector3	bt_ray_start = toBulletVec(delta_pos.old_pos);
		const btVector3	bt_ray_end = toBulletVec(delta_pos.old_pos + rocket.velocity.normalized());
		ClosestNotMeRayResultCallback	ray_result_callback(
			bt_ray_start,
			bt_ray_end,
			&rocket.model->rigid_body->bt_rb()
			);

		physics_world.m_dynamicsWorld.rayTest(
			bt_ray_start,
			bt_ray_end,
			ray_result_callback
			);


		//
		V3f		explosion_center;

#if 0
		//
		const V3f hit_pos_world = fromBulletVec(ray_result_callback.m_hitPointWorld);

		// move the explosion a bit away from the surface so that light flash looks better
		const V3f explosion_pos_world = hit_pos_world + hit_normal_world * 0.5f;

		explosion_center = explosion_pos_world;
#else
		explosion_center = delta_pos.old_pos;
#endif



		//
		bool	should_explode_rocket = false;


		//
		if( ray_result_callback.hasHit() )
		{
			should_explode_rocket = true;

			rocket.hit_pos = fromBulletVec(ray_result_callback.m_hitPointWorld);
			rocket.hit_normal = fromBulletVec(ray_result_callback.m_hitNormalWorld);

			//
			// Apply damage to hit entity.
			//
			if(btRigidBody* bt_rigid_body = (btRigidBody*) btRigidBody::upcast(ray_result_callback.m_collisionObject))
			{
				//bt_rigid_body->activate(true);
				//bt_rigid_body->applyCentralImpulse(
				//	(bt_ray_end - bt_ray_start).normalized() * PLASMA_IMPACT_IMPULSE_STRENGTH
				//	);

				//
				if(NwRigidBody* my_rigid_body = (NwRigidBody*) bt_rigid_body->getUserPointer())
				{
					if(EnemyNPC* enemy_npc = my_rigid_body->npc._ptr)
					{
						enemy_npc->behavior->InflictDamage(
							*enemy_npc
							, ROCKET_DIRECT_DAMAGE
							, explosion_center
							);
					}
				}
			}

		}
		else
		{
			should_explode_rocket = rocket.model->rigid_body->rocket_did_collide;

			rocket.hit_pos = explosion_center;
			rocket.hit_normal = -rocket.velocity.normalized();
		}

		//
		if(should_explode_rocket)
		{
			// will be removed during the next update
			rocket.life = 1.0f;
		}
	}//test rockets

#endif // GAME_CFG_WITH_PHYSICS
}

void GameProjectiles::_CollidePlasmaBolts(
	const StateDeltaForCollisionRayCasts& phys_delta_state
	, NwPhysicsWorld& physics_world
	, NwSoundSystemI& sound_world
	, GameParticleRenderer& game_particles
	, NwDecalsSystem& decals
	, VX5::WorldI* voxel_world
	)
{
	if(phys_delta_state.plasma_bolts.isEmpty())
	{
		return;
	}

	nwTWEAKABLE_CONST(float, PLASMA_DAMAGE, WeaponStats::PLASMAGUN_DAMAGE);
	//nwTWEAKABLE_CONST(float, PLASMA_SPLASH_DAMAGE_RADIUS, 5);
	//nwTWEAKABLE_CONST(float, PLASMA_SPLASH_DAMAGE_STRENGTH, 100);

	nwTWEAKABLE_CONST(float, PLASMA_IMPACT_IMPULSE_STRENGTH, 300);

	nwTWEAKABLE_CONST(float, PLASMA_BURNING_FIRE_RADIUS, 0.15f);
	nwTWEAKABLE_CONST(SecondsF, PLASMA_BURNING_FIRE_LIFESPAN, 4);	// NOTE: must be same as PLASMA_BURN_GLOW_SECONDS


#if GAME_CFG_WITH_PHYSICS

	for(UINT i = 0; i < phys_delta_state.plasma_bolts._count; i++)
	{
		const StateDeltaForCollisionRayCasts::DeltaPos& delta_pos = phys_delta_state.plasma_bolts._data[i];
		//
		const btVector3	bt_ray_start = toBulletVec(delta_pos.old_pos);
		const btVector3	bt_ray_end = toBulletVec(delta_pos.new_pos);
		btCollisionWorld::ClosestRayResultCallback	ray_result_callback(bt_ray_start, bt_ray_end);

		physics_world.m_dynamicsWorld.rayTest(bt_ray_start, bt_ray_end, ray_result_callback);
		if( ray_result_callback.hasHit() )
		{
			// will be removed during the next update
			plasma_bolts[ delta_pos.item_index ].life = 1.0f;

			//
			const V3f hit_pos_world = fromBulletVec(ray_result_callback.m_hitPointWorld);
			const V3f hit_normal_world = fromBulletVec(ray_result_callback.m_hitNormalWorld);

			// move the explosion a bit away from the surface so that light flash looks better
			const V3f explosion_pos_world = hit_pos_world + hit_normal_world * 0.5f;

			//
			if(btRigidBody* bt_rigid_body = (btRigidBody*) btRigidBody::upcast(ray_result_callback.m_collisionObject))
			{
				bt_rigid_body->activate(true);
				bt_rigid_body->applyCentralImpulse(
					(bt_ray_end - bt_ray_start).normalized() * PLASMA_IMPACT_IMPULSE_STRENGTH
					);

				//
				if(NwRigidBody* my_rigid_body = (NwRigidBody*) bt_rigid_body->getUserPointer())
				{
					if(EnemyNPC* enemy_npc = my_rigid_body->npc._ptr)
					{
						enemy_npc->behavior->InflictDamage(
							*enemy_npc
							, PLASMA_DAMAGE
							, hit_pos_world
							);
					}
				}
			}
			else
			{
				// Hit a (static) collision object.

				decals.AddScorchMark(
					hit_pos_world,
					hit_normal_world
					);

				//FPSGame::Get().world.voxels.voxel_engine_dbg_drawer.addPoint(
				//	V3d::fromXYZ(hit_pos_world)
				//	, RGBAf::RED
				//	, 0.1f
				//	);

				//
				FPSGame::Get().renderer.burning_fires.AddBurningFire(
					hit_pos_world
					, PLASMA_BURNING_FIRE_RADIUS
					, PLASMA_BURNING_FIRE_LIFESPAN
					);
			}

#if GAME_CFG_WITH_SOUND
			//
			sound_world.PlaySound3D(
				MakeAssetID("plasma_impact")
				, hit_pos_world
				);

			//sound_world.PlayAudioClip(
			//	MakeAssetID("expl3")
			//	);


#endif // GAME_CFG_WITH_SOUND


	
			//
			game_particles.CreateSmallCloud_PlasmaImpact(hit_pos_world);
		}
	}//test plasma bolts

#endif // GAME_CFG_WITH_PHYSICS

}

ERet GameProjectiles::RenderProjectilesIfAny(
									  MyGameRenderer& renderer
									  )
{
#if 0 && MX_DEVELOPER
	//
	nwFOR_LOOP(UINT, iRocket, flying_rockets._count)
	{
		const FlyingRocket& rocket = flying_rockets._data[ iRocket ];
		//
		renderer._render_system->_debug_renderer.DrawSolidSphere(
			rocket.pos
			, 0.1f
			, RGBAf::WHITE
			, 4, 4
			);
	}

	//// it's ok
	//nwFOR_LOOP(UINT, i, plasma_bolts._count)
	//{
	//	const PlasmaBolt& plasma_bolt = plasma_bolts._data[ i ];
	//	//
	//	renderer._render_system->_debug_renderer.DrawSolidSphere(
	//		plasma_bolt.pos
	//		, 0.1f
	//		, RGBAf::lightblue
	//		, 4, 4
	//		);
	//}
#endif

	////
	//renderer.Render_VolumetricParticlesWithShader(
	//	Arrays::getSpan(flying_rockets)
	//	);

	_RenderPlasmaBoltsIfAny(
		renderer
		);

	return ALL_OK;
}

ERet GameProjectiles::_RenderPlasmaBoltsIfAny(
	MyGameRenderer& renderer
	)
{
	if(plasma_bolts.isEmpty())
	{
		return ALL_OK;
	}

	PlasmaBolt* plasma_bolts_ptr = plasma_bolts.raw();
	UINT num_plasma_bolts = plasma_bolts.num();

	//
	ParticleVertex *	allocated_particles;
	mxDO(renderer.Render_VolumetricParticlesWithShader(
		MakeAssetID("particles_plasma_bolt")
		, num_plasma_bolts
		, allocated_particles
		));

	//

	nwFOR_LOOP(UINT, i, num_plasma_bolts)
	{
		const PlasmaBolt& plasma_bolt = plasma_bolts_ptr[ i ];

		//
		ParticleVertex &particle_vertex = *allocated_particles++;
		particle_vertex.position_and_size = V4f::set( plasma_bolt.pos, plasma_bolt.size );
		particle_vertex.color_and_life = V4f::set(
			GetPlasmaBoltColor(plasma_bolt.life),
			plasma_bolt.life
			);
	}

	return ALL_OK;
}

void GameProjectiles::CreateMuzzleSmokePuff_RocketLauncher(const V3f& pos_in_world)
{

}

const M44f M44_WorldMatrix( const V3f& eyePosition, const V3f& lookDirection, const V3f& upDirection )
{
    mxASSERT(!V3_Equal(lookDirection, V3_Zero()));
    mxASSERT(!V3_IsInfinite(lookDirection));
    mxASSERT(!V3_Equal(upDirection, V3_Zero()));
    mxASSERT(!V3_IsInfinite(upDirection));
	mxASSERT(V3_IsNormalized(lookDirection));
	mxASSERT(V3_IsNormalized(upDirection));

	V3f	axisX;	// camera right vector
	V3f	axisY;	// camera forward vector
	V3f	axisZ;	// camera up vector

	axisY = V3_Normalized( lookDirection );

	axisX = V3_Cross( axisY, upDirection );
	axisX = V3_Normalized( axisX );

	axisZ = V3_Cross( axisX, axisY );
	axisZ = V3_Normalized( axisZ );

	M44f	cameraWorldMatrix;
	cameraWorldMatrix.r0 = Vector4_Set( axisX, 0 );
	cameraWorldMatrix.r1 = Vector4_Set( axisY, 0 );
	cameraWorldMatrix.r2 = Vector4_Set( axisZ, 0 );
	cameraWorldMatrix.r3 = Vector4_Set( eyePosition, 1 );

	return cameraWorldMatrix;
}

ERet GameProjectiles::CreateRocket(
	const V3f& position_in_world
	, const V3f& direction_in_world
	, const float velocity_meters_per_second
	)
{
	const V3f	fudged_pos_to_avoid_collisions_with_player = position_in_world + direction_in_world * 0.6f;
	//const V3f	fudged_pos_to_avoid_collisions_with_player = position_in_world;

	//
	NwModel*	rocket_model;
	{
		FPSGame& game = FPSGame::Get();

		//
		NwModel_::PhysObjParams	phys_mat_info;
		phys_mat_info.restitution = 1.0f;

		//
		mxDO(NwModel_::Create(
			rocket_model

			, MakeAssetID("rocket")
			, game.runtime_clump

			, M44f::createUniformScaling(0.3f)
				* M44_RotationZ(DEG2RAD(180))	// fix the rocket facing backwards

			, *game.world.render_world
			
			, game.world.physics_world
			, PHYS_OBJ_ROCKET

			// rotate the rocket to face the target
			,M44_WorldMatrix(
				fudged_pos_to_avoid_collisions_with_player,
				direction_in_world,
				V3_UP
				)

			// use sphere, because it's less probable to tunnel thru geometry
			, NwModel_::COL_REPR::COL_SPHERE

			, direction_in_world * velocity_meters_per_second
			//, CV3f(0.1f, 0.0f, 0.0f)
			, CV3f(0)	// no rotation

			, phys_mat_info
			));
		
		//
		btRigidBody& bt_rb = rocket_model->rigid_body->bt_rb();
		btRigidBody_::DisableGravity(bt_rb);
	}


	//
	FlyingRocket	new_rocket;
	{
		new_rocket.pos = fudged_pos_to_avoid_collisions_with_player;
		new_rocket.velocity = direction_in_world * velocity_meters_per_second;
		new_rocket.life = 0;

		new_rocket.model = rocket_model;
	}

	flying_rockets.AddWithWraparoundOnOverflow(new_rocket);



	return ALL_OK;
}

void GameProjectiles::CreatePlasmaBolt(
	const V3f& position_in_world
	, const V3f& direction_in_world
	, const float velocity
	)
{
	PlasmaBolt	new_plasma_bolt;
	{
		new_plasma_bolt.pos = position_in_world;
		new_plasma_bolt.velocity = direction_in_world * velocity;
		new_plasma_bolt.life = 0;
		new_plasma_bolt.size = 0.f;
	}

	plasma_bolts.AddWithWraparoundOnOverflow(new_plasma_bolt);
}
