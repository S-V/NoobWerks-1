#include "stdafx.h"
#pragma hdrstop

#include <Base/Math/Random.h>
#include <Core/Util/Tweakable.h>
#include <Renderer/Core/MeshInstance.h>

#include "game_compile_config.h"
#include "game_rendering/game_renderer.h"
#include "game_rendering/particles/game_particles.h"
#include "FPSGame.h"
#include "gameplay_constants.h"

#if GAME_CFG_WITH_SOUND
#include <Sound/SoundSystem.h>
#endif

#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
#endif

#include <Engine/Model.h>

#include "game_world/explosions.h"


namespace MyGameUtils
{

struct ExplosionSettings
{
	float influence_radius;
	float impulse_strength;

	float splash_damage_min_radius;
	float splash_damage_max_radius;
	float splash_damage_max_amount;
};


static
void Apply_ExplosionImpulse_and_Inflict_SplashDamage(
	const V3f& explosion_center

	, const ExplosionSettings& settings

	, NwPhysicsWorld& physics_world
	)
{

#if GAME_CFG_WITH_PHYSICS

	struct MyBroadphaseAabbCallback: btBroadphaseAabbCallback
	{
		const V3f	explosion_center;
		const ExplosionSettings	settings;

	public:
		MyBroadphaseAabbCallback(const V3f& explosion_center, const ExplosionSettings& settings)
			: explosion_center(explosion_center)
			, settings(settings)
		{
		}
		virtual bool	process(const btBroadphaseProxy* proxy) override
		{
			btCollisionObject* col_obj = (btCollisionObject*) proxy->m_clientObject;

			if(btRigidBody* bt_rigid_body = btRigidBody::upcast(col_obj))
			{
				const V3f vec_from_explosion = fromBulletVec(
					bt_rigid_body->getCenterOfMassPosition()
					) - explosion_center;

				float dist_from_explosion;
				const V3f dir_from_explosion = V3_Normalized(vec_from_explosion, dist_from_explosion);

				//
				bt_rigid_body->activate(true);
				bt_rigid_body->applyCentralImpulse(
					toBulletVec(dir_from_explosion * dist_from_explosion * settings.impulse_strength)
					);

				//
				const float splash_damage_amount01 = clampf(
					1 - dist_from_explosion / settings.splash_damage_max_radius
					, 0, 1
					);
				if( splash_damage_amount01 > 0 )
				{
					const float splash_damage = splash_damage_amount01 * settings.splash_damage_max_amount;

					//
					if(NwRigidBody* my_rigid_body = (NwRigidBody*) bt_rigid_body->getUserPointer())
					{
						if(EnemyNPC* enemy_npc = my_rigid_body->npc._ptr)
						{
							enemy_npc->behavior->InflictDamage(
								*enemy_npc
								, splash_damage
								, my_rigid_body->GetCenterWorld()
								);
						}
					}

					//
					if(col_obj->getUserIndex() == PHYS_OBJ_PLAYER_CHARACTER)
					{
						const float MAX_SPLASH_DAMAGE_TO_PLAYER = 30;

						FPSGame::Get().player.DealDamage(
							splash_damage_amount01 * MAX_SPLASH_DAMAGE_TO_PLAYER
							);
					}
				}
			}

			return true;
		}
	} aabb_callback(explosion_center, settings);
	
	//
	const btVector3& aabbMin = toBulletVec(explosion_center - CV3f(settings.influence_radius));
	const btVector3& aabbMax = toBulletVec(explosion_center + CV3f(settings.influence_radius));

	physics_world.m_broadphase.aabbTest(
		aabbMin, aabbMax,
		aabb_callback
		);

#endif // GAME_CFG_WITH_PHYSICS

}


