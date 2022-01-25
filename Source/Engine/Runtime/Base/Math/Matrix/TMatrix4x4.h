// Templated 4x4 matrix type.
#pragma once


#include <Base/Math/MiniMath.h>
#include <Base/Math/Matrix/TMatrix3x3.h>


///
template< typename Scalar >
struct TMatrix4x4
{
	typedef Tuple3< Scalar >	V3t;
	typedef CTuple3< Scalar >	CV3t;
	typedef Tuple4< Scalar >	V4t;
	typedef CTuple4< Scalar >	CV4t;

	typedef
		typename TSIMDVector< typename Scalar >::Vector4Type
		SimdV4t;

	typedef TMatrix3x3< Scalar >	Matrix3x3t;


	union
	{
		// scalars
		Scalar	m[4][4];
		Scalar	a[16];

		// vectors
		struct {
			V4t	v0, v1, v2, v3;	// rows
		};
		V4t	v[4];

		struct {
			SimdV4t	r0, r1, r2, r3;	// registers
		};
		SimdV4t	r[4];
	};

public:	// Overloaded Operators.

	mmINLINE V4t& operator [] ( int row )
	{ return v[row]; }

	mmINLINE const V4t& operator [] ( int row ) const
	{ return v[row]; }

public:

	mmINLINE const V3t GetScaling() const
	{
		return CV3t(m[0][0], m[1][1], m[2][2]);
	}

	/// Transforms a 4D vector by a 4x4 matrix: v' = v * M.
	mmINLINE V4t TransformVector4( const V4t& v ) const
	{
		V4t	result;
		// v' = v * M
		result.x = (this->v0.x * v.x) + (this->v1.x * v.y) + (this->v2.x * v.z) + (this->v3.x * v.w);
		result.y = (this->v0.y * v.x) + (this->v1.y * v.y) + (this->v2.y * v.z) + (this->v3.y * v.w);
		result.z = (this->v0.z * v.x) + (this->v1.z * v.y) + (this->v2.z * v.z) + (this->v3.z * v.w);
		result.w = (this->v0.w * v.x) + (this->v1.w * v.y) + (this->v2.w * v.z) + (this->v3.w * v.w);
		return result;
	}

	/// Transforms a 3D point (W=1) by a 4x4 matrix.
	mmINLINE V4t transformPointH( const V3t& p ) const
	{
		V4t	result;
		// p' = p * M
		result.x = (this->v0.x * p.x) + (this->v1.x * p.y) + (this->v2.x * p.z) + (this->v3.x);
		result.y = (this->v0.y * p.x) + (this->v1.y * p.y) + (this->v2.y * p.z) + (this->v3.y);
		result.z = (this->v0.z * p.x) + (this->v1.z * p.y) + (this->v2.z * p.z) + (this->v3.z);
		result.w = (this->v0.w * p.x) + (this->v1.w * p.y) + (this->v2.w * p.z) + (this->v3.w);
		return result;
	}

	mmINLINE V3t transformPoint( const V3t& p ) const
	{
		return V3t::fromXYZ( this->transformPointH( p ) );
	}

	/// [ X, Y, Z, W ] => projection transform => homogeneous divide => [Px, Py, Pz, W ]
	mmINLINE const V4t projectPoint( const V3t& p ) const
	{
		const V4t pointH = this->transformPointH( p );
		const float invW = Scalar(1) / pointH.w;

		return V4t::set(
			pointH.x * invW,
			pointH.y * invW,
			pointH.z * invW,
			pointH.w
			);
	}


public:
	static TMatrix4x4 createUniformScaling( const Scalar uniform_scaling )
	{
		TMatrix4x4	result;
		result.v0 = CV4t( uniform_scaling, 0, 0, 0 );
		result.v1 = CV4t( 0, uniform_scaling, 0, 0 );
		result.v2 = CV4t( 0, 0, uniform_scaling, 0 );
		result.v3 = CV4t( 0, 0, 0, 1 );
		return result;
	}

	/// Builds a matrix which translates by (x, y, z).
	static TMatrix4x4 createTranslation( const V3t& translation )
	{
		TMatrix4x4	result;
		result.v0 = CV4t( 1, 0, 0, 0 );
		result.v1 = CV4t( 0, 1, 0, 0 );
		result.v2 = CV4t( 0, 0, 1, 0 );
		result.v3 = CV4t( translation, 1 );
		return result;
	}

	static TMatrix4x4 CreateWorldMatrix(
		const V3t& right_dir
		, const V3t& forward_dir
		, const V3t& up_dir
		, const V3t& translation
		)
	{
		TMatrix4x4	result;

		result.v0 = CV4t( right_dir,    0 );
		result.v1 = CV4t( forward_dir,  0 );
		result.v2 = CV4t( up_dir,		0 );
		result.v3 = CV4t( translation, 1 );

		return result;
	}


