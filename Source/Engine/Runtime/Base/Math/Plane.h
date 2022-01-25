#pragma once


/*
===============================================================================

	3D plane with equation: a * x + b * y + c * z + d = 0

===============================================================================
*/
template< typename Scalar >
struct TPlane
{
	typedef Tuple3< Scalar >	V3t;
	typedef CTuple3< Scalar >	CV3t;
	typedef Tuple4< Scalar >	V4t;
	typedef CTuple4< Scalar >	CV4t;


	V4t		abcd;

public:
	static const TPlane createFromPointAndNormal(
		const V3t& point, const V3t& normal
		)
	{
		mxASSERT(normal.isNormalized());
		TPlane plane = {
			normal.x, normal.y, normal.z,
			-V3t::dot( normal, point )
		};
		return plane;
	}

public:
	mxFORCEINLINE Scalar distanceToPoint(
		const V3t& point
		) const
	{
		return abcd.x * point.x + abcd.y * point.y
			+ (abcd.z * point.z + abcd.w)
			;
	}

	mxFORCEINLINE const V3t& normal() const
	{
		return *(V3t*) &abcd;
	}
};

typedef TPlane< float >		PlaneF;
typedef TPlane< double >	PlaneD;


//=====================================================================
//	PLANE OPERATIONS
//	3D plane with equation: a * x + b * y + c * z + d = 0,
//	where d is the negative distance from the origin.
//=====================================================================
const V4f 	Plane_FromThreePoints( const V3f& a, const V3f& b, const V3f& c );
const V4f 	Plane_FromPointNormal( const V3f& point, const V3f& normal );
const V3f 	Plane_CalculateNormal( const V4f& plane );
const V3f&	Plane_GetNormal( const V4f& plane );
void		Plane_Normalize( V4f & plane );
const float Plane_PointDistance( const V4f& plane, const V3f& point );
const bool 	Plane_ContainsPoint( const V4f& plane, const V3f& point, float epsilon );
const V4f 	Plane_Translate( const V4f& plane, const V3f& translation );
//const V4f 	Plane_Transform( const V4f& plane, const M44f& transform );
const bool	Plane_RayIntersection( const V4f& plane, const V3f& start, const V3f& dir, float &_fraction );
const bool	Plane_LineIntersection( const V4f& plane, const V3f& start, const V3f& end, float &_fraction );

const Vector4	Plane_DotCoord( Vec4Arg0 plane, Vec4Arg0 xyz );

/// spatial relation to a plane
enum EPlaneSide
{
	PLANESIDE_ON = 0,
	PLANESIDE_BACK,
	PLANESIDE_FRONT,
	PLANESIDE_CROSS,
};

enum EPlaneType
{
	PLANETYPE_X		= 0,
	PLANETYPE_Y,
	PLANETYPE_Z,
	PLANETYPE_NEGX,
	PLANETYPE_NEGY,
	PLANETYPE_NEGZ,
	PLANETYPE_TRUEAXIAL,	// all types < 6 are true axial planes
	PLANETYPE_ZEROX,
	PLANETYPE_ZEROY,
	PLANETYPE_ZEROZ,
	PLANETYPE_NONAXIAL,
};

EPlaneType CalculatePlaneType( const V4f& plane );
