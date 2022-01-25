// Bounding Sphere.
#pragma once

#include <Base/Math/MiniMath.h>


///
template< typename Scalar >
struct TSphere
{
	typedef Tuple3< Scalar > V3t;

	V3t		center;
	Scalar	radius;	//!< must always be > 0

public:
	mmINLINE static const TSphere create( const V3t& center, const Scalar radius )
	{
		const TSphere result = { center, radius };
		return result;
	}

public:
	mmINLINE const TSphere Translated( const V3t& offset ) const
	{
		const TSphere result = { this->center + offset, this->radius };
		return result;
	}

public:
	/// includes touching
	mmINLINE bool containsPoint( const V3t& point ) const
	{
		return ( this->center - point ).lengthSquared() <= ( this->radius * this->radius );
	}

	//
	mmINLINE Scalar signedDistanceToPoint( const V3t& point ) const
	{
		return ( this->center - point ).length() - this->radius;
	}
};


/*
==========================================================
	TYPEDEFS
==========================================================
*/
typedef TSphere< F32 >		Sphere16;
typedef TSphere< double >	BSphereD;

// makes an inside-out sphere
void Sphere16_Clear( Sphere16 *sphere );
const Sphere16 Sphere_Set( const V3f& center, const float radius );

/*
==========================================================
	REFLECTION
==========================================================
*/
#if MM_ENABLE_REFLECTION

mxDECLARE_STRUCT(Sphere16);
mxDECLARE_POD_TYPE(Sphere16);

#endif // MM_ENABLE_REFLECTION

/*
==========================================================
	LOGGING
==========================================================
*/

#if MM_DEFINE_STREAM_OPERATORS

#endif // MM_DEFINE_STREAM_OPERATORS


/*
==========================================================
	INLINE IMPLEMENTATIONS
==========================================================
*/
mmINLINE void Sphere16_Clear( Sphere16 *sphere )
{
	sphere->center = V3_Zero();
	sphere->radius = -1.0f;
}

mmINLINE const Sphere16 Sphere_Set( const V3f& center, const float radius )
{
	const Sphere16 result = { center, radius };
	return result;
}
