#include "stdafx.h"
#pragma hdrstop

#include <Core/Util/Tweakable.h>
#include <Core/Tasking/TaskSchedulerInterface.h>	// threadLocalHeap();

#include "rendering/game_renderer.h"
#include "rendering/particles/game_particles.h"


static const V3f EXPLOSION_LIGHT_COLOR = { 1.0f, 0.8f, 0.4f };


ERet GameParticleRenderer::Initialize(
	const NwEngineSettings& engine_settings
	, const RrGlobalSettings& renderer_settings
	, NwClump* scene_clump
	)
{
	return ALL_OK;
}

void GameParticleRenderer::Shutdown()
{

}

void GameParticleRenderer::AdvanceParticlesOncePerFrame(
	const NwTime& game_time
	)
{
	_AdvanceSmokePuffs(
		game_time
		);
	_AdvanceExlosions(
		game_time
		);
}

void GameParticleRenderer::_AdvanceSmokePuffs(
	const NwTime& game_time
	)
{
	const SecondsF particle_life_span_seconds = 4.0f;
	const F32 particle_lifespan_inv = 1.0f / particle_life_span_seconds;

	//
	SmokePuff* particles = smoke_puffs.raw();
	UINT num_alive_particles = smoke_puffs.num();

	UINT iParticle = 0;

	while( iParticle < num_alive_particles )
	{
		SmokePuff &	smoke_puff = particles[ iParticle ];

		const F32 particle_life = smoke_puff.life;
		const F32 particle_life_new = particle_life + game_time.real.delta_seconds * particle_lifespan_inv;

		if( particle_life_new < 1.0f )
		{
			// the smoke_puff is still alive
			++iParticle;

			//
			const F32 particle_size_new = smoke_puff.min_radius
				+ (smoke_puff.max_radius - smoke_puff.min_radius) * particle_life_new;
				;

			smoke_puff.curr_radius = particle_size_new;
			smoke_puff.life = particle_life_new;
		}
		else
		{
			// the smoke_puff is dead - remove it
			TSwap( smoke_puff, particles[ --num_alive_particles ] );
		}
	}

	//
	smoke_puffs.setNum( num_alive_particles );
}




void GameParticleRenderer::_AdvanceExlosions(
	const NwTime& game_time
	)
{
	const SecondsF explosion_life_span_seconds = 1.0f;
	const F32 explosion_lifespan_inv = 1.0f / explosion_life_span_seconds;

	////
	static float EXPLOSION_LIGHT_MIN_RADIUS = 0.0f;
	HOT_VAR(EXPLOSION_LIGHT_MIN_RADIUS);

	static float EXPLOSION_LIGHT_MAX_RADIUS = 10.0f;
	HOT_VAR(EXPLOSION_LIGHT_MAX_RADIUS);

	//
	Explosion* explosions_ptr = explosions.raw();
	UINT num_alive_explosions = explosions.num();

	UINT explosion_idx = 0;

	while( explosion_idx < num_alive_explosions )
	{
		Explosion &	explosion = explosions_ptr[ explosion_idx ];

		const F32 explosion_life = explosion.life;
		const F32 explosion_life_new = explosion_life + game_time.real.delta_seconds * explosion_lifespan_inv;

		if( explosion_life_new < 1.0f )
		{
			// the explosion is still alive
			++explosion_idx;

			//
			const F32 explosion_radius_new = EXPLOSION_LIGHT_MIN_RADIUS
				+ (EXPLOSION_LIGHT_MAX_RADIUS - EXPLOSION_LIGHT_MIN_RADIUS) * explosion_life_new
				;

			explosion.radius = explosion_radius_new;
			explosion.life = explosion_life_new;
		}
		else
		{
			// the explosion is dead - remove it
			TSwap( explosion, explosions_ptr[ --num_alive_explosions ] );
		}
	}

	//
	explosions.setNum( num_alive_explosions );
}



