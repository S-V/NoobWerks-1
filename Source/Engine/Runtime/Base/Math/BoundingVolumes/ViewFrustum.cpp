/*
=============================================================================
	File:	Utils.cpp
	Desc:	Math helpers.
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Math/BoundingVolumes/ViewFrustum.h>


/*================================
		ViewFrustum
================================*/

//tbPRINT_SIZE_OF(ViewFrustum);

ViewFrustum::ViewFrustum()
{}

ViewFrustum::ViewFrustum( const M44f& mat )
{
	this->extractFrustumPlanes_D3D( mat );
}

//
//	ViewFrustum::PointInFrustum
//
//	NOTE: This test assumes frustum planes face inward.
//
FASTBOOL ViewFrustum::PointInFrustum( const V3f& point ) const
{
#if 0 
	for( UINT iPlane = 0; iPlane < VF_NUM_PLANES; ++iPlane )
	{
		if( Plane_PointDistance( planes[ iPlane ], point ) < 0.0f )
		{
			return FALSE;
		}
	}
    return TRUE;
#else
	// assume the point is inside by default
	FASTBOOL	mask = 1;
	for( UINT iPlane = 0; (iPlane < VF_NUM_PLANES) && mask; ++iPlane )
	{
		mask &= ( Plane_PointDistance( planes[ iPlane ], point ) >= 0.0f );
	}
    return mask;
#endif
}

//
//	ViewFrustum::IntersectSphere
//
//	NOTE: This test assumes frustum planes face inward.
//
FASTBOOL ViewFrustum::IntersectSphere( const Sphere16& sphere ) const
{
	//DX_CYCLE_COUNTER( gStats.cull_sphere_cycles );
#if 0 
	for( UINT iPlane = 0; iPlane < VF_NUM_PLANES; ++iPlane )
	{
		if( Plane_PointDistance( planes[ iPlane ], sphere.center ) < -sphere.radius )
		{
			return FALSE;
		}
	}
    return TRUE;
#else
	// assume the sphere is inside by default
	FASTBOOL	mask = 1;
	for( UINT iPlane = 0; (iPlane < VF_NUM_PLANES) && mask; ++iPlane )
	{
		// Test whether the sphere is on the positive half space of each frustum plane.  
		// If it is on the negative half space of one plane we can reject it.
		mask &= ( Plane_PointDistance( planes[ iPlane ], sphere.center ) + sphere.radius > 0.0f );
	}
    return mask;
#endif
}

//
//	ViewFrustum::IntersectsAABB
//
//	NOTE: This test assumes frustum planes face inward.
//
FASTBOOL ViewFrustum::IntersectsAABB( const AABBf& aabb ) const
{
	//DX_CYCLE_COUNTER( gStats.cull_aabb_cycles );
	mxSTATIC_ASSERT( (int)SpatialRelation::Outside == 0 );
	return Classify(aabb);// != SpatialRelation::Outside;
}

//
//	ViewFrustum::Classify
//
//	See: http://www.flipcode.com/articles/article_frustumculling.shtml
//
//	NOTE: This test assumes frustum planes face inward.
//
SpatialRelation::Enum ViewFrustum::Classify( const AABBf& aabb ) const
{
	// See "Geometric Approach - Testing Boxes II":
	// https://cgvr.cs.uni-bremen.de/teaching/cg_literatur/lighthouse3d_view_frustum_culling/index.html

	//// fully inside the frustum by default
	SpatialRelation::Enum result = SpatialRelation::Inside;

	for( UINT iPlane = 0; iPlane < VF_CLIP_PLANES; ++iPlane )
	{
		const UINT nV = this->signs[ iPlane ];

		// pVertex is diagonally opposed to nVertex
		const CV3f nVertex(
			( nV & 1 ) ? aabb.min_corner.x : aabb.max_corner.x,
			( nV & 2 ) ? aabb.min_corner.y : aabb.max_corner.y,
			( nV & 4 ) ? aabb.min_corner.z : aabb.max_corner.z
			);
		const CV3f pVertex(
			( nV & 1 ) ? aabb.max_corner.x : aabb.min_corner.x,
			( nV & 2 ) ? aabb.max_corner.y : aabb.min_corner.y,
			( nV & 4 ) ? aabb.max_corner.z : aabb.min_corner.z
			);

		// Testing a single vertex is enough for the cases where the box is outside.

		// is the positive vertex outside?
		if ( Plane_PointDistance( this->planes[ iPlane ], pVertex ) < 0.0f ) {
			return SpatialRelation::Outside;
		}


		// The second vertex needs only to be tested if one requires distinguishing
		// between boxes totally inside and boxes partially inside the view this->

		// is the negative vertex outside?
		if ( Plane_PointDistance( this->planes[ iPlane ], nVertex ) <= 0.0f ) {
			result = SpatialRelation::Intersects;
		}
	}

	return result;
}

