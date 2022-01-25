#pragma once


#include <Base/Math/Matrix/TMatrix4x4.h>
#include <Base/Math/Matrix/M33f.h>


///
///-------------------------------------------------------------------
///	4x4 matrix: sixteen 32 bit floating point components
///	aligned on a 16 byte boundary and mapped to four hardware vector registers.
///	Translation component is in r[3].xyz.
///	NOTE: both in OpenGL and Direct3D the matrix layout in memory is the same.
///	Column-major versus row-major is purely a notational convention.
///-------------------------------------------------------------------
///
//template<>
//struct mxPREALIGN(16) TMatrix4x4< float >
//{
//	union {
//		// scalars
//		float	m[4][4];
//		struct {
//			V4f	v0, v1, v2, v3;	// rows
//		};
//		V4f		v[4];
//
//		// vectors
//		struct {
//			Vector4	r0, r1, r2, r3;	// rows/registers
//		};
//		Vector4	r[4];
//
//		float	a[16];
//	};
//
//#if MM_OVERLOAD_OPERATORS
//
//	mmINLINE Vector4& operator [] ( int row )
//	{ return r[row]; }
//
//	mmINLINE const Vector4& operator [] ( int row ) const
//	{ return r[row]; }
//
//#endif
//
//};

typedef TMatrix4x4< float > M44f;

ASSERT_SIZEOF(M44f, 64);


//@todo: specify passing by value or register depending on the platform (e.g. pass in-register on Xbox 360, by reference otherwise)
#if !MM_NO_INTRINSICS
	typedef const M44f &	Mat4Arg;
#else
	typedef const M44f &	Mat4Arg;
#endif

	
//=====================================================================
//	MATRIX OPERATIONS
//=====================================================================

const M44f 		M44_Identity();
const M44f 		M44_uniformScaling( float uniform_scaling );
const M44f 		M44_scaling( const V3f& scaling );
const M44f 		M44_Scaling( float x, float y, float z );
const M44f 		M44_buildTranslationMatrix( float x, float y, float z );
const M44f 		M44_buildTranslationMatrix( const V3f& xyz );
const V4f		M44_Transform( const M44f& m, const V4f& p );
const V4f		M44_Transform3( const M44f& m, const V4f& p );
const M44f 		M44_Multiply( Mat4Arg A, Mat4Arg B );
const M44f 		M44_Transpose( Mat4Arg M );
const bool 		M44_TryInverse( M44f & m );
const M44f 		M44_Inverse( const M44f& m );
const M44f 		M44_OrthoInverse( Mat4Arg m );
const M44f		ProjectionM44_Inverse( Mat4Arg m );
const M44f 		M44_LookAt( const V3f& eyePosition, const V3f& cameraTarget, const V3f& cameraUpVector );
const M44f 		M44_LookTo( const V3f& eyePosition, const V3f& lookDirection, const V3f& upDirection );
const M44f 		M44_PerspectiveOGL( float FoV_Y_radians, float aspect_ratio, float near_clip, float far_clip );
const M44f 		M44_PerspectiveD3D( float FoV_Y_radians, float aspect_ratio, float near_clip, float far_clip );
const M44f 		M44_OrthographicD3D( float width, float height, float near_clip, float far_clip );
const M44f 		M44_OrthographicD3D( float left, float right, float bottom, float top, float near_clip, float far_clip );
const M44f		M44_BuildTRS( const V3f& translation, const M33f& rotation, const V3f& scaling );
const M44f		M44_BuildTRS( const V3f& translation, const V4f& rotation, const V3f& scaling );

/// Builds a matrix which translates and scales.
const M44f		M44_BuildTS( const V3f& translation, const V3f& scaling );
const M44f		M44_BuildTS( const V3f& _translation, const float _scaling );
const M44f		M44_BuildTR( const V3f& translation, const V4f& rotation );
const V4f		M44_Project( const M44f& m, const V4f& p );
const bool		M44_Equal( const M44f& A, const M44f& B );
const bool		M44_Equal( const M44f& A, const M44f& B, const float epsilon );


void			M44_SetTranslation( M44f *m, const V3f& T );
const V3f		M44_getTranslation( const M44f& m );
const V3f		M44_getScaling( const M44f& m );
const V3f		M44_TransformPoint( const M44f& m, const V3f& p );
const V3f		M44_TransformNormal( const M44f& m, const V3f& v );
//const V3f		M44_RotatePoint( const M44f& M, const V3f& P );



