/*
=============================================================================
	File:	Quaternion.h
	Desc:	Quaternion.
=============================================================================
*/

#pragma once
#ifndef __NW_BASE_MATH_QUATERNION_H__
#define __NW_BASE_MATH_QUATERNION_H__


#include <Base/Math/AxisAngle.h>
#include <Base/Math/EulerAngles.h>
#include <Base/Math/Matrix/TMatrix3x3.h>
#include <Base/Math/Matrix/M44f.h>	// g_MM_Identity


mxSTOLEN("idSoftware's idQuat from idLib, Doom3/Prey/Quake4 SDKs, Doom3 BFG edition");

///
struct NwQuaternion
{
	union
	{
		Vector4	q;
		V4f		v;

		struct
		{
			float	x;
			float	y;
			float	z;
			float	w;	// real part (rotation angle)
		};
	};

public:
	static mxFORCEINLINE const NwQuaternion identity()
	{
		NwQuaternion result;
		result.x = result.y = result.z = 0;
		result.w = 1;
		return result;
	}

	static mxFORCEINLINE const NwQuaternion create( float x, float y, float z, float w )
	{
		const NwQuaternion result = {
			x, y, z, w
		};
		return result;
	}

	static mxFORCEINLINE const NwQuaternion create( const float q[4] )
	{
		const NwQuaternion result = {
			q[0], q[1], q[2], q[3]
		};
		return result;
	}

	static mxFORCEINLINE const NwQuaternion FromAxisAngle(
		const NwAxisAngle& axis_angle
	)
	{
		return FromAxisAngle( axis_angle.axis, axis_angle.angle );
	}

	/// Creates a rotation which rotates angle radians around axis.
	static mxFORCEINLINE const NwQuaternion FromAxisAngle(
		const V3f& rotation_axis,
		const float angle_in_radians
	)
	{
		if(mxUNLIKELY( angle_in_radians == 0.0f )) {
			return identity();
		}

		mxASSERT(V3_IsNormalized( rotation_axis ));

		float	s, c;
		mmSinCos( angle_in_radians * 0.5f, s, c );

		const NwQuaternion result = {
			rotation_axis.x * s,
			rotation_axis.y * s,
			rotation_axis.z * s,
			c,
		};

		return result;
	}

	static mxFORCEINLINE const NwQuaternion fromRawFloatPtr( const float* q )
	{
		const NwQuaternion result = {
			q[0], q[1], q[2], q[3]
		};
		return result;
	}

	/// Creates a new quaternion representing the rotation A followed by B.
	static mxFORCEINLINE const NwQuaternion concat(
		const NwQuaternion& a,
		const NwQuaternion& b
		)
	{
		const NwQuaternion result = {
			a.w * b.x  +  a.x * b.w  +  a.y * b.z  -  a.z * b.y,
			a.w * b.y  +  a.y * b.w  +  a.z * b.x  -  a.x * b.z,
			a.w * b.z  +  a.z * b.w  +  a.x * b.y  -  a.y * b.x,
			a.w * b.w  -  a.x * b.x  -  a.y * b.y  -  a.z * b.z
		};
		return result;
	}

	static const NwQuaternion lerp(
		const NwQuaternion &from, const NwQuaternion &to
		, float fraction
		)
	{
		NwQuaternion	result;
		result.v = V4f::lerp( from.v, to.v, fraction );
		return result;
	}

public:

	mxFORCEINLINE void setIdentity()
	{
		*this = identity();
	}

	/// NOTE: The quaternion must be normalized
	/// so that its inverse is equal to its conjugate.
	mxFORCEINLINE const NwQuaternion inverse() const
	{
		const NwQuaternion result = { -x, -y, -z, w };
		return result;
	}

	mxFORCEINLINE const NwQuaternion negated() const
	{
		const NwQuaternion result = { -x, -y, -z, -w };
		return result;
	}