template <class T> void QuickDepthSort( T* indices, float* depths, int lo, int hi )
{
    //  lo is the lower index, hi is the upper index
    //  of the region of array a that is to be sorted
    int i = lo, j = hi;
    float h;
    T index;
    float x = depths[( lo + hi ) / 2];

    //  partition
    do
    {
        while( depths[i] < x ) i++;
        while( depths[j] > x ) j--;
        if( i <= j )
        {
            h = depths[i]; depths[i] = depths[j]; depths[j] = h;
            index = indices[i]; indices[i] = indices[j]; indices[j] = index;
            i++; j--;
        }
    } while( i <= j );

    //  recursion
    if( lo < j ) QuickDepthSort( indices, depths, lo, j );
    if( i < hi ) QuickDepthSort( indices, depths, i, hi );
}

void GameParticleRenderer::RenderParticlesIfAny(
	SgRenderer& renderer
	, SpatialDatabaseI* spatial_database
	)
{
	_RenderSmokePuffsIfAny(
		renderer
		);
	_RenderExplosionsIfAny(
		renderer
		, spatial_database
		);
}

ERet GameParticleRenderer::_RenderSmokePuffsIfAny(
	SgRenderer& renderer
	)
{
	const UINT num_alive_smoke_puffs = smoke_puffs.num();
	if(!num_alive_smoke_puffs) {
		return ALL_OK;
	}

	// Sort smoke puffs to render them back-to-front.

	AllocatorI& scratch_alloc = threadLocalHeap();

	//
	int *	sorted_particle_indices;
	mxTRY_ALLOC_SCOPED( sorted_particle_indices, num_alive_smoke_puffs, scratch_alloc );

	float *	sorted_particle_depths;
	mxTRY_ALLOC_SCOPED( sorted_particle_depths, num_alive_smoke_puffs, scratch_alloc );

	nwFOR_LOOP(UINT, i, num_alive_smoke_puffs)
	{
		const SmokePuff& smoke_puff = smoke_puffs._data[ i ];

        sorted_particle_indices[i] = i;
		sorted_particle_depths[i] = (smoke_puff.pos - renderer.scene_view.eye_pos).lengthSquared();
	}

	QuickDepthSort(
		sorted_particle_indices, sorted_particle_depths,
		0, num_alive_smoke_puffs - 1
		);


	//
	ParticleVertex *	allocated_particles;
	mxDO(renderer.Render_VolumetricParticlesWithShader(
		MakeAssetID("particles_rocket_smoke_trail")
		, num_alive_smoke_puffs
		, allocated_particles
		));

	//

	nwFOR_LOOP(UINT, i, num_alive_smoke_puffs)
	{
		const SmokePuff& smoke_puff = smoke_puffs._data[ sorted_particle_indices[ num_alive_smoke_puffs - i - 1 ] ];

		//
		ParticleVertex &particle_vertex = *allocated_particles++;
		particle_vertex.position_and_size = V4f::set( smoke_puff.pos, smoke_puff.curr_radius );
		particle_vertex.color_and_life.w = smoke_puff.life;
	}

	return ALL_OK;
}



ERet GameParticleRenderer::_RenderExplosionsIfAny(
	SgRenderer& renderer
	, SpatialDatabaseI* spatial_database
	)
{
	const UINT num_alive_explosions = explosions.num();
	if(!num_alive_explosions) {
		return ALL_OK;
	}

	//
	nwFOR_LOOP(UINT, i, num_alive_explosions)
	{
		const Explosion& explosion = explosions._data[ i ];

		////
		//ParticleVertex &particle_vertex = *allocated_particles++;
		//particle_vertex.position_and_size = V4f::set( explosion.center, explosion.radius );
		//particle_vertex.color_and_life.w = explosion.life;

		// add (1-..) so that stars fast and decays slow
		const float explosion_brightness = CalcExplosionBrightness(explosion.life);

		//
		{
			TbDynamicPointLight	explosion_light;
			explosion_light.position = explosion.center;
			explosion_light.radius = explosion.radius;
			explosion_light.color = EXPLOSION_LIGHT_COLOR * explosion_brightness;

			spatial_database->AddDynamicPointLight(explosion_light);
		}
	}

	return ALL_OK;
}

