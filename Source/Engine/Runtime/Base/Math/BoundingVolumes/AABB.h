// Axis Aligned Bounding Box.
#pragma once

#include <Base/Math/MiniMath.h>
#include <Base/Math/BoundingVolumes/Sphere.h>



/// Axis Aligned Bounding Box represented by two extreme (corner) points
/// (the box's minimum and maximum extents).
/// May be empty or nonempty. For nonempty bounds, minimum <= maximum has to hold for all axes.
/// empty bounds should be represented as minimum = MaxBoundsExtents and maximum = -MaxBoundsExtents for all axes.
/// All other representations are invalid and the behavior is undefined.
template< typename Scalar >
struct AabbMM
{
	typedef Tuple3< Scalar >	V3t;
	typedef CTuple3< Scalar >	CV3t;

	V3t	min_corner;	//!< the minimum corner of the box
	V3t	max_corner;	//!< the maximum corner of the box

public:
	static mxFORCEINLINE const Scalar MaxBoundsExtents()
	{
		return std::numeric_limits<Scalar>::max() * Scalar(0.25);
	}

	/// makes an inside-out bounds
	void clear()
	{
		this->min_corner = CV3t( +MaxBoundsExtents() );
		this->max_corner = CV3t( -MaxBoundsExtents() );
	}

	static const AabbMM GetVeryLarge()
	{
		const AabbMM result = {
			CV3t( +MaxBoundsExtents() ),
			CV3t( -MaxBoundsExtents() )
		};
		return result;
	}

	static const AabbMM make( const V3t& mins, const V3t& maxs )
	{
		const AabbMM result = { mins, maxs };
		return result;
	}
	static const AabbMM fromSphere( const V3t& center, const Scalar radius )
	{
		const CV3t halfSize(radius);
		const AabbMM result = {
			center - halfSize,
			center + halfSize
		};
		return result;
	}

	static const AabbMM makeUnsignedUnitCube()
	{
		return make(CV3t(0), CV3t(1));
	}

	/// [-1..+1] has the best floating-point precision
	static const AabbMM makeSignedUnitCube()
	{
		return make(CV3t(-1), CV3t(+1));
	}

	/// is used for converting double-precision AABBs to single-precision ones
	template< typename Y >
	static const AabbMM fromOther( const AabbMM<Y>& other )
	{
		return make( V3t::fromXYZ(other.min_corner), V3t::fromXYZ(other.max_corner) );
	}

	static const AabbMM scaledAndShifted( const AabbMM& aabb, const V3t& centerPoint, const Scalar scaleFactor )
	{
		const AabbMM result = {
			( aabb.min_corner - centerPoint ) * scaleFactor + centerPoint,
			( aabb.max_corner - centerPoint ) * scaleFactor + centerPoint
		};
		return result;
	}

	/// expands the volume to include the given point
	mmINLINE void addPoint( const V3t& new_point )
	{
		this->min_corner = V3t::min( this->min_corner, new_point );
		this->max_corner = V3t::max( this->max_corner, new_point );
	}

	mmINLINE void ExpandToIncludeAABB( const AabbMM& other )
	{
		this->min_corner = V3t::min( this->min_corner, other.min_corner );
		this->max_corner = V3t::max( this->max_corner, other.max_corner );
	}

	/// Tests if this AABB is degenerate.
	mmINLINE bool IsValid() const
	{
		return V3t::lessThan( this->min_corner, this->max_corner );
	}

	/// Returns the center point of this AABB.
	mmINLINE const V3t center() const
	{
		return (this->max_corner + this->min_corner) * Scalar(0.5);
	}

	/// Returns the side lengths of this AABB in x, y and z directions.
	/// The returned vector is equal to the diagonal vector of this AABB,
	/// i.e. it spans from the minimum corner of the AABB to the maximum corner of the AABB.
	mmINLINE const V3t size() const
	{
		return this->max_corner - this->min_corner;
	}
	mmINLINE const V3t halfSize() const
	{
		return this->size() * Scalar(0.5);
	}

