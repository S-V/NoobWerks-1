/*
=============================================================================
	File:	Utils.h
	Desc:	Math helpers.
=============================================================================
*/
#pragma once

#include <Base/Math/MiniMath.h>


//
//	EViewFrustumPlane
//
enum EViewFrustumPlane
{
	/// testing against the near plane culls almost the half of the scene
	VF_NEAR_PLANE = 0,

	/// checking against left and right clipping planes is important in 'horizontal' scenes (e.g. terrains)
	VF_LEFT_PLANE,
	VF_RIGHT_PLANE,

	VF_BOTTOM_PLANE,
	VF_TOP_PLANE,

	/// testing against the far clipping plane can be skipped in huge environments
	VF_FAR_PLANE,

	VF_NUM_PLANES,

	/// number of planes to use for culling
	//VF_CLIP_PLANES = VF_NUM_PLANES
	
	// Don't test against the far plane (e.g. in a space sim):
	VF_CLIP_PLANES = VF_FAR_PLANE
};
mxOPTIMIZE("use SSE for frustum culling")

//
//	ViewFrustum
//
struct ViewFrustum
{
	/// empty constructor leaves data uninitialized!
	ViewFrustum();

	/// Builds frustum planes from the given matrix.
	ViewFrustum( const M44f& mat );

	FASTBOOL			PointInFrustum( const V3f& point ) const;
	FASTBOOL			IntersectSphere( const Sphere16& sphere ) const;
	FASTBOOL			IntersectsAABB( const AABBf& aabb ) const;

	///
	SpatialRelation::Enum	Classify( const AABBf& aabb ) const;

	///
	///	Builds frustum planes from the given matrix.
	///
	///   If the given matrix is equal to the camera projection matrix
	/// then the resulting planes will be given in view space ( i.e., camera space ).
	///
	///   If the matrix is equal to the combined view and projection matrices,
	/// then the resulting planes will be given in world space.
	///
	///   If the matrix is equal to the combined world, view and projection matrices,
	/// then the resulting planes will be given in object space.
	///
	/// NOTE: Frustum planes' normals point towards the inside of the frustum.
	/// NOTE: Can only be used with the "normal" perspective projection! i.e. no reversed depth/Z.
	///
	void		extractFrustumPlanes_D3D( const M44f& mat );

	/// can be used with any perspective projection
	void extractFrustumPlanes_Generic(
		const V3f& camera_position
		, const V3f& camera_axis_right
		, const V3f& camera_axis_forward
		, const V3f& camera_axis_up
		, const float half_FoV_Y_in_radians
		, const float aspect_ratio
		, const float depth_near
		, const float depth_far
		);

	/// make sure not to skip the far plane when testing with this frustum
	void extractFrustumPlanes_InfiniteFarPlane(
		const V3f& camera_position
		, const V3f& camera_axis_right
		, const V3f& camera_axis_forward
		, const V3f& camera_axis_up
		, const float half_FoV_Y_in_radians
		, const float aspect_ratio
		);

public:
	enum Corners
	{
		nearTopLeft,
		nearTopRight,
		nearBottomRight,
		nearBottomLeft,

		farTopLeft,
		farTopRight,
		farBottomRight,
		farBottomLeft,

		Count
	};

	bool		GetFarLeftDown( V3f & point ) const;
	bool		GetFarLeftUp( V3f & point ) const;
	bool		GetFarRightUp( V3f & point ) const;
	bool		GetFarRightDown( V3f & point ) const;

	/// Computes positions of the 8 vertices of the frustum.
	/// NOTE: this isn't a cheap function to call.
	///
	bool		calculateCornerPoints( V3f corners_[Corners::Count] ) const;

	/// Returns 1 if all points are outside the frustum.
	FASTBOOL	CullPoints( const V3f* points, UINT numPoints ) const;

public_internal:
	V4f		planes[ VF_NUM_PLANES ];	//!< actual frustum planes (the normals point inwards).
	UINT	signs[ VF_NUM_PLANES ];		//!< signs of planes' normals
	// 120 bytes
};
