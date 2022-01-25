// A gib is a chunk of a body, or a piece of wood/metal/rocks/etc.
#pragma once

#include <Renderer/VertexFormats.h>	// ParticleVertex

#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
#include <Physics/RigidBody.h>
#endif // GAME_CFG_WITH_PHYSICS

#include "game_forward_declarations.h"

///TODO: a type-safe wrapper around AssetID
//template< class ASSET >
//struct TAssetID
//{
//	AssetID	id;
//};


class MyGameGibs
{
	enum
	{
		MAX_GIBS = 32,
	};

	struct GibItem
	{
		V3f			pos;
		float01_t	life;	// 0..1, dead if >= 1
		F32			inv_lifespan;
		V3f			velocity;
		float		model_scale;

		//
		NwRigidBody *	rigid_body;
		NwModel *		model;
	};
	//ASSERT_SIZEOF(GibItem, 32);

	U32		next_item_to_remove_if_full;
	TFixedArray< GibItem, MAX_GIBS >	alive_gib_items;

public:
	MyGameGibs();

	void UpdateOncePerFrame(
		const NwTime& game_time
		//, GameParticleRenderer& game_particles
		, ARenderWorld* render_world
		//, StateDeltaForCollisionRayCasts &phys_delta_state
		);

public:
	ERet SpawnGibs(
		const V3f& position_in_world
		, const V3f& model_center
		, const AABBf& model_aabb_world
		, const float model_scale = 0.3f
	
		, const int git_item_count = 4
		, const float git_item_velocity = 4.0f
		);

	ERet SpawnSingleGibItem(
		const V3f& position_in_world
		, const AssetID& model_def
		, const V3f& velocity_in_world
		, const V3f& angular_velocity
		, const float model_scale = 0.3f
		);

	void PlayMeatSplatSound(
		const V3f& position_in_world
		);

private:
	void _RemoveResourcesUsedByOldGibItem(
		const GibItem& old_gib_item
		);
};