	static TMatrix4x4 create(
		const Matrix3x3t& rotation
		, const V3t& translation
		, const Scalar uniform_scaling = Scalar(1)
		)
	{
		TMatrix4x4	result;

		// NOTE: the 3x3 rotation matrix is transposed because it is column-major

		const Matrix3x3t& R = rotation;
		const Scalar S = uniform_scaling;

		result.v0 = CV4t( R.m[0][0] * S,    R.m[1][0],         R.m[2][0],        0 );
		result.v1 = CV4t( R.m[0][1],        R.m[1][1] * S,     R.m[2][1],        0 );
		result.v2 = CV4t( R.m[0][2],        R.m[1][2],         R.m[2][2] * S,    0 );

		result.v3 = CV4t( translation, 1 );

		return result;
	}

public:
	mmINLINE TMatrix4x4& setTranslation( const V3t& translation )
	{
		v3 = CV4t( translation, 1 );
		return *this;
	}

	mmINLINE void SetTranslationZ( const Scalar& new_z )
	{
		v3.z = new_z;
	}

	mmINLINE const V3f& GetTranslation() const
	{
		return *reinterpret_cast< const V3f* >( &v3.x );
	}
};

/// Matrix transform concatenation operator.
/// Performs a 4x4 matrix multiply by a 4x4 matrix.
/// Matrix multiplication order: right-to-left as in Math books,
/// i.e. the transform 'B' is applied after 'A'.
///
template< typename Scalar >
static mmINLINE
TMatrix4x4<Scalar> operator <<= (
								 const TMatrix4x4<Scalar>& B
								 , const TMatrix4x4<Scalar>& A
								 )
{
	TMatrix4x4<Scalar>	result;
	result.v0 = A.TransformVector4( B.v0 );
	result.v1 = A.TransformVector4( B.v1 );
	result.v2 = A.TransformVector4( B.v2 );
	result.v3 = A.TransformVector4( B.v3 );
	return result;
}



typedef TMatrix4x4< double >	M44d;


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	TYPE CONVERSIONS
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

template< class SrcScalar, class DstScalar >
mmINLINE
void V4_Copy_Convert(
					 Tuple4<DstScalar> *destination_vector
					 , const Tuple4<SrcScalar>& source_vector
					 )
{
	destination_vector->x = source_vector.x;
	destination_vector->y = source_vector.y;
	destination_vector->z = source_vector.z;
	destination_vector->w = source_vector.w;
}

template< class MatrixTypeTo, class MatrixTypeFrom >
mmINLINE
MatrixTypeTo M44_ConvertTo( const MatrixTypeFrom& source_matrix )
{
	mxSTATIC_ASSERT(sizeof(MatrixTypeTo) != sizeof(MatrixTypeFrom));

	MatrixTypeTo	result;
	V4_Copy_Convert( &result.v0, source_matrix.v0 );
	V4_Copy_Convert( &result.v1, source_matrix.v1 );
	V4_Copy_Convert( &result.v2, source_matrix.v2 );
	V4_Copy_Convert( &result.v3, source_matrix.v3 );
	return result;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	TRANSFORMING BOUNDING BOXES
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

template< typename Scalar >
struct TScaleTranslation
{
	typedef Tuple3< Scalar >		V3t;
	typedef Tuple4< Scalar >		V4t;
	typedef CTuple4< Scalar >		CV4t;
	typedef TMatrix4x4< Scalar >	M44t;

	V3t		translation;
	Scalar	scale;

public:

	// WRONG!

	//mmINLINE V3t transformVector3D( const V3t& v ) const
	//{
	//	return ( v + GetTranslation ) * scale;
	//}

	//mmINLINE M44t	toMatrix4x4() const
	//{
	//	M44t	result;
	//	result.v0 = CV4t( scale, 0, 0, 0 );
	//	result.v1 = CV4t( 0, scale, 0, 0 );
	//	result.v2 = CV4t( 0, 0, scale, 0 );
	//	result.v3 = CV4t( GetTranslation, 1 );
	//	return result;
	//}
};

//template< class SrcScalar, class DstScalar >
//Tuple4<DstScalar> V4_Converted( const Tuple4<SrcScalar>& source_vector )
//{
//	Tuple4<DstScalar>	result;
//	result.x = source_vector.x;
//	result.y = source_vector.y;
//	result.z = source_vector.z;
//	result.w = source_vector.w;
//	return result;
//}


//template< typename Scalar >
//mmINLINE
//TScaleTranslation<Scalar> ST_getCubeTransform(
//	const CubeML<Scalar>& from
//	, const CubeML<Scalar>& to
//	)
//{
//	TScaleTranslation<Scalar>	result;
//	result.scale = to.side_length / from.side_length;
//	result.GetTranslation = to.min_corner - from.min_corner;
//	return result;
//}
