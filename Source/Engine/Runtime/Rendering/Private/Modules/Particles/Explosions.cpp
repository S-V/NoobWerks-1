#include <Base/Base.h>
#pragma hdrstop

#include <Base/Math/Random.h>

#include <Core/Client.h>
#include <Core/CoreDebugUtil.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>	// threadLocalHeap()
#include <Core/Util/Tweakable.h>

#include <GPU/Public/render_context.h>
#include <Graphics/Public/graphics_shader_system.h>

#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Public/Utility/DrawPointList.h>

#include <Rendering/Private/Modules/Particles/Explosions.h>

namespace Rendering
{
RrExplosionsSystem::RrExplosionsSystem()
{
}

void RrExplosionsSystem::UpdateOncePerFrame(
	const NwTime& game_time
	)
{
	//
	Explosion* active_items_ptr = explosions.raw();
	UINT num_active_items = explosions.num();

	UINT active_item_idx = 0;

	while( active_item_idx < num_active_items )
	{
		Explosion &	explosion = active_items_ptr[ active_item_idx ];

		const F32 updated_life = explosion.life01
			+ game_time.real.delta_seconds * explosion.inv_lifespan_sec
			;

		// if the particle is alive:
		if( updated_life < 1 )
		{
			explosion.life01 = updated_life;

			explosion.rotation_angle_rad += explosion.rotation_speed * game_time.real.delta_seconds;

			//DBGOUT("Life = %f", updated_life);

			//
			//explosion.radius_WS = explosion.max_radius * (1 - updated_life);


			////
			//F32 anim_cycle_new = explosion.anim_cycle
			//	+ game_time.real.delta_seconds * inv_explosion_anim_seconds
			//	;

			//// wrap around anim cycle
			//if( anim_cycle_new > 1.0f )
			//{
			//	anim_cycle_new -= 1.0f;
			//}

			//explosion.anim_cycle = anim_cycle_new;

			// the particle is alive
			++active_item_idx;
		}
		else
		{
			// the particle is dead - remove it (swap with last and decrease count)
			TSwap(
				explosion,
				active_items_ptr[ --num_active_items ]
			);
		}
	}

	//
	explosions.setNum( num_active_items );
}

ERet RrExplosionsSystem::Render(
	SpatialDatabaseI* spatial_database
	)
{
	const UINT num_active_items = explosions.num();
	if(!num_active_items) {
		return ALL_OK;
	}

	//
	AllocatorI& scratch_alloc = threadLocalHeap();

	//
	RrBillboardVertex *	allocated_vertices;

	NwShaderEffect* shader_effect = nil;
	mxDO(Resources::Load( shader_effect,
		MakeAssetID("rocket_explosion")
		));

	{
		const RenderSystemData& render_sys = *Globals::g_data;
		//
		/*mxDO*/(shader_effect->SetInput(
			nwNAMEHASH("DepthTexture"),
			NGpu::AsInput(render_sys._deferred_renderer.m_depthRT->handle)
			));

		//
		NwTexture *	sprite_tex;
		mxDO(Resources::Load(sprite_tex, MakeAssetID("boomboom")));

		mxDO(shader_effect->SetInput(
			nwNAMEHASH("t_sprite"),
			sprite_tex->m_resource
			));
	}

	mxDO(RrUtils::T_DrawPointListWithShader(
		allocated_vertices
		, *shader_effect
		, num_active_items
		));

	//
	const V3f EXPLOSION_COLOR = CV3f(0.95f, 0.3f, 0.2f);
UNDONE;
#if 0
	//
	nwFOR_LOOP(UINT, i, num_active_items)
	{
		const Explosion& explosion = explosions._data[ i ];

		//
		RrBillboardVertex &new_vertex = *allocated_vertices++;

		//
		//const float explosion_brightness = CalcExplosionBrightness(explosion.life01);
		const float alpha = 1 - EaseInOutBack(explosion.life01);
		const float explosion_radius = explosion.life01 * explosion.max_radius;

		new_vertex.position_and_size = V4f::set(
			explosion.center_WS,
			explosion_radius
			);

		// x - rotation in [0..1]
		// y - opacity
		// w: life in [0..1], if life > 1 => particle is dead
		new_vertex.color_and_life.x = explosion.rotation_angle_rad;
		new_vertex.color_and_life.y = alpha;
		new_vertex.color_and_life.w = explosion.life01;

		//
		if( explosion.life01 < 1.0f )
		{
			TbDynamicPointLight	plasma_light;
			plasma_light.position = explosion.center_WS;
			plasma_light.radius = explosion_radius;
			plasma_light.color = EXPLOSION_COLOR * maxf(1 - explosion.life01, 0);

			spatial_database->AddDynamicPointLight(plasma_light);
		}
	}
#endif
	return ALL_OK;
}

void RrExplosionsSystem::AddExplosion(
	const V3f& pos_world
	, const float radius
	, const SecondsF lifespan
	, int rng_seed
	)
{
	Explosion	new_explosion;
	{
		new_explosion.center_WS = pos_world;
		//new_explosion.radius_WS = radius;
		new_explosion.max_radius = radius;

		new_explosion.life01 = 0;
		new_explosion.inv_lifespan_sec = (lifespan > 0)
			? (1.0f / lifespan)
			: 0
			;

		//
		NwRandom	rng(rng_seed);
		new_explosion.rotation_angle_rad = rng.GetRandomFloatInRange(0, mxTWO_PI);
		new_explosion.rotation_speed = rng.GetRandomFloatInRange(-0.2f, +0.2f);
	}

	explosions.AddWithWraparoundOnOverflow(new_explosion);
}

float01_t CalcExplosionBrightness( float01_t explosion_life )
{
	// add (1-..) so that stars fast and decays slow
	float explosion_brightness = (1 - EaseInOutBack(explosion_life)) * 2;
	return maxf(0, explosion_brightness);
}

}//namespace Rendering
