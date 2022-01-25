// Vectorized Axis Aligned Bounding Box for use with SIMD.
#pragma once

#include <Base/Math/MiniMath.h>
#include <Base/Math/BoundingVolumes/AABB.h>


struct SimdAabb
{
	Vector4	mins;
	Vector4	maxs;
};

mxFORCEINLINE Vector4 SimdAabb_GetCenter(const SimdAabb& aabb)
{
	return V4_MUL(
		V4_ADD(aabb.mins, aabb.maxs)
		, V4_REPLICATE(0.5f)
		);
}

mxFORCEINLINE Vector4 SimdAabb_GetSize(const SimdAabb& aabb)
{
	return V4_SUB(aabb.maxs, aabb.mins);
}

mxFORCEINLINE void SimdAabb_ExpandToIncludeAABB(SimdAabb *aabb, const SimdAabb& other)
{
	aabb->mins = V4_MIN( aabb->mins, other.mins );
	aabb->maxs = V4_MAX( aabb->maxs, other.maxs );
}

mxFORCEINLINE void SimdAabb_ExpandToIncludePoint(SimdAabb *aabb, const Vector4& point)
{
	aabb->mins = V4_MIN( aabb->mins, point );
	aabb->maxs = V4_MAX( aabb->maxs, point );
}

/// doesn't include touching
mmINLINE bool SimdAabb_IntersectAABBs( const SimdAabb& aabb0, const SimdAabb& aabb1)
{
	//return this->min_corner.x <= other.max_corner.x && this->min_corner.y <= other.max_corner.y && this->min_corner.z <= other.max_corner.z
	//	&& this->max_corner.x >= other.min_corner.x && this->max_corner.y >= other.min_corner.y && this->max_corner.z >= other.min_corner.z
	//	;
	const Vector4 a = V4_CMPGE(aabb0.mins, aabb1.maxs);
	const Vector4 b = V4_CMPGE(aabb1.mins, aabb0.maxs);
	return V4_A_AND_B_ARE_ZEROS(
		_mm_or_ps(a, b),
		V4_LOAD_PS_HEX(0, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU)
		) != 0;
}

mxFORCEINLINE bool SimdAabb_ContainsPoint(const SimdAabb& aabb, const Vector4& point)
{
	return V4_ALLZERO_PS(_mm_or_ps(
		_mm_cmplt_ps(point, aabb.mins),
		_mm_cmpgt_ps(point, aabb.maxs)
		)) != 0;
}




mxFORCEINLINE void SimdAabb_CreateFromSphere(
	SimdAabb *aabb_
	, const Vector4& center, float radius
	)
{
	const Vector4 vRadius = V4_REPLICATE(radius);

	aabb_->mins = V4_SUB( center, vRadius );
	aabb_->maxs = V4_ADD( center, vRadius );
}


mxFORCEINLINE const AABBf SimdAabb_Get_AABBf(
	const SimdAabb &aabb
	)
{
	const AABBf aabb_f = {
		Vector4_As_V3(aabb.mins),
		Vector4_As_V3(aabb.maxs),
	};
	return aabb_f;
}
