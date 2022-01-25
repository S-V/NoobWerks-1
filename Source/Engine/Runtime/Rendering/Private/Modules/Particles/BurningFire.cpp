#include <Base/Base.h>
#pragma hdrstop

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

#include <Rendering/Private/Modules/Particles/BurningFire.h>


namespace Rendering
{
BurningFireSystem::BurningFireSystem()
{
}

void BurningFireSystem::UpdateOncePerFrame(
	const NwTime& game_time
	)
{
	static SecondsF BURNING_FIRE_ANIM_DURATION_SECONDS = 4.0f;
	HOT_VAR(BURNING_FIRE_ANIM_DURATION_SECONDS);

	const F32 inv_burning_fire_anim_seconds = 1.0f / BURNING_FIRE_ANIM_DURATION_SECONDS;

	//////
	//static float FIRE_RADIUS = 0.4f;
	//HOT_VAR(FIRE_RADIUS);

	//
	BurningFire* active_items_ptr = burning_fires.raw();
	UINT num_active_items = burning_fires.num();

	UINT active_item_idx = 0;

	while( active_item_idx < num_active_items )
	{
		BurningFire &	burning_fire = active_items_ptr[ active_item_idx ];

		const F32 updated_life = burning_fire.life01
			+ game_time.real.delta_seconds * burning_fire.inv_lifespan_sec
			;

		// if the particle is alive:
		if( updated_life < 1 )
		{
			burning_fire.life01 = updated_life;

			//
			burning_fire.radius_WS = burning_fire.initial_radius * (1 - updated_life);

			//
			F32 anim_cycle_new = burning_fire.anim_cycle
				+ game_time.real.delta_seconds * inv_burning_fire_anim_seconds
				;

			// wrap around anim cycle
			if( anim_cycle_new > 1.0f )
			{
				anim_cycle_new -= 1.0f;
			}

			burning_fire.anim_cycle = anim_cycle_new;

			// the particle is alive
			++active_item_idx;
		}
		else
		{
			// the particle is dead - remove it (swap with last and decrease count)
			TSwap(
				burning_fire,
				active_items_ptr[ --num_active_items ]
			);
		}
	}

	//
	burning_fires.setNum( num_active_items );
}

ERet BurningFireSystem::Render(
	SpatialDatabaseI* spatial_database
	)
{
	const UINT num_active_items = burning_fires.num();
	if(!num_active_items) {
		return ALL_OK;
	}

	//
	AllocatorI& scratch_alloc = threadLocalHeap();

	//
	RrBillboardVertex *	allocated_vertices;

	NwShaderEffect* shader_effect = nil;
	mxDO(Resources::Load( shader_effect,
		MakeAssetID("billboard_fire")
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
		mxDO(Resources::Load(sprite_tex, MakeAssetID("flames4x4")));

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
	const V3f BURNING_FIRE_COLOR = CV3f(0.95f, 0.3f, 0.2f);
UNDONE;
#if 0
	//
	nwFOR_LOOP(UINT, i, num_active_items)
	{
		const BurningFire& burning_fire = burning_fires._data[ i ];

		//
		RrBillboardVertex &new_vertex = *allocated_vertices++;
		new_vertex.position_and_size = V4f::set(
			burning_fire.center_WS,
			burning_fire.radius_WS
			);
		new_vertex.color_and_life.w = burning_fire.anim_cycle;

		//
		if( burning_fire.life01 < 1.0f )
		{
			TbDynamicPointLight	plasma_light;
			plasma_light.position = burning_fire.center_WS;
			plasma_light.radius = 3.0f; // in meters
			plasma_light.color = BURNING_FIRE_COLOR * maxf(1 - burning_fire.life01, 0);

			spatial_database->AddDynamicPointLight(plasma_light);
		}
	}
#endif
	return ALL_OK;
}

void BurningFireSystem::AddBurningFire(
	const V3f& pos_world
	, const float radius
	, const SecondsF lifespan
	)
{
	BurningFire	new_burning_fire;
	{
		new_burning_fire.center_WS = pos_world;
		new_burning_fire.radius_WS = radius;
		new_burning_fire.initial_radius = radius;

		new_burning_fire.life01 = 0;
		new_burning_fire.inv_lifespan_sec = (lifespan > 0)
			? (1.0f / lifespan)
			: 0
			;
		new_burning_fire.anim_cycle = 0;
	}

	burning_fires.AddWithWraparoundOnOverflow(new_burning_fire);
}

}//namespace Rendering