	mmINLINE EAxis LongestAxis() const
	{
		const V3f aabb_size = this->size();
		return (EAxis) Max3Index( aabb_size.x, aabb_size.y, aabb_size.z );
	}

	mmINLINE float CenterAlongAxis(const EAxis axis) const
	{
		return (max_corner.a[ axis ] + min_corner.a[ axis ]) * Scalar(0.5);
	}

	mmINLINE Scalar surfaceArea() const
	{
		const V3t size = this->size();
		return ( size.x * size.z + size.x * size.y + size.y * size.z ) * Scalar(2);
	}
	mmINLINE Scalar volume() const
	{
		const V3t size = this->size();
		return size.x * size.y * size.z;
	}

	mmINLINE const AabbMM scaled( const Scalar scale ) const
	{
		return this->expanded( this->size() * (scale - Scalar(1)) );
	}
	/// returns translated bounds
	mmINLINE const AabbMM shifted( const V3t& offset ) const
	{
		const AabbMM result = { this->min_corner + offset, this->max_corner + offset };
		return result;
	}
	/// return bounds expanded in all directions
	mmINLINE const AabbMM expanded( const V3t& margin ) const
	{
		const AabbMM result = { this->min_corner - margin, this->max_corner + margin };
		return result;
	}
	mmINLINE const AabbMM expanded( const V3t& min_margin, const V3t& max_margin ) const
	{
		const AabbMM result = { this->min_corner - min_margin, this->max_corner + max_margin };
		return result;
	}
	/// Returns the eight corners of this box. Uses Morton/Z-order numbering for corner vertices.
	static void getCorners( const V3t& mins, const V3t& maxs, V3t points[8] )
	{
		for( UINT i = 0; i < 8; i++ ) {
			points[i].x = (i & BIT(AXIS_X)) ? maxs.x : mins.x;
			points[i].y = (i & BIT(AXIS_Y)) ? maxs.y : mins.y;
			points[i].z = (i & BIT(AXIS_Z)) ? maxs.z : mins.z;
		}
	}
	void getCorners( V3t points[8] ) const
	{
		getCorners( this->min_corner, this->max_corner, points );
	}

	/// Given a point P, return the point Q on or in AABB b that is closest to P.
	mmINLINE const V3t closestPoint( const V3t& point ) const
	{
		mxBIBREF("Real-Time Collision Detection (Christer Ericson)[2005], 5.1.3 Closest Point on AABB to Point, P.130");
		// For each coordinate axis, if the point coordinate value
		// is outside box, clamp it to the box, else keep it as is.
		return V3t::clamped( point, this->min_corner, this->max_corner );
	}
	/// Computes the squared Euclidean distance between the given point and this AABB.
	mmINLINE Scalar closestDistancePointSquared( const V3t& point ) const
	{
		mxBIBREF("Real-Time Collision Detection (Christer Ericson)[2005], 5.1.3.1 Distance of Point to AABB, P.131");
		return V3t::lengthSquared( point - this->closestPoint( point ) );
	}

	mxBIBREF("Graphics Gems I [1990], 'Transforming Axis-Aligned Bounding Boxes' (James Arvo), P.548");
	mxBIBREF("Real-Time Collision Detection (Christer Ericson)[2005], 4.2.6 AABB Recomputed from Rotated AABB, P.86");
	/// angle-preserving transform
	const AabbMM transformed( const M44f& transform ) const
	{
		// We don't need to transform all eight corners of the original AABB to find the new min and max.
		const V3f& axisX = V4_As_V3( transform.v0 );
		const V3f& axisY = V4_As_V3( transform.v1 );
		const V3f& axisZ = V4_As_V3( transform.v2 );

		// The transformed AABB is composed of only the (transformed) components of the original 'min_corner' and 'max_corner'.
		const V3f minX = axisX * this->min_corner.x;
		const V3f maxX = axisX * this->max_corner.x;

		const V3f minY = axisY * this->min_corner.y;
		const V3f maxY = axisY * this->max_corner.y;

		const V3f minZ = axisZ * this->min_corner.z;
		const V3f maxZ = axisZ * this->max_corner.z;

		AabbMM result;
		result.min_corner = V3f::min( minX, maxX ) + V3f::min( minY, maxY ) + V3f::min( minZ, maxZ );
		result.max_corner = V3f::max( minX, maxX ) + V3f::max( minY, maxY ) + V3f::max( minZ, maxZ );
		result.min_corner += M44_getTranslation( transform );
		result.max_corner += M44_getTranslation( transform );
		return result;
	}

