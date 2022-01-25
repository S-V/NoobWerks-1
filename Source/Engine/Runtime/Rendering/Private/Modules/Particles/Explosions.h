// 
#pragma once

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/VertexFormats.h>

namespace Rendering
{
struct RrExplosionsSystem: CStruct
{
	enum
	{
		MAX_EXPLOSIONS = 32,
	};

	//
	struct Explosion
	{
		V3f			center_WS;
		float01_t	life01;	// 0..1, dead if > 1

		float		inv_lifespan_sec;
		float		max_radius;
		float		rotation_angle_rad;
		float		rotation_speed;	// radians per second
	};

	TFixedArray< Explosion, MAX_EXPLOSIONS >	explosions;

public:
	RrExplosionsSystem();

	void UpdateOncePerFrame(
		const NwTime& game_time
		);

	ERet Render(
		SpatialDatabaseI* spatial_database
		);

	void AddExplosion(
		const V3f& pos_world
		, const float radius
		, const SecondsF lifespan
		, int rng_seed = 0
		);
};

float01_t CalcExplosionBrightness( float01_t explosion_life );

}//namespace Rendering