void GameParticleRenderer::CreateMuzzleSmokePuff_RocketLauncher(const V3f& pos_in_world)
{
//	DBGOUT("UNDONE;");
}

void GameParticleRenderer::CreateSmokePuff_RocketTrail(
	const V3f& position_in_world
	)
{
	////
	static float ROCKET_SMOKE_PUFF_MIN_RADIUS = 0.5f;
	HOT_VAR(ROCKET_SMOKE_PUFF_MIN_RADIUS);

	static float ROCKET_SMOKE_PUFF_MAX_RADIUS = 4.0f;
	HOT_VAR(ROCKET_SMOKE_PUFF_MAX_RADIUS);

	// so that smoke puffs are different
	const int ix = always_cast<int>(position_in_world.x);
	const int iy = always_cast<int>(position_in_world.y);
	const int iz = always_cast<int>(position_in_world.z);

	NwRandom	random2(
		smoke_puffs.num() ^ Hash32Bits_0(ix ^ iy ^ iz)
		);

	const float	size_mul = random2.RandomFloat(0.5f, 2.0f);

	//
	SmokePuff	new_smoke_puff;
	{
		new_smoke_puff.pos = position_in_world;
		new_smoke_puff.curr_radius = ROCKET_SMOKE_PUFF_MIN_RADIUS * size_mul;

		new_smoke_puff.min_radius = ROCKET_SMOKE_PUFF_MIN_RADIUS * size_mul;
		new_smoke_puff.max_radius = ROCKET_SMOKE_PUFF_MAX_RADIUS * size_mul;
		new_smoke_puff.life = 0;	// initial life
	}
	smoke_puffs.AddWithWraparoundOnOverflow( new_smoke_puff );
}

void GameParticleRenderer::CreateSmallCloud_PlasmaImpact(
	const V3f& position_in_world
	)
{
	////
	static float PLASMA_SMOKE_PUFF_MIN_RADIUS = 0.1f;
	HOT_VAR(PLASMA_SMOKE_PUFF_MIN_RADIUS);

	static float PLASMA_SMOKE_PUFF_MAX_RADIUS = 2.2f;
	HOT_VAR(PLASMA_SMOKE_PUFF_MAX_RADIUS);

	// so that smoke puffs are different
	const int ix = always_cast<int>(position_in_world.x);
	const int iy = always_cast<int>(position_in_world.y);
	const int iz = always_cast<int>(position_in_world.z);

	NwRandom	random2(
		smoke_puffs.num() ^ Hash32Bits_0(ix ^ iy ^ iz)
		);

	const float	size_mul = random2.RandomFloat(0.5f, 2.0f);

	//
	SmokePuff	new_smoke_puff;
	{
		new_smoke_puff.pos = position_in_world;
		new_smoke_puff.curr_radius = PLASMA_SMOKE_PUFF_MIN_RADIUS * size_mul;

		new_smoke_puff.min_radius = PLASMA_SMOKE_PUFF_MIN_RADIUS * size_mul;
		new_smoke_puff.max_radius = PLASMA_SMOKE_PUFF_MAX_RADIUS * size_mul;
		new_smoke_puff.life = 0;	// initial life
	}
	smoke_puffs.AddWithWraparoundOnOverflow( new_smoke_puff );
}

void GameParticleRenderer::CreateExplosion_from_Rocket(
	const V3f& position_in_world
	)
{
	Explosion	new_explosion;
	{
		new_explosion.center = position_in_world;
		new_explosion.radius = 0;
		new_explosion.life = 0;	// initial life
	}
	explosions.AddWithWraparoundOnOverflow(new_explosion);
}