static inline void NormalizePlane( V4f & plane )
{
	V3f & N = *reinterpret_cast< V3f* >( &plane );
	const float invLength = 1.0f / V3_Length( N );
	N *= invLength;
	plane.w *= invLength;
}

//
//	ViewFrustum::extractFrustumPlanes_D3D - extracts from a matrix the planes that make up the frustum.
//
// 
// See: http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf

// Summary of method:

/*	If we take a vector v and a projection matrix M, the transformed point v' is = vM.  
A transformed point in clip space will be inside the frustum (AABB) if -w' < x' < w', -w' < y' < w' and 0 < z' < w'

We can deduce that it is on the positive side of the left plane's half space if x' > -w'
Since w' = v.col4 and x' = v.col1, we can rewrite this as v.col1 > -v.col4
thus v.col1 + v.col4 > 0   ... and ... v.(col1 + col4) > 0

This is just another way of writing the following: x( m14 + m11 ) + y( m24 + m21 ) + z( m34 + m31 ) + w( m44 + m41 )
For the untransformed point w = 1, so we can simplify to x( m14 + m11 ) + y( m24 + m21 ) + z( m34 + m31 ) + m44 + m41
Which is the same as the standard plane equation ax + by + cz + d = 0
Where a = m14 + m11, b = m24 + m21, c = m34 + m31 and d = m44 + m41

Running this algorithm on a projection matrix yields the clip planes in view space
Running it on a view projection matrix yields the clip planes in world space
.... for world view projection it yields the clip planes in object/local space
*/

// Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix [06/15/2001]
// http://www.cs.otago.ac.nz/postgrads/alexis/planeExtraction.pdf

mxBIBREF("Mathematics for 3D Game Programming and Computer Graphics [2004], 4.5.3 Extracting Frustum Planes, P.127")
mxBIBREF("Game Programming Gems 4 [2004], 2.2 Extracting Frustum and Camera Information, P.147")

// Three Methods to Extract Frustum Points (Don Williamson) [October 23, 2016]:
// http://donw.io/post/frustum-point-extraction/

// minimal frustum culling - 2010 (Inigo Quilez)
// https://www.iquilezles.org/www/articles/frustum/frustum.htm

void ViewFrustum::extractFrustumPlanes_D3D( const M44f& mat )
{
	V4f  col0 = { mat.v0[0], mat.v1[0], mat.v2[0], mat.v3[0] };
	V4f  col1 = { mat.v0[1], mat.v1[1], mat.v2[1], mat.v3[1] };
	V4f  col2 = { mat.v0[2], mat.v1[2], mat.v2[2], mat.v3[2] };
	V4f  col3 = { mat.v0[3], mat.v1[3], mat.v2[3], mat.v3[3] };

	// Planes' normals face inward.

	planes[ VF_LEFT_PLANE ]		= col3 + col0;	// -w' < x'		-> -(v.col4) < v.col -> 0 < (v.col4) + (v.col1) -> 0 < v.(col1 + col4)
	planes[ VF_RIGHT_PLANE ]	= col3 - col0;	// x' < w'		-> 0 < v.(col4 - col1)

	planes[ VF_BOTTOM_PLANE ]	= col3 + col1;	// y' < w'		-> 0 < v.(col4 - col2)
	planes[ VF_TOP_PLANE	]	= col3 - col1;	// -w' < y'		-> 0 < v.(col4 + col2)

	planes[ VF_NEAR_PLANE ]		= col2;			// 0 < z'		-> 0 < v.col3
	planes[ VF_FAR_PLANE ]		= col3 - col2;	// z' < w'		-> 0 < v.(col4 - col3)

	for( UINT iPlane = 0; iPlane < VF_NUM_PLANES; iPlane++ )
	{
		NormalizePlane( planes[ iPlane ] );
		signs[ iPlane ] = ( (planes[ iPlane ].x >= 0.f) ? 1 : 0 )
						| ( (planes[ iPlane ].y >= 0.f) ? 2 : 0 )
						| ( (planes[ iPlane ].z >= 0.f) ? 4 : 0 );
	}
}