	/// includes touching
	mmINLINE bool containsPoint( const V3t& point ) const
	{
		return V3t::greaterOrEqualTo( point, this->min_corner ) && V3t::lessOrEqualTo( point, this->max_corner );
	}
	/// includes touching
	mmINLINE bool contains( const AabbMM& other ) const
	{
		return other.min_corner.x >= this->min_corner.x && other.min_corner.y >= this->min_corner.y && other.min_corner.z >= this->min_corner.z
			&& other.max_corner.x <= this->max_corner.x && other.max_corner.y <= this->max_corner.y && other.max_corner.z <= this->max_corner.z;
	}
	mmINLINE bool contains_NoTouch( const AabbMM& other ) const
	{
		return other.min_corner.x > this->min_corner.x && other.min_corner.y > this->min_corner.y && other.min_corner.z > this->min_corner.z
			&& other.max_corner.x < this->max_corner.x && other.max_corner.y < this->max_corner.y && other.max_corner.z < this->max_corner.z;
	}
	/// includes touching
	mmINLINE bool intersects( const AabbMM& other ) const
	{
		return this->min_corner.x <= other.max_corner.x && this->min_corner.y <= other.max_corner.y && this->min_corner.z <= other.max_corner.z
			&& this->max_corner.x >= other.min_corner.x && this->max_corner.y >= other.min_corner.y && this->max_corner.z >= other.min_corner.z;
	}

	/// exact compare, no epsilon
	mmINLINE bool equals( const AabbMM& other ) const
	{
		return this->min_corner == other.min_corner && this->max_corner == other.max_corner;
	}
	mmINLINE bool equals( const AabbMM& other, const Scalar _epsilon ) const
	{
		return this->min_corner.equals( other.min_corner, _epsilon ) && this->max_corner.equals( other.max_corner, _epsilon );
	}
	/// returns the union of the two bounding boxes
	mmINLINE static const AabbMM getUnion( const AabbMM& a, const AabbMM& b )
	{
		AabbMM	result;
		result.min_corner = V3t::min( a.min_corner, b.min_corner );
		result.max_corner = V3t::max( a.max_corner, b.max_corner );
		return result;
	}
	/// returns the intersection of the given two bounding boxes
	mmINLINE static const AabbMM getIntersection( const AabbMM& a, const AabbMM& b )
	{
		AabbMM	result;
		result.min_corner = V3t::max( a.min_corner, b.min_corner );
		result.max_corner = V3t::min( a.max_corner, b.max_corner );
		return result;
	}
	mmINLINE static const AabbMM getMinkowskiDifference( const AabbMM& a, const AabbMM& b )
	{
		AabbMM	result;
		result.min_corner = a.min_corner - b.max_corner;
		result.max_corner = a.min_corner + ( a.size() + b.size() );
		return result;
	}
};

/// AABB represented by a center point and half-size (aka half-diagonal, radius or half-width extents -
/// - the positive vector from the center of the box to the vertex with the three largest components).
template< typename Scalar >
struct AabbCE
{
	typedef Tuple3< Scalar > V3t;

	V3t	center;	//!< the center of the box
	V3t	extent;	//!< the half-size of the box

public:
	mmINLINE const V3t minCorner() const
	{
		return center - extent;
	}

	mmINLINE const V3t maxCorner() const
	{
		return center + extent;
	}