	const NwAxisAngle toAxisAngle() const
	{
		mxASSERT(this->IsNormalized());

		NwAxisAngle	result;

		result.angle = mmACos( w ) * 2.0f;

		// use the identity sin^2 + cos^2 = 1
		float rcp_sin_angle = mmInvSqrt( 1.0f - w * w );
		result.axis = Vector4_As_V3( q ) * rcp_sin_angle;

		return result;
	}

#if 0
	const V3f toEulerAngles() const
	{
		mxASSERT(this->IsNormalized());

		// Derivation from http://www.geometrictools.com/Documentation/EulerAngles.pdf
		// Order of rotations: Z first, then X, then Y
		float check = 2.0f * (-y * z + w * x);

		if (check < -0.995f)
		{
			return CV3f(
				-90.0f,
				0.0f,
				RAD2DEG( -atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)) )
				);
		}
		else if (check > 0.995f)
		{
			return CV3f(
				90.0f,
				0.0f,
				RAD2DEG( atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)) )
				);
		}
		else
		{
			return CV3f(
				RAD2DEG( asinf(check) ),
				RAD2DEG( atan2f(2.0f * (x * z + w * y), 1.0f - 2.0f * (x * x + y * y)) ),
				RAD2DEG( atan2f(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z)) )
				);
		}
	}