void ViewFrustum::extractFrustumPlanes_Generic(
	const V3f& camera_position
	, const V3f& camera_axis_right
	, const V3f& camera_axis_forward
	, const V3f& camera_axis_up
	, const float half_FoV_Y_in_radians
	, const float aspect_ratio
	, const float depth_near
	, const float depth_far
	)
{
	// Based on http://donw.io/post/frustum-point-extraction/

	mxTODO("use doubles for intermediate calcs");

	// Near/far plane center points
	const V3f near_center	= camera_position + camera_axis_forward * depth_near;
	const V3f far_center	= camera_position + camera_axis_forward * depth_far;

	// Get projected viewport extents on near/far planes
	const float e = tanf( half_FoV_Y_in_radians );
	const float near_ext_y = e * depth_near;
	const float near_ext_x = near_ext_y * aspect_ratio;
	const float far_ext_y = e * depth_far;
	const float far_ext_x = far_ext_y * aspect_ratio;

	//
	V3f	frustum_corners[Corners::Count];

	// Points are just offset from the center points along camera basis

	frustum_corners[nearBottomLeft] =
		near_center - camera_axis_right * near_ext_x - camera_axis_up * near_ext_y
		;

	frustum_corners[nearTopLeft] =
		near_center - camera_axis_right * near_ext_x + camera_axis_up * near_ext_y
		;

	frustum_corners[nearTopRight] =
		near_center + camera_axis_right * near_ext_x + camera_axis_up * near_ext_y
		;

	frustum_corners[nearBottomRight] =
		near_center + camera_axis_right * near_ext_x - camera_axis_up * near_ext_y
		;

	//

#if 0	// not used
	frustum_corners[farBottomLeft] =
		far_center  - camera_axis_right * far_ext_x  - camera_axis_up * far_ext_y
		;

	frustum_corners[farTopLeft] =
		far_center  - camera_axis_right * far_ext_x  + camera_axis_up * far_ext_y
		;

	frustum_corners[farTopRight] =
		far_center  + camera_axis_right * far_ext_x  + camera_axis_up * far_ext_y
		;

	frustum_corners[farBottomRight] =
		far_center  + camera_axis_right * far_ext_x  - camera_axis_up * far_ext_y
		;
#endif
	//

	planes[ VF_LEFT_PLANE ] = Plane_FromThreePoints(
		camera_position, frustum_corners[nearBottomLeft], frustum_corners[nearTopLeft]
	);

	planes[ VF_RIGHT_PLANE ] = Plane_FromThreePoints(
		camera_position, frustum_corners[nearTopRight], frustum_corners[nearBottomRight]
	);

	planes[ VF_BOTTOM_PLANE ] = Plane_FromThreePoints(
		camera_position, frustum_corners[nearBottomRight], frustum_corners[nearBottomLeft]
	);

	planes[ VF_TOP_PLANE ] = Plane_FromThreePoints(
		camera_position, frustum_corners[nearTopLeft], frustum_corners[nearTopRight]
	);

	planes[ VF_NEAR_PLANE ] = Plane_FromPointNormal(
		near_center, camera_axis_forward
	);

	planes[ VF_FAR_PLANE ] = Plane_FromPointNormal(
		far_center, -camera_axis_forward
	);

	//
	for( UINT iPlane = 0; iPlane < VF_NUM_PLANES; iPlane++ )
	{
		NormalizePlane( planes[ iPlane ] );
		signs[ iPlane ] = ( (planes[ iPlane ].x >= 0.f) ? 1 : 0 )
						| ( (planes[ iPlane ].y >= 0.f) ? 2 : 0 )
						| ( (planes[ iPlane ].z >= 0.f) ? 4 : 0 );
	}
}

void ViewFrustum::extractFrustumPlanes_InfiniteFarPlane(
	const V3f& camera_position
	, const V3f& camera_axis_right
	, const V3f& camera_axis_forward
	, const V3f& camera_axis_up
	, const float half_FoV_Y_in_radians
	, const float aspect_ratio
	)
{
	this->extractFrustumPlanes_Generic(
		camera_position
		, camera_axis_right
		, camera_axis_forward
		, camera_axis_up
		, half_FoV_Y_in_radians
		, aspect_ratio
		, 1.0f
		, BIG_NUMBER
		);
}

