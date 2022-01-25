// medkits, ammo boxes, valuable items, etc.
#pragma once

#include <Renderer/VertexFormats.h>	// ParticleVertex

#if GAME_CFG_WITH_PHYSICS
#include <Physics/PhysicsWorld.h>
#include <Physics/RigidBody.h>
#endif // GAME_CFG_WITH_PHYSICS

#include "game_forward_declarations.h"
#include "gameplay_constants.h"


namespace MyGameUtils
{
	ERet SpawnItem(
		const Gameplay::EPickableItem item_type
		, const V3f& pos_ws
		);

	bool PickItemIfShould(
		int col_obj_item_type
		);

}//namespace MyGameUtils
