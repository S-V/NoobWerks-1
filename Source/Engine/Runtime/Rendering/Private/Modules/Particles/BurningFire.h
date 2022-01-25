// 
#pragma once

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/VertexFormats.h>


namespace Rendering
{
struct BurningFireSystem: CStruct
{
	enum
	{
		MAX_BURNING_FIRES = 32,
	};

	//
	struct BurningFire
	{
		V3f			center_WS;
		float		radius_WS;

		float01_t	life01;	// 0..1, dead if > 1
		float		inv_lifespan_sec;
		float		anim_cycle;	// 0..1 - for sprite-sheet animation
		float		initial_radius;
	};
	ASSERT_SIZEOF(BurningFire, 32);

	TFixedArray< BurningFire, MAX_BURNING_FIRES >	burning_fires;


public:
	BurningFireSystem();

	void UpdateOncePerFrame(
		const NwTime& game_time
		);

	ERet Render(
		SpatialDatabaseI* spatial_database
		);

	void AddBurningFire(
		const V3f& pos_world
		, const float radius
		, const SecondsF lifespan
		);
};

}//namespace Rendering
