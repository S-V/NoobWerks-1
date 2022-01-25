#pragma once

#include <Renderer/VertexFormats.h>	// ParticleVertex
#include "game_forward_declarations.h"


class GameProjectiles
{
	enum
	{
		MAX_FLYING_ROCKETS = 64,
		MAX_FLYING_PLASMA = 128,
		MAX_FLYING_BULLETS = 512,
	};

	struct FlyingRocket
	{
		V3f			pos;
		float01_t	life;	// 0..1, dead if >= 1

		V3f			velocity;
		float		pad;

		///
		NwModel*	model;

		//
		V3f		hit_pos;
		V3f		hit_normal;
		//bool	hit_anything;
	};
	//ASSERT_SIZEOF(FlyingRocket, 32);

	TFixedArray< FlyingRocket, MAX_FLYING_ROCKETS >	flying_rockets;

	//

	struct PlasmaBolt
	{
		V3f			pos;
		float01_t	life;	// 0..1, dead if > 1
		V3f			velocity;
		float		size;	//
	};
	ASSERT_SIZEOF(PlasmaBolt, 32);

	TFixedArray< PlasmaBolt, MAX_FLYING_PLASMA >	plasma_bolts;


public:
	ERet Initialize(
		const NwEngineSettings& engine_settings
		, const RrRuntimeSettings& renderer_settings
		, NwClump* scene_clump
		);

	void Shutdown();

public:
	struct StateDeltaForCollisionRayCasts: NonCopyable
	{
		struct DeltaPos
		{
			V3f		old_pos;
			U32		item_index;
			V3f		new_pos;
			float	pad1;
		};
		ASSERT_SIZEOF(DeltaPos, 32);

		DynamicArray< DeltaPos >	rockets;
		DynamicArray< DeltaPos >	plasma_bolts;

	public:
		StateDeltaForCollisionRayCasts(AllocatorI& allocator)
			: rockets(allocator)
			, plasma_bolts(allocator)
		{
		}
	};

	void AdvanceParticlesOncePerFrame(
		const NwTime& game_time
		, GameParticleRenderer& game_particles
		, ARenderWorld* render_world
		, StateDeltaForCollisionRayCasts &phys_delta_state
		);

	void CollideWithWorld(
		const StateDeltaForCollisionRayCasts& phys_delta_state
		, NwPhysicsWorld& physics_world
		, NwSoundSystemI& sound_world
		, GameParticleRenderer& game_particles
		, NwDecalsSystem& decals
		, VX5::WorldI* voxel_world
		, VX5::NoiseBrushI* crater_noise_brush
		);

	/// may add smoke trails to the particle renderer
	ERet RenderProjectilesIfAny(
		MyGameRenderer& renderer
		);

private:
	ERet _AdvanceRocketsIfAny(
		const NwTime& game_time
		, GameParticleRenderer& game_particles
		, ARenderWorld* render_world
		, StateDeltaForCollisionRayCasts &phys_delta_state
		);
	ERet _AdvancePlasmaBoltsIfAny(
		const NwTime& game_time
		, GameParticleRenderer& game_particles
		, ARenderWorld* render_world
		, StateDeltaForCollisionRayCasts &phys_delta_state
		);

	ERet _RenderPlasmaBoltsIfAny(
		MyGameRenderer& renderer
		);

private:
	void _CollideRockets(
		const StateDeltaForCollisionRayCasts& phys_delta_state
		, NwPhysicsWorld& physics_world
		, NwSoundSystemI& sound_world
		, GameParticleRenderer& game_particles
		, VX5::WorldI* voxel_world
		, VX5::NoiseBrushI* crater_noise_brush
		);
	void _CollidePlasmaBolts(
		const StateDeltaForCollisionRayCasts& phys_delta_state
		, NwPhysicsWorld& physics_world
		, NwSoundSystemI& sound_world
		, GameParticleRenderer& game_particles
		, NwDecalsSystem& decals
		, VX5::WorldI* voxel_world
		);

public:
	ERet CreateRocket(
		const V3f& position_in_world
		, const V3f& direction_in_world
		, const float velocity_meters_per_second
		);
	void CreatePlasmaBolt(
		const V3f& position_in_world
		, const V3f& direction_in_world
		, const float velocity
		);

public:
	void CreateMuzzleSmokePuff_RocketLauncher(const V3f& pos_in_world);
};
