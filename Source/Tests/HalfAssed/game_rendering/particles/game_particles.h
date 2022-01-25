#pragma once

#include <Base/Math/Random.h>
#include <Renderer/VertexFormats.h>	// ParticleVertex
#include "game_forward_declarations.h"


class GameParticleRenderer
{
public_internal:

	enum
	{
		/// e.g. rocket smoke trails
		MAX_SMOKE_PUFFS = 1024,
	
		/// e.g. exploding rockets
		MAX_EXPLOSIONS = 128,
	};


	//
	idRandom	random;

	//
	struct SmokePuff
	{
		V3f			pos;	// in world space
		float		curr_radius;

		float		min_radius;
		float		max_radius;
		float01_t	life;	// 0..1, dead if > 1
		float		unused[1];
	};
	ASSERT_SIZEOF(SmokePuff, 32);

	TFixedArray< SmokePuff, MAX_SMOKE_PUFFS >	smoke_puffs;


	//
	struct Explosion
	{
		V3f			center;
		float		radius;	// light flash radius in world space
		float01_t	life;	// 0..1, dead if > 1
		float		unused[3];
	};
	ASSERT_SIZEOF(Explosion, 32);

	TFixedArray< Explosion, MAX_EXPLOSIONS >	explosions;

public:
	ERet Initialize(
		const NwEngineSettings& engine_settings
		, const RrRuntimeSettings& renderer_settings
		, NwClump* scene_clump
		);

	void Shutdown();

	void AdvanceParticlesOncePerFrame(
		const NwTime& game_time
		);

	void RenderParticlesIfAny(
		MyGameRenderer& renderer
		, ARenderWorld* render_world
		);

private:
	void _AdvanceSmokePuffs(
		const NwTime& game_time
		);
	void _AdvanceExlosions(
		const NwTime& game_time
		);

	ERet _RenderSmokePuffsIfAny(
		MyGameRenderer& renderer
		);
	ERet _RenderExplosionsIfAny(
		MyGameRenderer& renderer
		, ARenderWorld* render_world
		);

public:
	void CreateMuzzleSmokePuff_RocketLauncher(const V3f& pos_in_world);

	void CreateSmokePuff_RocketTrail(
		const V3f& position_in_world
		);

	void CreateSmallCloud_PlasmaImpact(
		const V3f& position_in_world
		);

	void CreateExplosion_from_Rocket(
		const V3f& position_in_world
		);
};


/// Use it only if you know whatcha doin'!
template< typename T, UINT ALIGNMENT >
void AddUnsafe_NoOverflowCheck(
								 DynamicArray< T, ALIGNMENT > &a,
								 const T& new_item
								 )
{
	mxASSERT(a._count < a.capacity());
	a._data[ a._count++ ] = new_item;
}

/// Use it only if you know whatcha doin'!
template< typename T, UINT ALIGNMENT >
T& AllocateUnsafe_NoOverflowCheck(
									DynamicArray< T, ALIGNMENT > &a
								 )
{
	mxASSERT(a._count < a.capacity());
	return a._data[ a._count++ ];
}