	mmINLINE const AabbMM< Scalar > ToAabbMinMax() const
	{
		return AabbMM< Scalar >::make(
			minCorner(), maxCorner()
			);
	}
};


/*
==========================================================
	TYPEDEFS
==========================================================
*/

typedef AabbMM< F32 >	AABBf;	ASSERT_SIZEOF( AABBf, 24 );
typedef AabbMM< F64 >	AABBd;	ASSERT_SIZEOF( AABBd, 48 );
typedef AabbMM< I32 >	AABBi;	ASSERT_SIZEOF( AABBi, 24 );
typedef AabbMM< U32 >	AABBu;	ASSERT_SIZEOF( AABBu, 24 );

typedef AabbCE< F32 >	AabbCEf;	ASSERT_SIZEOF( AabbCEf, 24 );
typedef AabbCE< F64 >	AabbCEd;	ASSERT_SIZEOF( AabbCEd, 48 );
typedef AabbCE< I32 >	AabbCEi;	ASSERT_SIZEOF( AabbCEi, 24 );
typedef AabbCE< U32 >	AabbCEu;	ASSERT_SIZEOF( AabbCEu, 24 );


void Sphere16_From_AABB( const AABBf& aabb, Sphere16 *sphere );
const AABBf AABBf_From_Sphere( const Sphere16& _sphere );


static inline
bool AABBi_Contains( const AABBi& bounds, const Int3& cellCoords )
{
	//NB: min - inclusive, max - exclusive
	return Int3::lessOrEqualTo( bounds.min_corner, cellCoords ) && Int3::lessThan( cellCoords, bounds.max_corner );
}




/// This function is used for LoD calculations.
template< typename Scalar >
inline Scalar ChebyshevDistance(
							   const AabbMM< Scalar >& bounds,
							   const Tuple3< Scalar >& eyePos
							   )
{
	// Get the closest distance between the camera and the AABB under 'max' norm:
	const Scalar minX = Min( fabs( eyePos.x - bounds.min_corner.x ), fabs( eyePos.x - bounds.max_corner.x ) );
	const Scalar minY = Min( fabs( eyePos.y - bounds.min_corner.y ), fabs( eyePos.y - bounds.max_corner.y ) );
	const Scalar minZ = Min( fabs( eyePos.z - bounds.min_corner.z ), fabs( eyePos.z - bounds.max_corner.z ) );
	const Scalar dist = Max3( minX, minY, minZ );
	return dist;
}

/// Computes Chebyshev distance between the two integer AABBs.
inline
int Calc_Chebyshev_Dist( const AABBi& a, const AABBi& b )
{
	Int3 dists( 0 );	// always non-negative
	for( int axis = 0; axis < NUM_AXES; axis++ )
	{
		if( a.min_corner[ axis ] > b.max_corner[ axis ] ) {
			dists[ axis ] = a.min_corner[ axis ] - b.max_corner[ axis ];
		} else if( a.max_corner[ axis ] < b.min_corner[ axis ] ) {
			dists[ axis ] = b.min_corner[ axis ] - a.max_corner[ axis ];
		} else {
			dists[ axis ] = 0;
		}
	}
	return Max3( dists.x, dists.y, dists.z );
}




