// Bounding Cube.
#pragma once

#include <Base/Math/MiniMath.h>
#include <Base/Math/BoundingVolumes/AABB.h>



/// minimum corner and size (side length)
template< typename Scalar >
struct CubeML
{
	typedef Scalar				Scalar;
	typedef Tuple3< Scalar >	V3t;
	typedef CTuple3< Scalar >	CV3t;

	V3t		min_corner;		//!< the minimum corner of the cube
	Scalar	side_length;	//!< the size of the cube

public:
	mmINLINE static CubeML create( const V3t& min_corner, const Scalar size )
	{
		CubeML	result;
		result.min_corner = min_corner;
		result.side_length = size;
		return result;
	}

	mmINLINE static CubeML fromCenterRadius( const V3t& center, const Scalar radius )
	{
		CubeML	result;
		result.min_corner = center - CV3t(radius);
		result.side_length = radius * Scalar(2.0);
		return result;
	}

	template< typename OtherScalarType >
	mmINLINE static CubeML fromOther( const CubeML<OtherScalarType>& other )
	{
		CubeML	result;
		result.min_corner = V3t::fromXYZ( other.min_corner );
		result.side_length = Scalar( other.side_length );
		return result;
	}

public:
	mmINLINE const V3t minCorner() const
	{
		return min_corner;
	}

	mmINLINE const V3t maxCorner() const
	{
		return min_corner + CV3t( side_length );
	}

	mmINLINE const Scalar sideLength() const
	{
		return side_length;
	}

	/// Returns the center point of this cube.
	mmINLINE const V3t center() const
	{
		return min_corner + CV3t( side_length * Scalar(0.5) );
	}

	mmINLINE const AabbMM< Scalar > ToAabbMinMax() const
	{
		return AabbMM< Scalar >::make(
			minCorner(), maxCorner()
			);
	}

	/// includes touching
	mmINLINE bool containsPoint( const V3t& point ) const
	{
		return V3t::greaterOrEqualTo( point, this->min_corner ) && V3t::lessOrEqualTo( point, this->maxCorner() );
	}

	mmINLINE const V3t getClampedPoint( const V3t& position ) const
	{
		return CV3t(
			Clamp( position.x, min_corner.x, min_corner.x + side_length ),
			Clamp( position.y, min_corner.y, min_corner.y + side_length ),
			Clamp( position.z, min_corner.z, min_corner.z + side_length )
			);
	}

	mmINLINE const CubeML ExpandedBy( const Scalar margin ) const
	{
		CubeML	expanded_cube;
		expanded_cube.side_length = this->side_length + margin * Scalar(2);
		expanded_cube.min_corner = this->min_corner - CV3t(Scalar(margin));
		return expanded_cube;
	}

	mmINLINE const CubeML Scaled( const Scalar scale ) const
	{
		CubeML	scaled_cube;
		scaled_cube.side_length = this->side_length * scale;
		scaled_cube.min_corner = this->min_corner - CV3t(
			(scaled_cube.side_length - this->side_length) * Scalar(0.5)
			);
		return scaled_cube;
	}

	/// Converts coords to [0..1] range.
	mmINLINE const V3t getNormalizedPosition( const V3t& position_inside_cube ) const
	{
		mxASSERT(this->containsPoint( position_inside_cube ));
		return V3t::div( position_inside_cube - this->min_corner, this->side_length );
	}

	mmINLINE const V3t restoreOriginalPosition( const V3t& normalized01_position_in_cube ) const
	{
		return normalized01_position_in_cube * this->side_length
			+ this->min_corner
			;
	}

	mmINLINE const V3t getCorner( const UINT cube_corner_index ) const
	{
		return CV3t(
			min_corner.x + ( ( cube_corner_index & BIT(AXIS_X) ) ? side_length : Scalar(0) ),
			min_corner.y + ( ( cube_corner_index & BIT(AXIS_Y) ) ? side_length : Scalar(0) ),
			min_corner.z + ( ( cube_corner_index & BIT(AXIS_Z) ) ? side_length : Scalar(0) )
		);
	}

