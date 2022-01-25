//
#pragma once

#include "game_forward_declarations.h"


namespace MyGameUtils
{
	ERet Explode_Rocket(
		const V3f& explosion_center
		, const V3f surface_normal
		, const int rng_seed = 0
		);
}//namespace MyGameUtils