#endif


	float CalcW() const
	{
		// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
		return sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) );
	}

	float Length() const
	{
		float len;
		len = x * x + y * y + z * z + w * w;
		return sqrt( len );
	}

	float lengthSquared() const
	{
		return x * x + y * y + z * z + w * w;
	}

	bool IsNormalized( float epsilon = 1e-5f ) const
	{
		return mmAbs( lengthSquared() - 1.0f ) < epsilon;
	}

	const NwQuaternion normalized() const
	{
		NwQuaternion	result( *this );
		result.NormalizeSelf();
		return result;
	}

	NwQuaternion& NormalizeSelf()
	{
		float len;
		float ilength;
		len = this->Length();
		if ( len ) {
			ilength = 1 / len;
			x *= ilength;
			y *= ilength;
			z *= ilength;
			w *= ilength;
		}
		return *this;
	}

	mxREMOVE_THIS
	NwQuaternion MulWith( const NwQuaternion &a ) const
	{
		NwQuaternion result = {
			w*a.x + x*a.w + y*a.z - z*a.y,
			w*a.y + y*a.w + z*a.x - x*a.z,
			w*a.z + z*a.w + x*a.y - y*a.x,
			w*a.w - x*a.x - y*a.y - z*a.z
		};
		return result;
	}

	const V3f rotateVector( const V3f& v ) const
	{
		mxOPTIMIZE("mul without quat-to-matrix conversion");
		return this->toMat3().transformVector( v );
#if 0
		//TODO: this is wrong - inverse rotation!
		// result = this->Inverse() * NwQuaternion( a.x, a.y, a.z, 0.0f ) * (*this)
		float xxzz = x*x - z*z;
		float wwyy = w*w - y*y;

		float xw2 = x*w*2.0f;
		float xy2 = x*y*2.0f;
		float xz2 = x*z*2.0f;
		float yw2 = y*w*2.0f;
		float yz2 = y*z*2.0f;
		float zw2 = z*w*2.0f;

		return V3f::set(
			(xxzz + wwyy)*a.x		+ (xy2 + zw2)*a.y		+ (xz2 - yw2)*a.z,
			(xy2 - zw2)*a.x			+ (y*y+w*w-x*x-z*z)*a.y	+ (yz2 + xw2)*a.z,
			(xz2 + yw2)*a.x			+ (yz2 - xw2)*a.y		+ (wwyy - xxzz)*a.z
			);
#endif
	}

	M33f toMat3() const
	{
		M33f	mat;

#if 0
		// https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion

		const float xx = this->x * this->x;
		const float yy = this->y * this->y;
		const float xy = this->x * this->y;
		const float xz = this->x * this->z;
		const float xw = this->x * this->w;
		const float yz = this->y * this->z;
		const float yw = this->y * this->w;
		const float zz = this->z * this->z;
		const float zw = this->z * this->w;

		//
		const V3f	first_row = {
			1.0f  -  2.0f * yy  -  2.0f * zz,        2.0f * xy  -  2.0f * zw,              2.0f * xz  +  2.0f * yw
		};
		const V3f	second_row = {
			2.0f * xy  +  2.0f * zw,            1.0f  -  2.0f * xx  -  2.0f * zz,          2.0f * yz  -  2.0f * xw
		};
		const V3f	third_row = {
			2.0f * xz  -  2.0f * yw,                 2.0f * yz  +  2.0f * xw,          1.0f  -  2.0f * xx  -  2.0f * yy
		};

		//
		mat.r0 = first_row;
		mat.r1 = second_row;
		mat.r2 = third_row;

#else

		// Stolen from Doom 3 (idTech 4).
		// The original code in idLib used column-major 3x3 matrices,
		// but we use row-major matrices, so we need to transpose.

		const float	x2 = x + x;
		const float	y2 = y + y;
		const float	z2 = z + z;

		const float	xx = x * x2;
		const float	xy = x * y2;
		const float	xz = x * z2;

		const float	yy = y * y2;
		const float	yz = y * z2;
		const float	zz = z * z2;

		const float	wx = w * x2;
		const float	wy = w * y2;
		const float	wz = w * z2;

		mat.m[ 0 ][ 0 ] = 1.0f - ( yy + zz );
		mat.m[ 0 ][ 1 ] = xy + wz;
		mat.m[ 0 ][ 2 ] = xz - wy;

		mat.m[ 1 ][ 0 ] = xy - wz;
		mat.m[ 1 ][ 1 ] = 1.0f - ( xx + zz );
		mat.m[ 1 ][ 2 ] = yz + wx;

		mat.m[ 2 ][ 0 ] = xz + wy;
		mat.m[ 2 ][ 1 ] = yz - wx;
		mat.m[ 2 ][ 2 ] = 1.0f - ( xx + yy );

#endif

		return mat;
	}

	M44f ToMat4() const
	{
		return M44_FromRotationMatrix(this->toMat3());
	}

	/// angles are in radians!
	static const NwQuaternion FromAngles_YawPitchRoll(
		const float yaw,
		const float pitch,
		const float roll = 0
		)
	{
		float s1, c1;
		mmSinCos( roll * 0.5f, s1, c1 );

		float s2, c2;
		mmSinCos( yaw * 0.5f, s2, c2 );

		float s3, c3;
		mmSinCos( pitch * 0.5f, s3, c3 );

		const float c1_x_c2 = c1 * c2;
		const float s1_x_s2 = s1 * s2;

		const NwQuaternion result = {
			c1_x_c2 * s3 + s1_x_s2 * c3,
			s1 * c2 * c3 + c1 * s2 * s3,
			c1 * s2 * c3 - s1 * c2 * s3,
			c1_x_c2 * c3 - s1_x_s2 * s3
		};
		return result;
	}

	static const NwQuaternion FromEulerAngles( const NwEulerAngles& euler_angles )
	{
		return FromAngles_YawPitchRoll(
			euler_angles.yaw,
			euler_angles.pitch,
			euler_angles.roll
			);
	}

	static const NwQuaternion FromEulerAngles( const V3f& euler_angles_in_degrees )
	{
		return FromAngles_YawPitchRoll(
			euler_angles_in_degrees.z,
			euler_angles_in_degrees.x,
			euler_angles_in_degrees.y
			);
	}