// legacy code
void AABBf_Clear( AABBf *aabb );
const AABBf AABBf_Make( const V3f& mins, const V3f& maxs );
void AABBf_AddPoint( AABBf *aabb, const V3f& p );
void AABBf_AddAABB( AABBf *aabb, const AABBf& other );
V3f AABBf_Center( const AABBf& aabb );
V3f AABBf_Extent( const AABBf& aabb );	// returns half size
V3f AABBf_FullSize( const AABBf& aabb );
int AABBf_LongestAxis( const AABBf& aabb );
bool AABBf_Intersect( const AABBf& a, const AABBf& b );
void AABBf_Merge( const AABBf& a, const AABBf& b, AABBf &_result );
const AABBf AABBf_Merge( const AABBf& a, const AABBf& b );
const AABBf AABBf_Shift( const AABBf& aabb, const V3f& offset );
const AABBf AABBf_Scaled( const AABBf& aabb, const float scale );
const AABBf AABBf_GrowBy( const AABBf& aabb, const float epsilon );
bool AABBf_ContainsPoint( const AABBf& aabb, const V3f& point );// includes touching
bool AABBf_ContainsPoint( const AABBf& aabb, const V3f& point, float epsilon );// includes touching
V3f AABBf_GetClampedPoint( const AABBf& aabb, const V3f& _p );
const V3f AABBf_GetNormalizedPosition( const AABBf& aabb, const V3f& positionInAabb );
const V3f AABBf_GetOriginalPosition( const AABBf& aabb, const V3f& normalizedPosition );
mxOPTIMIZE("Template quantization so that it works with shorts, ints, etc");
void AABBf_EncodePosition( const AABBf& aabb, const V3f& point, UINT8 output[3] );
void AABBf_DecodePosition( const AABBf& aabb, const UINT8 input[3], V3f *point );
const V3f AABBf_GetNormal( const AABBf& aabb, const V3f& point );
float AABBf_SurfaceArea( const AABBf& aabb );
bool AABBf_Equal( const AABBf& a, const AABBf& b, float epsilon );

/// find the minimum axis aligned bounding box containing a set of points.
const AABBf AABBf_From_Points_Aligned( UINT _count, const V3f* points, UINT _stride );
const AABBf AABBf_From_Points_Unaligned( UINT _count, const V3f* points, UINT _stride );

M44f M44_getTransform( const AABBf& from, const AABBf& to );



/*
==========================================================
	INLINE IMPLEMENTATIONS
==========================================================
*/
mmINLINE const AABBf AABBf_From_Sphere( const Sphere16& _sphere )
{
	const V3f extent = V3_SetAll(_sphere.radius);
	const AABBf result = {
		_sphere.center - extent,
		_sphere.center + extent
	};
	return result;
}

mmINLINE void Sphere16_From_AABB( const AABBf& aabb, Sphere16 *sphere )
{
	sphere->center = AABBf_Center(aabb);
	sphere->radius = V3_Length(AABBf_Extent(aabb));
}

mmINLINE void AABBf_Clear( AABBf *aabb )
{
	aabb->min_corner = V3_Set( +BIG_NUMBER, +BIG_NUMBER, +BIG_NUMBER );
	aabb->max_corner = V3_Set( -BIG_NUMBER, -BIG_NUMBER, -BIG_NUMBER );
}
mmINLINE const AABBf AABBf_Make( const V3f& mins, const V3f& maxs )
{
	const AABBf result = { mins, maxs };
	return result;
}
mmINLINE void AABBf_AddPoint( AABBf *aabb, const V3f& p )
{
	aabb->min_corner = V3_Mins( aabb->min_corner, p );
	aabb->max_corner = V3_Maxs( aabb->max_corner, p );
}
mmINLINE void AABBf_AddAABB( AABBf *aabb, const AABBf& other )
{
	aabb->min_corner = V3_Mins( aabb->min_corner, other.min_corner );
	aabb->max_corner = V3_Maxs( aabb->max_corner, other.max_corner );
}