mxBIBREF("Mathematics for 3D Game Programming and Computer Graphics [2004], 4.2.2 Intersection of Three Planes, P.107")
bool PlanesIntersection(
						const V4f& plane_a,
						const V4f& plane_b,
						const V4f& plane_c,
						V3f &intersection_point_
						)
{
	const M33f A = {
		plane_a.x, plane_a.y, plane_a.z,
		plane_b.x, plane_b.y, plane_b.z,
		plane_c.x, plane_c.y, plane_c.z,
	};
	const V3f B = {
		-plane_a.w,
		-plane_b.w,
		-plane_c.w,
	};

	//const M33f A_T = M33_Transpose( A );

	M33f A_inv = A;
	if( !M33_inverse( A_inv ) ) {
		return false;
	}

	//
	intersection_point_ = M33_Transform( A_inv, B );

	return true;
}


//
//	ViewFrustum::calculateCornerPoints - Computes positions of the 8 vertices of the frustum.
//
bool ViewFrustum::calculateCornerPoints( V3f corners[Corners::Count] ) const
{
	bool	status = true;

	status &= PlanesIntersection(
		planes[VF_NEAR_PLANE],
		planes[VF_TOP_PLANE],
		planes[VF_LEFT_PLANE],
		corners[Corners::nearTopLeft]
	);

	status &= PlanesIntersection(
		planes[VF_NEAR_PLANE],
		planes[VF_TOP_PLANE],
		planes[VF_RIGHT_PLANE],
		corners[Corners::nearTopRight]
	);

	status &= PlanesIntersection(
		planes[VF_NEAR_PLANE],
		planes[VF_BOTTOM_PLANE],
		planes[VF_RIGHT_PLANE],
		corners[Corners::nearBottomRight]
	);
	
	status &= PlanesIntersection(
		planes[VF_NEAR_PLANE],
		planes[VF_BOTTOM_PLANE],
		planes[VF_LEFT_PLANE],
		corners[Corners::nearBottomLeft]
	);

	//

	status &= PlanesIntersection(
		planes[VF_FAR_PLANE],
		planes[VF_TOP_PLANE],
		planes[VF_LEFT_PLANE],
		corners[Corners::farTopLeft]
	);

	status &= PlanesIntersection(
		planes[VF_FAR_PLANE],
		planes[VF_TOP_PLANE],
		planes[VF_RIGHT_PLANE],
		corners[Corners::farTopRight]
	);

	status &= PlanesIntersection(
		planes[VF_FAR_PLANE],
		planes[VF_BOTTOM_PLANE],
		planes[VF_RIGHT_PLANE],
		corners[Corners::farBottomRight]
	);

	status &= PlanesIntersection(
		planes[VF_FAR_PLANE],
		planes[VF_BOTTOM_PLANE],
		planes[VF_LEFT_PLANE],
		corners[Corners::farBottomLeft]
	);

	mxASSERT(status);
	return status;
}


#if 0
bool ViewFrustum::GetFarLeftDown( V3f & point ) const
{
	return PlanesIntersection(
		planes[ VF_FAR_PLANE ],
		planes[ VF_LEFT_PLANE ],
		planes[ VF_BOTTOM_PLANE ],
		point
	);
}

bool ViewFrustum::GetFarLeftUp( V3f & point ) const
{
	return PlanesIntersection(
		planes[ VF_FAR_PLANE ],
		planes[ VF_LEFT_PLANE ],
		planes[ VF_TOP_PLANE ],
		point
	);
}

bool ViewFrustum::GetFarRightUp( V3f & point ) const
{
	return PlanesIntersection(
		planes[ VF_FAR_PLANE ],
		planes[ VF_RIGHT_PLANE ],
		planes[ VF_TOP_PLANE ],
		point
	);
}

bool ViewFrustum::GetFarRightDown( V3f & point ) const
{
	return PlanesIntersection(
		planes[ VF_FAR_PLANE ],
		planes[ VF_RIGHT_PLANE ],
		planes[ VF_BOTTOM_PLANE ],
		point
	);
}

//
//	ViewFrustum::CullPoints - Returns 1 if all points are outside the frustum.
//
FASTBOOL ViewFrustum::CullPoints( const V3f* points, UINT numPoints ) const
{
	mxOPTIMIZE(:);
	for( UINT iPlane = 0; iPlane < VF_CLIP_PLANES; iPlane++ )
	{
		FASTBOOL bAllOut = 1;

		for( UINT iPoint = 0; iPoint < numPoints; iPoint++ )  
		{
			if( planes[ iPlane ].Distance( points[iPoint] ) > 0.0f )
			{
				bAllOut = 0;
				break;
			}
		}

		if( bAllOut ) {
			return TRUE;
		}
	}
	return FALSE;
}
#endif