public:	// SLERP

	/// Spherical linear interpolation between two quaternions.
	NwQuaternion &SlerpSelf( const NwQuaternion &from, const NwQuaternion &to, float t )
	{
		NwQuaternion	temp;
		float	omega, cosom, sinom, scale0, scale1;

		if ( t <= 0.0f ) {
			*this = from;
			return *this;
		}

		if ( t >= 1.0f ) {
			*this = to;
			return *this;
		}

		if ( from == to ) {
			*this = to;
			return *this;
		}

		cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
		if ( cosom < 0.0f ) {
			temp = -to;
			cosom = -cosom;
		} else {
			temp = to;
		}

		if ( ( 1.0f - cosom ) > 1e-6f ) {
#if 1
			omega = acos( cosom );
			sinom = 1.0f / sin( omega );
			scale0 = sin( ( 1.0f - t ) * omega ) * sinom;
			scale1 = sin( t * omega ) * sinom;
#else
			scale0 = 1.0f - cosom * cosom;
			sinom = idMath::InvSqrt( scale0 );
			omega = idMath::ATan16( scale0 * sinom, cosom );
			scale0 = idMath::Sin16( ( 1.0f - t ) * omega ) * sinom;
			scale1 = idMath::Sin16( t * omega ) * sinom;
#endif
		} else {
			scale0 = 1.0f - t;
			scale1 = t;
		}

		*this = ( from * scale0 ) + ( temp * scale1 );
		return *this;
	}

public:	// Operator Overloads

	inline NwQuaternion operator-() const
	{
		NwQuaternion result = { -x, -y, -z, -w };
		return result;
	}
	inline NwQuaternion operator*( float a ) const
	{
		NwQuaternion result = { x * a, y * a, z * a, w * a };
		return result;
	}

	inline NwQuaternion operator+( const NwQuaternion &a ) const
	{
		NwQuaternion result = { x + a.x, y + a.y, z + a.z, w + a.w };
		return result;
	}
	inline bool equals( const NwQuaternion &a ) const
	{
		return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && ( w == a.w ) );
	}

	inline bool equals( const NwQuaternion &a, const float epsilon ) const
	{
		if ( mmAbs( x - a.x ) > epsilon ) {
			return false;
		}
		if ( mmAbs( y - a.y ) > epsilon ) {
			return false;
		}
		if ( mmAbs( z - a.z ) > epsilon ) {
			return false;
		}
		if ( mmAbs( w - a.w ) > epsilon ) {
			return false;
		}
		return true;
	}

	inline bool operator==( const NwQuaternion &a ) const
	{
		return equals( a );
	}

	inline bool operator!=( const NwQuaternion &a ) const
	{
		return !equals( a );
	}

public:	// Logging

	friend ATextStream & operator << ( ATextStream & log, const NwQuaternion& q );

	String128 humanReadableString() const;
};

ASSERT_SIZEOF(NwQuaternion, 16);

/// Quaternion concatenation operator.
/// The multiplication order: right-to-left as in Math books,
/// i.e. the transform 'B' is applied after 'A'.
///
static mxFORCEINLINE
const NwQuaternion operator <<= (
								 const NwQuaternion& second_rotation
								 , const NwQuaternion& first_rotation
								 )
{
	return NwQuaternion::concat( first_rotation, second_rotation );
}



/// shortcut to do less typing
typedef NwQuaternion	Q4f;








/// Compressed quaternion
template< typename TYPE >
struct TPackedQuaternion
{
	TYPE	x;
	TYPE	y;
	TYPE	z;

public:
	mmINLINE NwQuaternion ToQuat() const
	{
		// for now, only floats are supported
		ASSERT_SIZEOF(TYPE, sizeof(float));

		// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
		NwQuaternion result = { x, y, z, sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) ) };
		return result;
	}
};

typedef TPackedQuaternion< float >	Q3f;







//=====================================================================
//	QUATERNION OPERATIONS
//	Quaternion: q = u*sin(a/2) + cos(a/2),
//	where u - a unit vector (axis of rotation), a - angle of rotation;
//	if N - (normalized) axis vector, then Q = ( N*sin(a/2), cos(a/2) )
//	A quaternion only represents a rotation if it is normalized.
//	If it is not normalized, then there is also a uniform scale that accompanies the rotation.
//=====================================================================

const bool      Quaternion_Equal( Vec4Arg0 a, Vec4Arg0 b );
const bool      Quaternion_NotEqual( Vec4Arg0 a, Vec4Arg0 b );
const bool      Quaternion_IsNaN( Vec4Arg0 q );
const bool      Quaternion_IsInfinite( Vec4Arg0 q );
const bool      Quaternion_IsIdentity( Vec4Arg0 q );