	EPlaneSide relationToPlane( const TPlane< Scalar >& plane ) const
	{
		// Convert AABB to center-extents representation

		const V3t	cube_center = this->center();
		const V3t	cube_half_extent = cube_center - min_corner;

		 // Compute the projection interval radius of cube wrt the plane
		const V3t		abs_plane_normal = V3t::abs( plane.normal() );
		const Scalar	projected_cube_radius = V3t::dot( cube_half_extent, abs_plane_normal );

		const Scalar	distance_from_cube_center_to_plane = plane.distanceToPoint( cube_center );

		if( distance_from_cube_center_to_plane > projected_cube_radius )
		{
			return PLANESIDE_FRONT;
		}
		else if( distance_from_cube_center_to_plane < -projected_cube_radius )
		{
			return PLANESIDE_BACK;
		}
		else
		{
			return PLANESIDE_CROSS;
		}
	}

public:
	static const CubeML makeUnsignedUnitCube()
	{
		return create( CV3t(0), Scalar(1) );
	}
};



/// center and 'radius' (half-extent)
template< typename Scalar >
struct CubeCR
{
	typedef Tuple3< Scalar >	V3t;
	typedef CTuple3< Scalar >	CV3t;

	V3t		center;	//!< the center of the box
	Scalar	radius;	//!< the half-size of the box

public:
	static const CubeCR MakeFromCenterAndRadius(
		const V3t& cube_center,
		const Scalar cube_half_extent
		)
	{
		const CubeCR result = {
			cube_center, cube_half_extent
		};
		return result;
	}

public:
	mmINLINE const V3t minCorner() const
	{
		return center - CV3t( radius );
	}

	mmINLINE const V3t maxCorner() const
	{
		return center + CV3t( radius );
	}

	mmINLINE const Scalar sideLength() const
	{
		return radius * Scalar(2);
	}

	mmINLINE const AabbMM< Scalar > ToAabbMinMax() const
	{
		return AabbMM< Scalar >::fromSphere( center, radius );
	}

	mmINLINE bool contains( const CubeCR& other ) const
	{
		return this->ToAabbMinMax().contains( other.ToAabbMinMax() );
	}
};


/*
==========================================================
	TYPEDEFS
==========================================================
*/

typedef CubeML< F32 >	CubeMLf;	ASSERT_SIZEOF( CubeMLf, 16 );
typedef CubeML< F64 >	CubeMLd;	ASSERT_SIZEOF( CubeMLd, 32 );
typedef CubeML< I32 >	CubeMLi;	ASSERT_SIZEOF( CubeMLi, 16 );
typedef CubeML< U32 >	CubeMLu;	ASSERT_SIZEOF( CubeMLu, 16 );

typedef CubeCR< F32 >	CubeCRf;	ASSERT_SIZEOF( CubeCRf, 16 );
typedef CubeCR< F64 >	CubeCRd;	ASSERT_SIZEOF( CubeCRd, 32 );
typedef CubeCR< I32 >	CubeCRi;	ASSERT_SIZEOF( CubeCRi, 16 );
typedef CubeCR< U32 >	CubeCRu;	ASSERT_SIZEOF( CubeCRu, 16 );


/*
==========================================================
	FUNCTIONS
==========================================================
*/

