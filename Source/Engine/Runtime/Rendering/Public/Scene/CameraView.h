//
#pragma once

#include <Base/Math/BoundingVolumes/ViewFrustum.h>
#include <Core/VectorMath.h>


namespace Rendering
{




/*
===============================================================================
	LARGE WORLDS SUPPORT

	Right now we use doubles for positioning objects within the Star system.
	World units are in meters.

	In the future it can be extended to nested coordinate systems for galaxies.
===============================================================================
*/

#if 1

	typedef V3d	NwLargePosition;

#else

	struct NwLargePosition
	{
		/// position within the Star system
		V3d		xyz;	// 24

		//UShort4	sector;	// 8

	public:
		NwLargePosition()
		{
			reset();
		}

		void reset()
		{
			xyz = V3d::zero();
			//sector.x = sector.y = sector.z = sector.w = 0;
		}
	};
	//mxSTATIC_ASSERT(sizeof(NwLargePosition) == 32);

#endif







/// When the camera moves a certain distance away from the floating origin,
/// the floating origin is shifted to the camera's position.
/// This keeps the visible world centered around the camera,
/// allowing for high floating point precision.
/// NOTE: should be lightweight to copy
struct NwFloatingOrigin
{
	/// The position of the floating origin in world space.
	/// The scene is periodically re-centered around this point.
	V3d				center_in_world_space;

	// padding so that the following variable is 16-byte aligned
	double			pad8;

	/// The position of the camera relative to the floating origin.
	/// The scene is always rendered relative to this position.
	V3f				relative_eye_position;

public:
	NwFloatingOrigin()
	{
		center_in_world_space = V3d::zero();
		relative_eye_position = V3f::zero();
	}

	/// Returns the position relative to the floating origin.
	mxFORCEINLINE const V3d getRelativePosition_in_DoublePrecision(
		const V3d& position_in_world_space
		) const
	{
		return position_in_world_space - this->center_in_world_space;
	}

	mxFORCEINLINE const V3f getRelativePosition( const V3d& position_in_world_space ) const
	{
		return V3f::fromXYZ( position_in_world_space - this->center_in_world_space );
	}

	mxFORCEINLINE const AABBf getRelativeAABB( const AABBd& aabb_in_world_space ) const
	{
		const AABBd aabb_relative_to_floating_origin = {
			aabb_in_world_space.min_corner - this->center_in_world_space,
			aabb_in_world_space.max_corner - this->center_in_world_space,
		};
		return AABBf::fromOther( aabb_relative_to_floating_origin );
	}

static NwFloatingOrigin GetDummy_TEMP_TODO_REMOVE() {
	return NwFloatingOrigin();
}
};







/// contains camera parameters for scene rendering and precomputed matrices for passing to shaders
struct NwCameraView: NwView3D
{
	///
	NwLargePosition	eye_pos_in_world_space;

	// derived data:

	/// near clipping plane equation - used for computing approximate view-space depth
	/// (for rough depth sorting)
	V4f		near_clipping_plane;

	// the most commonly-used matrices should placed at the beginning
	M44f	view_projection_matrix;
	M44f	view_matrix;	// 3x4
	M44f	inverse_view_projection_matrix;
	M44f	inverse_projection_matrix;
	M44f	inverse_view_matrix;	// 3x4 World matrix of the camera (inverse of the view matrix)
	M44f	projection_matrix;

public:
	void recomputeDerivedMatrices();

	bool projectViewSpaceToScreenSpace(
		const V3f& position_in_view_space
		, V2f *position_in_screen_space_
		) const;

	const ViewFrustum BuildViewFrustum() const;
};

}//namespace Rendering