const Vector4	Quaternion_Identity();
//const Vector4	Quaternion_Dot( Vec4Arg0 a, Vec4Arg0 b );
const Vector4	Quaternion_Multiply( Vec4Arg0 a, Vec4Arg0 b );
const Vector4	Quaternion_LengthSq( Vec4Arg0 q );
const Vector4	Quaternion_ReciprocalLength( Vec4Arg0 q );
const Vector4	Quaternion_Length( Vec4Arg0 q );
const Vector4	Quaternion_NormalizeEst( Vec4Arg0 q );
const Vector4	Quaternion_Normalize( Vec4Arg0 q );
const Vector4	Quaternion_Conjugate( Vec4Arg0 q );
const Vector4	Quaternion_Inverse( Vec4Arg0 q );
const Vector4	Quaternion_Ln( Vec4Arg0 q );
const Vector4	Quaternion_Exp( Vec4Arg0 q );
const Vector4	Quaternion_Slerp( Vec4Arg0 Q0, Vec4Arg0 a, float t );
const Vector4	Quaternion_SlerpV( Vec4Arg0 Q0, Vec4Arg0 a, Vec4Arg0 T );
const Vector4	Quaternion_Squad( Vec4Arg0 Q0, Vec4Arg0 a, Vec4Arg0 b, Vec4Arg4 Q3, float t );
const Vector4	Quaternion_SquadV( Vec4Arg0 Q0, Vec4Arg0 a, Vec4Arg0 b, Vec4Arg4 Q3, Vec4Arg5 T );
void			Quaternion_SquadSetup(_Out_ Vector4* pA, _Out_ Vector4* pB, _Out_ Vector4* pC, Vec4Arg0 Q0, Vec4Arg0 a, Vec4Arg0 b, Vec4Arg4 Q3 );
const Vector4	Quaternion_BaryCentric( Vec4Arg0 Q0, Vec4Arg0 a, Vec4Arg0 b, float f, float g );
const Vector4	Quaternion_BaryCentricV( Vec4Arg0 Q0, Vec4Arg0 a, Vec4Arg0 b, Vec4Arg4 F, Vec4Arg5 G );

const Vector4	Quaternion_RotationPitchRollYaw( float pitch, float roll, float yaw );
const Vector4	Quaternion_RotationRollPitchYawFromVector( Vec4Arg0 Angles );
const Vector4	Quaternion_RotationNormal( const V3f& normalAxis, float angle );
const Vector4	Quaternion_RotationAxis( Vec4Arg0 axis, float angle );
const Vector4	Quaternion_RotationMatrix(Mat4Arg M );

void			Quaternion_ToAxisAngle(_Out_ Vector4* pAxis, _Out_ float* pAngle, Vec4Arg0 q );

// Q4f
// Q3f - compressed quaternion

//=====================================================================
//	DUAL QUATERNIONS
//=====================================================================

const Vector8	DualQuaternion_Identity();




/*
=======================================================================
	INLINE
=======================================================================
*/
mmINLINE const bool Quaternion_IsIdentity( Vec4Arg0 q )
{
	return Vector4_Equal( q, g_MM_Identity.r3 );	// {0,0,0,1}
}
mmINLINE const Vector4 Quaternion_Identity()
{
	return g_MM_Identity.r3;	// {0,0,0,1}
}




/*
=======================================================================
	REFLECTION
=======================================================================
*/
#if MM_ENABLE_REFLECTION

#include <Base/Object/Reflection/Reflection.h>
#include <Base/Object/Reflection/StructDescriptor.h>

mxDECLARE_STRUCT(Q4f);

#endif // MM_ENABLE_REFLECTION


/*
==========================================================
	UNIT TESTS
==========================================================
*/

#if MX_DEVELOPER

void UnitTest_Quaternions();

#endif // MX_DEVELOPER

#endif /* !__BASE_MATH_QUATERNION_H__ */
