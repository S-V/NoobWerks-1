#pragma once


#include <Base/Math/Matrix/TMatrix3x3.h>


//=====================================================================
//	MATRIX OPERATIONS
//=====================================================================

const M33f 		M33_Identity();
const M33f 		M33_Transpose( const M33f& m );
const bool 		M33_inverse( M33f & m_ );
const M33f 		M33_Multiply( const M33f& a, const M33f& b );
const V3f		M33_Transform( const M33f& m, const V3f& p );
const M33f		M33_FromQuaternion( Vec4Arg0 Q );
const bool		M33_isSymmetric( const M33f& m, const float epsilon = MATRIX_EPSILON );
const bool		M33_rowsNormalized( const M33f& m, const float epsilon = MATRIX_EPSILON );
const double	M33_determinant( const M33f& m );
const bool		M33_tryInverse( M33f & m, const float matrix_inverse_epsilon = MATRIX_INVERSE_EPSILON );
const bool		M33_equal( const M33f& a, const M33f& b, const float epsilon );
const bool		M33_isValidRotationMatrix( const M33f& m, const float epsilon = MATRIX_EPSILON );
const M33f		M33_GetSkewSymmetricMatrix( const V3f& v );
const M33f		M33_RotationX( float angle );
const M33f		M33_RotationY( float angle );
const M33f		M33_RotationZ( float angle );
const M33f		M33_RotationPitchRollYaw( float pitch, float roll, float yaw );
const M33f		M33_RotationNormal( const V3f& normalAxis, float angle );
const V3f		M33_ExtractPitchYawRoll( const M33f& m );