/// Builds a matrix which translates by (x, y, z).
const M44f		M44_buildTranslationMatrix( const V3f& _translation );

const Vector4	M44_TransformVector4( Mat4Arg M, Vec4Arg0 p );

void M44_TransformVector4_Array(
	const Vector4* input, const UINT _count, Mat4Arg _M,
	Vector4 *_output
);

/// Constructs a symmetric view frustum using Direct3D's clip-space conventions (NDC depth in range [0..+1]).
/// FoV_Y_radians - (full) vertical field of view, in radians (e.g. PI/2 radians or 90 degrees).
///
void M44_ProjectionAndInverse_D3D(
	M44f *__restrict projection_patrix_
	, M44f *__restrict inverse_projection_patrix_
	, float FoV_Y_radians
	, float aspect_ratio
	, float near_clip
	, float far_clip
	, bool reverse_depth = false
);


/*
-----------------------------------------------------------------------------
	REVERSE Z/DEPTH BUFFERING
-----------------------------------------------------------------------------
*/

void M44_ProjectionAndInverse_D3D_ReverseDepth(
	M44f *__restrict projection_patrix_
	, M44f *__restrict inverse_projection_patrix_
	, float FoV_Y_radians
	, float aspect_ratio
	, float near_clip
	, float far_clip
);
void M44_ProjectionAndInverse_D3D_ReverseDepth_InfiniteFarPlane(
	M44f *__restrict projection_patrix_
	, M44f *__restrict inverse_projection_patrix_
	, float FoV_Y_radians
	, float aspect_ratio
	, float near_clip
);



const M44f		M44_RotationX( float angle );
const M44f		M44_RotationY( float angle );
const M44f		M44_RotationZ( float angle );
const M44f		M44_RotationRollPitchYaw( float pitch, float yaw, float roll );
const M44f		M44_RotationRollPitchYawFromVector( const V4f& angles);
const M44f		M44_RotationNormal( const V4f& normalAxis, float angle );
const M44f		M44_RotationAxis( const V4f& axis, float angle );

const M44f		M44_FromQuaternion( Vec4Arg0 Q );
const M44f		M44_FromRotationMatrix( const M33f& m );
const M44f		M44_FromAxes( const V3f& axisX, const V3f& axisY, const V3f& axisZ );
const M44f		M44_NDC_to_TexCoords_with_Bias( const float bias = 0.0f );

/// Planes can be transformed as ordinary 4D vectors by the inverse-transpose of the matrix (transpose(inverse(M)) (and then they must be normalized).
mmINLINE void M44_TransformPlane_Array(
	const Vector4* __restrict input, const UINT _count, Mat4Arg _M,
	Vector4 *__restrict _output
);




const M44f M44_BuildView(
	const V3f& rightDirection,
	const V3f& lookDirection,
	const V3f& upDirection,
	const V3f& eyePosition
);








/*
=======================================================================
	GLOBAL CONSTANTS
=======================================================================
*/

mmGLOBALCONST M44f g_MM_Identity = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

#define V4_Zero		((const V4f&)g_MM_Zero)
#define V4_Forward	g_MM_Identity.r1	// Y
#define V4_Right	g_MM_Identity.r0	// X
#define V4_Up		g_MM_Identity.r2	// Z

#define V3_FORWARD	((const V3f&)V4_Forward)
#define V3_RIGHT	((const V3f&)V4_Right)
#define V3_UP		((const V3f&)V4_Up)


#define g_CardinalAxes		(g_MM_Identity.r)


/*
=======================================================================
	INLINE
=======================================================================
*/
#include <Base/Math/Matrix/M44f.inl>

/*
=======================================================================
	REFLECTION
=======================================================================
*/
#if MM_ENABLE_REFLECTION

#include <Base/Object/Reflection/Reflection.h>
#include <Base/Object/Reflection/StructDescriptor.h>

mxDECLARE_STRUCT(M44f);
mxDECLARE_POD_TYPE(M44f);

#endif // MM_ENABLE_REFLECTION

/*
==========================================================
	LOGGING
==========================================================
*/

#if MM_DEFINE_STREAM_OPERATORS

class ATextStream;

ATextStream & operator << ( ATextStream & log, const M44f& m );

#endif // MM_DEFINE_STREAM_OPERATORS


/*
==========================================================
	UNIT TESTING
==========================================================
*/
#if MX_DEVELOPER

void runUnitTests_M44f();

#endif // MX_DEVELOPER