mmINLINE V3f AABBf_Center( const AABBf& aabb )
{
	return (aabb.min_corner + aabb.max_corner) * 0.5f;
}
mmINLINE V3f AABBf_Extent( const AABBf& aabb )
{
	return AABBf_FullSize(aabb) * 0.5f;
}
mmINLINE V3f AABBf_FullSize( const AABBf& aabb )
{
	return aabb.max_corner - aabb.min_corner;
}
mmINLINE int AABBf_LongestAxis( const AABBf& aabb )
{
	const V3f aabbSize = AABBf_FullSize( aabb );
	return Max3Index( aabbSize.x, aabbSize.y, aabbSize.z );
}
mmINLINE bool AABBf_Intersect( const AABBf& a, const AABBf& b )
{
	bool overlap = true;
	overlap = (a.min_corner.x > b.max_corner.x || a.max_corner.x < b.min_corner.x) ? false : overlap;
	overlap = (a.min_corner.z > b.max_corner.z || a.max_corner.z < b.min_corner.z) ? false : overlap;
	overlap = (a.min_corner.y > b.max_corner.y || a.max_corner.y < b.min_corner.y) ? false : overlap;
	return overlap;
}
mmINLINE void AABBf_Merge( const AABBf& a, const AABBf& b, AABBf &_result )
{
	_result.min_corner = V3_Mins( a.min_corner, b.min_corner );
	_result.max_corner = V3_Maxs( a.max_corner, b.max_corner );
}
mmINLINE const AABBf AABBf_Merge( const AABBf& a, const AABBf& b )
{
	const AABBf result = {
		V3_Mins( a.min_corner, b.min_corner ),
		V3_Maxs( a.max_corner, b.max_corner )
	};
	return result;
}
mmINLINE const AABBf AABBf_Shift( const AABBf& aabb, const V3f& offset )
{
	const AABBf result = { aabb.min_corner + offset, aabb.max_corner + offset };
	return result;
}
mmINLINE const AABBf AABBf_Scaled( const AABBf& aabb, const float scale )
{
	const AABBf result = { aabb.min_corner * scale, aabb.max_corner * scale };
	return result;
}

inline const AABBf AABBf_GrowBy( const AABBf& aabb, const float epsilon )
{
	const V3f vEpsilon = V3_SetAll( epsilon );
	const V3f increaseAmount = (aabb.max_corner - aabb.min_corner) * epsilon + vEpsilon;
	const AABBf result = { aabb.min_corner - increaseAmount, aabb.max_corner + increaseAmount };
	return result;
}

mmINLINE bool AABBf_ContainsPoint( const AABBf& aabb, const V3f& point )
{
	//NOTE: this could have been written using '||' (short-circuiting).
	return point.x >= aabb.min_corner.x
		&& point.y >= aabb.min_corner.y
		&& point.z >= aabb.min_corner.z
		&& point.x <= aabb.max_corner.x
		&& point.y <= aabb.max_corner.y
		&& point.z <= aabb.max_corner.z;
}
mmINLINE bool AABBf_ContainsPoint( const AABBf& aabb, const V3f& point, float epsilon )
{
	//NOTE: this could have been written using '||' (short-circuiting).
	return point.x >= aabb.min_corner.x - epsilon
		&& point.y >= aabb.min_corner.y - epsilon
		&& point.z >= aabb.min_corner.z - epsilon
		&& point.x <= aabb.max_corner.x + epsilon
		&& point.y <= aabb.max_corner.y + epsilon
		&& point.z <= aabb.max_corner.z + epsilon;
}
mmINLINE V3f AABBf_GetClampedPoint( const AABBf& aabb, const V3f& _p )
{
	return CV3f(
		clampf( _p.x, aabb.min_corner.x, aabb.max_corner.x ),
		clampf( _p.y, aabb.min_corner.y, aabb.max_corner.y ),
		clampf( _p.z, aabb.min_corner.z, aabb.max_corner.z )
	);
}


/*
==========================================================
	REFLECTION
==========================================================
*/
#if MM_ENABLE_REFLECTION

mxDECLARE_STRUCT(AABBf);
mxDECLARE_POD_TYPE(AABBf);

#endif // MM_ENABLE_REFLECTION


/*
==========================================================
	LOGGING
==========================================================
*/

#if MM_DEFINE_STREAM_OPERATORS

template< typename TYPE >
ATextStream & operator << ( ATextStream & log, const AabbMM< TYPE >& bounds )
{
	log << "{min=" << bounds.min_corner << ", max=" << bounds.max_corner << "}";
	return log;
}

#endif // MM_DEFINE_STREAM_OPERATORS