	ERet Explode_Rocket(
		const V3f& explosion_center
		, const V3f surface_normal
		, const int rng_seed /*= 0*/
		)
	{
		//
		FPSGame& game = FPSGame::Get();

		//
		GameParticleRenderer& game_particles = game.renderer.particle_renderer;

		NwPhysicsWorld& physics_world = game.world.physics_world;
		NwSoundSystemI& sound_world = game.sound;
		VX5::WorldI* voxel_world = game.world.voxels.voxel_world;
		VX5::NoiseBrushI* crater_noise_brush = &game.world.voxels;

		//
		nwNON_TWEAKABLE_CONST(float, ROCKET_EXPLOSION_INFLUENCE_RADIUS, 10);
		nwNON_TWEAKABLE_CONST(float, ROCKET_EXPLOSION_IMPULSE_STRENGTH, 120);//PUSH

		nwNON_TWEAKABLE_CONST(float, SPLASH_DAMAGE_MAX_RADIUS, 10);
		nwNON_TWEAKABLE_CONST(float, SPLASH_DAMAGE_MAX_AMOUNT, 400);

		nwTWEAKABLE_CONST(float, ROCKET_EXPLOSION_CSG_RADIUS, 1.9f);
		nwNON_TWEAKABLE_CONST(float, ROCKET_EXPLOSION_CRATER_RIM_STRENGTH, 1.0f);

		//
		nwNON_TWEAKABLE_CONST(float, ROCKET_EXPLOSION_PARTICLE_RADIUS, 8);
		nwNON_TWEAKABLE_CONST(SecondsF, ROCKET_EXPLOSION_PARTICLE_LIFESPAN, 0.5f);


		//
		game_particles.CreateExplosion_from_Rocket(explosion_center);


		//
		ExplosionSettings	explosion_settings;
		{
			explosion_settings.influence_radius = ROCKET_EXPLOSION_INFLUENCE_RADIUS;
			explosion_settings.impulse_strength = ROCKET_EXPLOSION_IMPULSE_STRENGTH;

			explosion_settings.splash_damage_min_radius = 0;
			explosion_settings.splash_damage_max_radius = SPLASH_DAMAGE_MAX_RADIUS;
			explosion_settings.splash_damage_max_amount = SPLASH_DAMAGE_MAX_AMOUNT;
		}

		//
		Apply_ExplosionImpulse_and_Inflict_SplashDamage(
			explosion_center

			, explosion_settings

			, physics_world
			);

		//
		game.renderer.explosions.AddExplosion(
			// move 1 m in front of surface to reduce clipping by geometry
			explosion_center + surface_normal

			, ROCKET_EXPLOSION_PARTICLE_RADIUS
			, ROCKET_EXPLOSION_PARTICLE_LIFESPAN

			, rng_seed // rng seed
			);

#if GAME_CFG_WITH_SOUND
		//
		sound_world.PlaySound3D(
			MakeAssetID("rocket_impact")
			, explosion_center
			);
#endif // GAME_CFG_WITH_SOUND


		//
#if 1

		const MyGameplaySettings& gameplay_settings = game.user_settings.ingame.gameplay;

		if(gameplay_settings.destructible_environment)
		{
			const CV3f	GROUND_NORMAL(0,0,1);
			const bool probably_hit_the_floor = V3f::dot( surface_normal, GROUND_NORMAL ) >= 0.9f;

			// so that explosion craters are different
			idRandom	explosion_crater_size_random(
				rng_seed
				);

#if 1//GAME_CFG_RELEASE_BUILD
			const float	explosion_crater_random_radius_scale = explosion_crater_size_random.RandomFloat(0.9f, 1.1f);
#else // DEBUGGING CSG
			const float	explosion_crater_random_radius_scale = 1;
#endif

			VX5::ExplosionBrush	explosion_brush;
			explosion_brush.center = V3d::fromXYZ(explosion_center);
			explosion_brush.radius = ROCKET_EXPLOSION_CSG_RADIUS * explosion_crater_random_radius_scale;
			explosion_brush.surface_normal = surface_normal;
			explosion_brush.crater_rim_strength = ROCKET_EXPLOSION_CRATER_RIM_STRENGTH;
			explosion_brush.noise_brush = crater_noise_brush;



			if(probably_hit_the_floor)
			{
				if(gameplay_settings.destructible_floor)
				{
					if(gameplay_settings.shallow_craters)
					{
						explosion_brush.center = V3d::fromXYZ(
							explosion_center + surface_normal * explosion_brush.radius * 0.8f
							);
					}

					voxel_world->SubtractExplosionAsync(
						explosion_brush
						);
				}
			}
			else
			{
				voxel_world->SubtractExplosionAsync(
					explosion_brush
					);
			}
		}
#endif

		return ALL_OK;
	}

}//namespace MyGameUtils
