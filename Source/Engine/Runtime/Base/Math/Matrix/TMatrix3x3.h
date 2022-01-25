// Templated 3x3 matrix type.
#pragma once

#include <Base/Math/MiniMath.h>
#include <Base/Math/EulerAngles.h>


///
///------------------------------------------------------------------
///	3x3 matrix
///------------------------------------------------------------------
///
template< typename Scalar >
union TMatrix3x3
{
	typedef Tuple3< Scalar >	V3t;
	typedef CTuple3< Scalar >	CV3t;

	// scalars
	Scalar	m[3][3];
	Scalar	a[9];

	V3t		r[3];	// rows
	struct
	{
		V3t	r0, r1, r2;	// rows
	};

public:	// Overloaded Operators.

	mmINLINE V3t& operator [] ( int row )
	{ return r[row]; }

	mmINLINE const V3t& operator [] ( int row ) const
	{ return r[row]; }

	mmINLINE Scalar operator () ( int row, int column ) const
	{ return m[row][column]; }

	mmINLINE Scalar& operator () ( int row, int column )
	{ return m[row][column]; }

public:
	mmINLINE static const TMatrix3x3 identity()
	{
		const TMatrix3x3 result = {
			1, 0, 0,
			0, 1, 0,
			0, 0, 1,
		};
		return result;
	}

	static mmINLINE
	const TMatrix3x3 fromColumns(
		const V3t& col0, const V3t& col1, const V3t& col2
		)
	{
		TMatrix3x3	m;
		m.r0 = CV3t( col0.x, col1.x, col2.x );
		m.r1 = CV3t( col0.y, col1.y, col2.y );
		m.r2 = CV3t( col0.z, col1.z, col2.z );
		return m;
	}

public:

	mmINLINE bool isSymmetric( const Scalar epsilon = MATRIX_EPSILON ) const
	{
		return
			fabs( m[0][1] - m[1][0] ) <= epsilon &&
			fabs( m[0][2] - m[2][0] ) <= epsilon &&
			fabs( m[1][2] - m[2][1] ) <= epsilon ;
	}

	mmINLINE bool isDiagonal( const Scalar epsilon = MATRIX_EPSILON ) const
	{
		return
			fabs( m[0][1] ) <= epsilon &&
			fabs( m[0][2] ) <= epsilon &&
			fabs( m[1][0] ) <= epsilon &&
			fabs( m[1][2] ) <= epsilon &&
			fabs( m[2][0] ) <= epsilon &&
			fabs( m[2][1] ) <= epsilon ;
	}

	mmINLINE const TMatrix3x3 transposed() const
	{
		TMatrix3x3	result;
		result.r0 = CV3t( this->r0.x, this->r1.x, this->r2.x );
		result.r1 = CV3t( this->r0.y, this->r1.y, this->r2.y );
		result.r2 = CV3t( this->r0.z, this->r1.z, this->r2.z );
		return result;
	}

public:

	/// Transforms a 3D vector by this 3x3 matrix: v' = M * v.
	mmINLINE V3t transformVector( const V3t& v ) const
	{
		V3t	result;
		result.x = (this->r0.x * v.x) + (this->r0.y * v.y) + (this->r0.z * v.z);
		result.y = (this->r1.x * v.x) + (this->r1.y * v.y) + (this->r1.z * v.z);
		result.z = (this->r2.x * v.x) + (this->r2.y * v.y) + (this->r2.z * v.z);
		return result;
	}

public:

	/// returns the pitch/yaw/roll each in the range [-180, 180] degrees
	NwEulerAngles toEulerAngles() const
	{
		NwEulerAngles angles;

		float s = sqrt( m[0][0] * m[0][0] + m[0][1] * m[0][1] );

		if ( s > FLT_EPSILON )
		{
			angles.yaw = RAD2DEG( atan2( m[0][1], m[0][0] ) );
			angles.pitch = RAD2DEG( - atan2( m[0][2], s ) );
			angles.roll = RAD2DEG( atan2( m[1][2], m[2][2] ) );
		}
		else
		{
			angles.yaw = RAD2DEG( - atan2( m[1][0], m[1][1] ) );
			angles.pitch = m[0][2] < 0.0f ? 90.0f : -90.0f;
			angles.roll = 0.0f;
		}

		return angles;
	}
};


typedef TMatrix3x3< float >		M33f;
typedef TMatrix3x3< double >	M33d;


ASSERT_SIZEOF(M33f, 36);