template< typename Scalar >
__forceinline
TMatrix4x4<Scalar> M44_getCubeTransform(
						  const CubeML<Scalar>& src_cube
						  , const CubeML<Scalar>& dst_cube
						  )
{
	const Scalar src_cube_size = src_cube.side_length;
	const Scalar dst_cube_size = dst_cube.side_length;

	const TMatrix4x4<Scalar> center_source_cube_around_origin =
		TMatrix4x4<Scalar>::createTranslation( -src_cube.center() );

	const TMatrix4x4<Scalar> scale_source_cube_to_destination_cube_size =
		TMatrix4x4<Scalar>::createUniformScaling( dst_cube_size / src_cube_size );

	const TMatrix4x4<Scalar> translate_scaled_cube_from_origin_to_destination_center =
		TMatrix4x4<Scalar>::createTranslation( dst_cube.center() );

	const TMatrix4x4<Scalar> combined_transform
		=   translate_scaled_cube_from_origin_to_destination_center
		<<= scale_source_cube_to_destination_cube_size
		<<= center_source_cube_around_origin
		;

	//return combined_transform;

	const TMatrix4x4<Scalar> tmp0
		=   scale_source_cube_to_destination_cube_size <<= center_source_cube_around_origin;

	return translate_scaled_cube_from_origin_to_destination_center <<= tmp0;
}



template< typename Scalar >
__forceinline
M44f M44_getCubeTransform2(
						  const CubeML<Scalar>& src_cube
						  , const CubeML<Scalar>& dst_cube
						  )
{
	const Scalar src_cube_size = src_cube.side_length;
	const Scalar dst_cube_size = dst_cube.side_length;

	const M44f center_source_cube_around_origin =
		M44_buildTranslationMatrix( V3f::fromXYZ( -src_cube.center() ) );

	const M44f scale_source_cube_to_destination_cube_size =
		M44_uniformScaling( dst_cube_size / src_cube_size );

	const M44f translate_scaled_cube_from_origin_to_destination_center =
		M44_buildTranslationMatrix( V3f::fromXYZ( dst_cube.center() ) );

	const M44f combined_transform
		=   center_source_cube_around_origin
		* scale_source_cube_to_destination_cube_size
		* translate_scaled_cube_from_origin_to_destination_center
		;

	return combined_transform;
}



/// This function is used for LoD calculations.
template< typename Scalar >
inline Scalar ChebyshevDistance(
							   const CubeML< Scalar >& bounds
							   , const Tuple3< Scalar >& point
							   )
{
	// Get the closest distance between the camera and the AABB under 'max' norm:

	const Tuple3< Scalar >	max_corner = bounds.maxCorner();

	const Scalar minX = Min( fabs( point.x - bounds.min_corner.x ), fabs( point.x - max_corner.x ) );
	const Scalar minY = Min( fabs( point.y - bounds.min_corner.y ), fabs( point.y - max_corner.y ) );
	const Scalar minZ = Min( fabs( point.z - bounds.min_corner.z ), fabs( point.z - max_corner.z ) );

	return Max3( minX, minY, minZ );
}


/*
==========================================================
	REFLECTION
==========================================================
*/
#if MM_ENABLE_REFLECTION

mxDECLARE_STRUCT(CubeMLf);
mxDECLARE_POD_TYPE(CubeMLf);

mxDECLARE_STRUCT(CubeMLd);
mxDECLARE_POD_TYPE(CubeMLd);


mxDECLARE_STRUCT(CubeCRf);
mxDECLARE_POD_TYPE(CubeCRf);

mxDECLARE_STRUCT(CubeCRd);
mxDECLARE_POD_TYPE(CubeCRd);

#endif // MM_ENABLE_REFLECTION


/*
==========================================================
	LOGGING
==========================================================
*/

#if MM_DEFINE_STREAM_OPERATORS

template< typename Scalar >
ATextStream & operator << ( ATextStream & log, const CubeCR< Scalar >& bounds )
{
	log << "{center=" << bounds.center << ", radius=" << bounds.radius << "}";
	return log;
}

template< typename Scalar >
ATextStream & operator << ( ATextStream & log, const CubeML< Scalar >& cube )
{
	log << "{corner=" << cube.min_corner << ", size=" << cube.side_length << "}";
	return log;
}

#endif // MM_DEFINE_STREAM_OPERATORS
