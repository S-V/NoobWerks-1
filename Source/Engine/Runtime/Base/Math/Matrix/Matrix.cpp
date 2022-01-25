#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Math/Matrix/M44f.h>


/*
Provide functions to build both left-handed and right-handed projection matrices (both orthographic and perspective projections).
Provide functions for building view matrices using LookAt for both left-handed and right-handed coordinate systems.
This would make this library useful for both OpenGL and DirectX applications (or OpenGL applications with a left-handed coordinate system!).
*/

/// http://stackoverflow.com/a/9614511/4232223
/// based on Laplace Expansion Theorem: http://www.geometrictools.com/Documentation/LaplaceExpansionTheorem.pdf
///
const bool M44_TryInverse( M44f & m )
{
	float s0 = m.m[0][0] * m.m[1][1] - m.m[1][0] * m.m[0][1];
	float s1 = m.m[0][0] * m.m[1][2] - m.m[1][0] * m.m[0][2];
	float s2 = m.m[0][0] * m.m[1][3] - m.m[1][0] * m.m[0][3];
	float s3 = m.m[0][1] * m.m[1][2] - m.m[1][1] * m.m[0][2];
	float s4 = m.m[0][1] * m.m[1][3] - m.m[1][1] * m.m[0][3];
	float s5 = m.m[0][2] * m.m[1][3] - m.m[1][2] * m.m[0][3];

	float c5 = m.m[2][2] * m.m[3][3] - m.m[3][2] * m.m[2][3];
	float c4 = m.m[2][1] * m.m[3][3] - m.m[3][1] * m.m[2][3];
	float c3 = m.m[2][1] * m.m[3][2] - m.m[3][1] * m.m[2][2];
	float c2 = m.m[2][0] * m.m[3][3] - m.m[3][0] * m.m[2][3];
	float c1 = m.m[2][0] * m.m[3][2] - m.m[3][0] * m.m[2][2];
	float c0 = m.m[2][0] * m.m[3][1] - m.m[3][0] * m.m[2][1];

	// Should check for 0 determinant
	float det = (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);
	if( det == 0 ) {
		return false;
	}

	float invdet = 1.0 / det;

	M44f result;

	result.m[0][0] = ( m.m[1][1] * c5 - m.m[1][2] * c4 + m.m[1][3] * c3) * invdet;
	result.m[0][1] = (-m.m[0][1] * c5 + m.m[0][2] * c4 - m.m[0][3] * c3) * invdet;
	result.m[0][2] = ( m.m[3][1] * s5 - m.m[3][2] * s4 + m.m[3][3] * s3) * invdet;
	result.m[0][3] = (-m.m[2][1] * s5 + m.m[2][2] * s4 - m.m[2][3] * s3) * invdet;

	result.m[1][0] = (-m.m[1][0] * c5 + m.m[1][2] * c2 - m.m[1][3] * c1) * invdet;
	result.m[1][1] = ( m.m[0][0] * c5 - m.m[0][2] * c2 + m.m[0][3] * c1) * invdet;
	result.m[1][2] = (-m.m[3][0] * s5 + m.m[3][2] * s2 - m.m[3][3] * s1) * invdet;
	result.m[1][3] = ( m.m[2][0] * s5 - m.m[2][2] * s2 + m.m[2][3] * s1) * invdet;

	result.m[2][0] = ( m.m[1][0] * c4 - m.m[1][1] * c2 + m.m[1][3] * c0) * invdet;
	result.m[2][1] = (-m.m[0][0] * c4 + m.m[0][1] * c2 - m.m[0][3] * c0) * invdet;
	result.m[2][2] = ( m.m[3][0] * s4 - m.m[3][1] * s2 + m.m[3][3] * s0) * invdet;
	result.m[2][3] = (-m.m[2][0] * s4 + m.m[2][1] * s2 - m.m[2][3] * s0) * invdet;

	result.m[3][0] = (-m.m[1][0] * c3 + m.m[1][1] * c1 - m.m[1][2] * c0) * invdet;
	result.m[3][1] = ( m.m[0][0] * c3 - m.m[0][1] * c1 + m.m[0][2] * c0) * invdet;
	result.m[3][2] = (-m.m[3][0] * s3 + m.m[3][1] * s1 - m.m[3][2] * s0) * invdet;
	result.m[3][3] = ( m.m[2][0] * s3 - m.m[2][1] * s1 + m.m[2][2] * s0) * invdet;

	m = result;

	return true;
}

const M44f M44_Inverse( const M44f& m )
{
	float s0 = m.m[0][0] * m.m[1][1] - m.m[1][0] * m.m[0][1];
	float s1 = m.m[0][0] * m.m[1][2] - m.m[1][0] * m.m[0][2];
	float s2 = m.m[0][0] * m.m[1][3] - m.m[1][0] * m.m[0][3];
	float s3 = m.m[0][1] * m.m[1][2] - m.m[1][1] * m.m[0][2];
	float s4 = m.m[0][1] * m.m[1][3] - m.m[1][1] * m.m[0][3];
	float s5 = m.m[0][2] * m.m[1][3] - m.m[1][2] * m.m[0][3];

	float c5 = m.m[2][2] * m.m[3][3] - m.m[3][2] * m.m[2][3];
	float c4 = m.m[2][1] * m.m[3][3] - m.m[3][1] * m.m[2][3];
	float c3 = m.m[2][1] * m.m[3][2] - m.m[3][1] * m.m[2][2];
	float c2 = m.m[2][0] * m.m[3][3] - m.m[3][0] * m.m[2][3];
	float c1 = m.m[2][0] * m.m[3][2] - m.m[3][0] * m.m[2][2];
	float c0 = m.m[2][0] * m.m[3][1] - m.m[3][0] * m.m[2][1];

	// Should check for 0 determinant
	float det = (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);
	mxASSERT(det != 0);
	float invdet = 1.0 / det;

	M44f result;

	result.m[0][0] = ( m.m[1][1] * c5 - m.m[1][2] * c4 + m.m[1][3] * c3) * invdet;
	result.m[0][1] = (-m.m[0][1] * c5 + m.m[0][2] * c4 - m.m[0][3] * c3) * invdet;
	result.m[0][2] = ( m.m[3][1] * s5 - m.m[3][2] * s4 + m.m[3][3] * s3) * invdet;
	result.m[0][3] = (-m.m[2][1] * s5 + m.m[2][2] * s4 - m.m[2][3] * s3) * invdet;

	result.m[1][0] = (-m.m[1][0] * c5 + m.m[1][2] * c2 - m.m[1][3] * c1) * invdet;
	result.m[1][1] = ( m.m[0][0] * c5 - m.m[0][2] * c2 + m.m[0][3] * c1) * invdet;
	result.m[1][2] = (-m.m[3][0] * s5 + m.m[3][2] * s2 - m.m[3][3] * s1) * invdet;
	result.m[1][3] = ( m.m[2][0] * s5 - m.m[2][2] * s2 + m.m[2][3] * s1) * invdet;

	result.m[2][0] = ( m.m[1][0] * c4 - m.m[1][1] * c2 + m.m[1][3] * c0) * invdet;
	result.m[2][1] = (-m.m[0][0] * c4 + m.m[0][1] * c2 - m.m[0][3] * c0) * invdet;
	result.m[2][2] = ( m.m[3][0] * s4 - m.m[3][1] * s2 + m.m[3][3] * s0) * invdet;
	result.m[2][3] = (-m.m[2][0] * s4 + m.m[2][1] * s2 - m.m[2][3] * s0) * invdet;

	result.m[3][0] = (-m.m[1][0] * c3 + m.m[1][1] * c1 - m.m[1][2] * c0) * invdet;
	result.m[3][1] = ( m.m[0][0] * c3 - m.m[0][1] * c1 + m.m[0][2] * c0) * invdet;
	result.m[3][2] = (-m.m[3][0] * s3 + m.m[3][1] * s1 - m.m[3][2] * s0) * invdet;
	result.m[3][3] = ( m.m[2][0] * s3 - m.m[2][1] * s1 + m.m[2][2] * s0) * invdet;

	return result;
}

/// Builds a right-handed look-at (or view) matrix.
/// A view transformation matrix transforms world coordinates into the camera or view space.
/// Parameters:
/// 'eyePosition' - camera position
/// 'cameraTarget' - camera look-at target
/// 'cameraUpVector' - camera UP vector
///
const M44f M44_LookAt( const V3f& eyePosition, const V3f& cameraTarget, const V3f& cameraUpVector )
{
    mxASSERT(!V3_Equal(cameraUpVector, V3_Zero()));
    mxASSERT(!V3_IsInfinite(cameraUpVector));
	mxASSERT(V3_IsNormalized(cameraUpVector));

	V3f	axisX;	// camera right vector
	V3f	axisY;	// camera forward vector
	V3f	axisZ;	// camera up vector

	axisY = V3_Normalized( V3_Subtract( cameraTarget, eyePosition ) );
	axisX = V3_Normalized( V3_Cross( axisY, cameraUpVector ) );
	axisZ = V3_Normalized( V3_Cross( axisX, axisY ) );

	float Tx = -V3_Dot( axisX, eyePosition );
	float Ty = -V3_Dot( axisY, eyePosition );
	float Tz = -V3_Dot( axisZ, eyePosition );

	M44f	result;
	result.r0 = Vector4_Set( axisX.x,	axisY.x,	axisZ.x,	0.0f );
	result.r1 = Vector4_Set( axisX.y,	axisY.y,	axisZ.y,	0.0f );
	result.r2 = Vector4_Set( axisX.z,	axisY.z,	axisZ.z,	0.0f );
	result.r3 = Vector4_Set( Tx,			 Ty,		 Tz,	1.0f );
	return result;
}

/// Builds a right-handed look-at (or view) matrix.
/// View transformation matrix transforms world coordinates into camera or view space.
/// 'eyePosition' - camera position
/// 'lookDirection' - camera look direction
/// 'upDirection' - camera UP vector
const M44f M44_LookTo( const V3f& eyePosition, const V3f& lookDirection, const V3f& upDirection )
{
    mxASSERT(!V3_Equal(lookDirection, V3_Zero()));
    mxASSERT(!V3_IsInfinite(lookDirection));
    mxASSERT(!V3_Equal(upDirection, V3_Zero()));
    mxASSERT(!V3_IsInfinite(upDirection));
	mxASSERT(V3_IsNormalized(lookDirection));
	mxASSERT(V3_IsNormalized(upDirection));

	V3f	axisX;	// camera right vector
	V3f	axisY;	// camera forward vector
	V3f	axisZ;	// camera up vector

	axisY = V3_Normalized( lookDirection );

	axisX = V3_Cross( axisY, upDirection );
	axisX = V3_Normalized( axisX );

	axisZ = V3_Cross( axisX, axisY );
	axisZ = V3_Normalized( axisZ );

	M44f	cameraWorldMatrix;
	cameraWorldMatrix.r0 = Vector4_Set( axisX, 0 );
	cameraWorldMatrix.r1 = Vector4_Set( axisY, 0 );
	cameraWorldMatrix.r2 = Vector4_Set( axisZ, 0 );
	cameraWorldMatrix.r3 = Vector4_Set( eyePosition, 1 );

	M44f	result;
	result = M44_OrthoInverse( cameraWorldMatrix );
	return result;
}

/// Constructs a symmetric view frustum using OpenGL's clip-space conventions (NDC depth in range [-1..+1]).
/// FoV_Y_radians - (full) vertical field of view, in radians (e.g. PI/2 radians or 90 degrees).
///
const M44f M44_PerspectiveOGL( float FoV_Y_radians, float aspect_ratio, float near_clip, float far_clip )
{
	float tan_half_FoV = tan( FoV_Y_radians * 0.5f );
	float scaleX = 1.0 / (aspect_ratio * tan_half_FoV);
	float scaleY = 1.0 / tan_half_FoV;

	float projA = (far_clip + near_clip) / (far_clip - near_clip);
	float projB = (-2.0f * near_clip * far_clip) / (far_clip - near_clip);

	M44f	result;
	result.r0 = Vector4_Set(
		scaleX,
		0.0f,
		0.0f,
		0.0f
	);
	result.r1 = Vector4_Set(
		0.0f,
		0.0f,
		projA,
		1.0f
	);
	result.r2 = Vector4_Set(
		0.0f,
		scaleY,
		0.0f,
		0.0f
	);
	result.r3 = Vector4_Set(
		0.0f,
		0.0f,
		projB,
		0.0f
	);
	return result;
}

/// Constructs a symmetric view frustum using Direct3D's clip-space conventions (NDC depth in range [0..+1]).
/// FoV_Y_radians - (full) vertical field of view, in radians (e.g. PI/2 radians or 90 degrees).
///
const M44f M44_PerspectiveD3D( float FoV_Y_radians, float aspect_ratio, float near_clip, float far_clip )
{
	mxASSERT(aspect_ratio != 0);
	mxASSERT(near_clip != far_clip);

	float tan_half_FoV = tan( FoV_Y_radians * 0.5f );
	float scaleY = 1.0 / tan_half_FoV;
	float scaleX = scaleY / aspect_ratio;
	float projA = far_clip / (far_clip - near_clip);
	float projB = -near_clip * projA;

	M44f	result;
	result.r0 = Vector4_Set(
		scaleX,
		0.0f,
		0.0f,
		0.0f
	);
	result.r1 = Vector4_Set(
		0.0f,
		0.0f,
		projA,
		1.0f
	);
	result.r2 = Vector4_Set(
		0.0f,
		scaleY,
		0.0f,
		0.0f
	);
	result.r3 = Vector4_Set(
		0.0f,
		0.0f,
		projB,
		0.0f
	);
	return result;
}
/*
Multiplication by our projection matrix (NOTE: z corresponds to height, y - to depth):

            | H   0   0   0 |
[x y z w] * | 0   0   A   1 | = | (x*H)  (z*V)  (y*A + w*B) (y) |
            | 0   V   0   0 |
            | 0   0   B   0 |
*/
/*
Multiplication by inverse projection matrix (P^-1):

            | 1/H   0   0    0  |
[x y z w] * |  0    0  1/V   0  | = | (x/H)   (w)   (y/V)  (z/B - w*A/B) |
            |  0    0   0   1/B |
            |  0    1   0  -A/B |
*/
mxNO_ALIAS void M44_ProjectionAndInverse_D3D(
	M44f *__restrict projection_patrix_
	, M44f *__restrict inverse_projection_patrix_
	, float FoV_Y_radians
	, float aspect_ratio
	, float near_clip
	, float far_clip
	, bool reverse_depth
)
{
	mxASSERT(FoV_Y_radians > 0);
	mxASSERT(aspect_ratio > 0);
	mxASSERT(near_clip > 0.0f);
	mxASSERT(near_clip != far_clip);

	const double tan_half_FoV = tan( (double)FoV_Y_radians * 0.5 );
	const double V = 1.0 / tan_half_FoV;
	const double H = V / aspect_ratio;
	const double A = far_clip / (far_clip - near_clip);
	const double B = -near_clip * A;

	projection_patrix_->v0 = V4f::set( H, 0, 0, 0 );
	projection_patrix_->v1 = V4f::set( 0, 0, A, 1 );
	projection_patrix_->v2 = V4f::set( 0, V, 0, 0 );
	projection_patrix_->v3 = V4f::set( 0, 0, B, 0 );

	//@todo: rearrange using algebraic rules for better precision
	inverse_projection_patrix_->v0 = V4f::set( 1/H, 0,  0,  0  );
	inverse_projection_patrix_->v1 = V4f::set(  0,  0, 1/V, 0  );
	inverse_projection_patrix_->v2 = V4f::set(  0,  0,  0, 1/B );
	inverse_projection_patrix_->v3 = V4f::set(  0,  1,  0,-A/B );
}

//
// Reverse Z Cheatsheet
// 2018-01-01 
// https://thxforthefish.com/posts/reverse_z/
//
// Multiplication by our reversed projection matrix
// (where z corresponds to height, y - to depth):
// 
//             | H   0   0   0 |
// [x y z w] * | 0   0   A   1 | = | (x*H)  (z*V)  (y*A + w*B) (y) |
//             | 0   V   0   0 |
//             | 0   0   B   0 |
//
// Znear is mapped to 1
// Zfar  is mapped to 0
//
// A = -near / (far - near)
// B = -far * A
// 
// With infinite far plane:
// A = 0
// B = near
//

void M44_ProjectionAndInverse_D3D_ReverseDepth(
	M44f *__restrict projection_patrix_
	, M44f *__restrict inverse_projection_patrix_
	, float FoV_Y_radians
	, float aspect_ratio
	, float near_clip
	, float far_clip
)
{
	mxASSERT(FoV_Y_radians > 0);
	mxASSERT(aspect_ratio > 0);
	mxASSERT(near_clip >= 0.0f);
	mxASSERT(near_clip < far_clip);

	const double tan_half_FoV = tan( (double)FoV_Y_radians * 0.5 );
	const double V = 1 / tan_half_FoV;
	const double H = V / aspect_ratio;
	const double A = -near_clip / (far_clip - near_clip);
	const double B = -far_clip * A;

	//
	projection_patrix_->v0 = V4f::set( H, 0, 0, 0 );
	projection_patrix_->v1 = V4f::set( 0, 0, A, 1 );
	projection_patrix_->v2 = V4f::set( 0, V, 0, 0 );
	projection_patrix_->v3 = V4f::set( 0, 0, B, 0 );

	//
	inverse_projection_patrix_->v0 = V4f::set( 1/H, 0,  0,  0  );
	inverse_projection_patrix_->v1 = V4f::set(  0,  0, 1/V, 0  );
	inverse_projection_patrix_->v2 = V4f::set(  0,  0,  0, 1/B );
	inverse_projection_patrix_->v3 = V4f::set(  0,  1,  0,  0  );
}

void M44_ProjectionAndInverse_D3D_ReverseDepth_InfiniteFarPlane(
	M44f *__restrict projection_patrix_
	, M44f *__restrict inverse_projection_patrix_
	, float FoV_Y_radians
	, float aspect_ratio
	, float near_clip
)
{
	mxASSERT(FoV_Y_radians > 0);
	mxASSERT(aspect_ratio > 0);
	mxASSERT(near_clip > 0.0f);

	const double tan_half_FoV = tan( (double)FoV_Y_radians * 0.5 );
	const double V = 1 / tan_half_FoV;
	const double H = V / aspect_ratio;
	const double A = 0;
	const double B = near_clip;

	//
	projection_patrix_->v0 = V4f::set( H, 0, 0, 0 );
	projection_patrix_->v1 = V4f::set( 0, 0, A, 1 );
	projection_patrix_->v2 = V4f::set( 0, V, 0, 0 );
	projection_patrix_->v3 = V4f::set( 0, 0, B, 0 );

	//
	inverse_projection_patrix_->v0 = V4f::set( 1/H, 0,  0,  0  );
	inverse_projection_patrix_->v1 = V4f::set(  0,  0, 1/V, 0  );
	inverse_projection_patrix_->v2 = V4f::set(  0,  0,  0, 1/B );
	inverse_projection_patrix_->v3 = V4f::set(  0,  1,  0,  0  );
}


// http://en.wikipedia.org/wiki/Orthographic_projection
#if 0
/// canonical D3D orhographic projection matrix
/*
	2/(r-l)      0            0           0
	0            2/(t-b)      0           0
	0            0            1/(zf-zn)   0
	(l+r)/(l-r)  (t+b)/(b-t)  zn/(zn-zf)  1
*/
const M44f M44_OrthographicD3D( float width, float height, float near_clip, float far_clip )
{
	mxASSERT(width > 0);
	mxASSERT(height > 0);
	mxASSERT(near_clip < far_clip);

	M44f	result;
	result.r0 = Vector4_Set(
		2.0f / width,		// X: [-W/2..+W/2] => [-1..+1]
		0.0f,
		0.0f,
		0.0f
	);
	result.r1 = Vector4_Set(
		0.0f,
		1.0f / (far_clip - near_clip),	// Y: [N..F] => [0..1]
		0.0f,
		0.0f
	);
	result.r2 = Vector4_Set(
		0.0f,
		0.0f,
		2.0f / height,		// Z: [-H/2..+H/2] => [-1..+1]
		0.0f
	);
	result.r3 = Vector4_Set(
		0.0f,
		near_clip / (near_clip - far_clip),
		0.0f,		
		1.0f
	);
	return result;
}
#else
/// Builds a customized, right-handed orthographic projection matrix.
/// adapted to my right-handed coordinate system
const M44f M44_OrthographicD3D( float width, float height, float near_clip, float far_clip )
{
	M44f	result;
	result.r0 = Vector4_Set(
		2.0f / width,		// X: [-W/2..+W/2] => [-1..+1]
		0.0f,
		0.0f,
		0.0f
	);
	result.r1 = Vector4_Set(
		0.0f,
		0.0f,
		1.0f / (far_clip - near_clip),	// Y: [N..F] => [0..1]
		0.0f
	);
	result.r2 = Vector4_Set(
		0.0f,
		2.0f / height,		// Z: [-H/2..+H/2] => [-1..+1]
		0.0f,		
		0.0f
	);
	result.r3 = Vector4_Set(
		0.0f,
		near_clip / (near_clip - far_clip),
		0.0f,		
		1.0f
	);
	return result;
}
#endif

/// Builds a customized, right-handed orthographic projection matrix.
/// Derivation:
/// In orthographic projection no perspective distortion occurs,
/// i.e. transformed positions are linear wrt to original positions:
/// x' = A * x + B.
/// The projection matrix should
/// transform X coordinate from [left, right] to [-1..+1]
/// transform Y coordinate from [bottom, top] to [-1..+1]
/// transform Z coordinate from [ near, far ] to [-1..+1] (OpenGL) or [0..1] (Direct3D).
///
/// After solving for A and B we have:
/// Map x from [l..r] to [-1..+1]:
///        2         r + l
/// x' = ----- * x - -----
///      r - l       r - l
///
/// Map y from [b..t] to [-1..+1]:
///        2         t + b
/// y' = ----- * y - -----
///      t - b       t - b
///
/// Map z from [n..f] to [0..1] (D3D):
///        1           n
/// z' = ----- * z + -----
///      f - n       n - f
///
const M44f M44_OrthographicD3D(
								   float left, float right,
								   float bottom, float top,
								   float near_clip, float far_clip
								   )
{
	mxASSERT(left < right);
	mxASSERT(bottom < top);
	mxASSERT(near_clip < far_clip);

	const float inv_R_minus_L = 1.f / (right - left);
	const float inv_T_minus_B = 1.f / (top - bottom);
	const float inv_F_minus_N = 1.f / (far_clip - near_clip);

	const float scaleX = 2.f * inv_R_minus_L;	// horizontal scale
	const float scaleZ = 2.f * inv_T_minus_B;	// vertical scale
	const float offsetX = -(left + right) * inv_R_minus_L;	// horizontal offset
	const float offsetZ = -(top + bottom) * inv_T_minus_B;	// vertical offset
	const float A = inv_F_minus_N;	// depth scale
	const float B = -near_clip * inv_F_minus_N;	// depth offset

#define _ 0.0f

	M44f	result;
	result.r0 = Vector4_Set( scaleX,   _,        _,  _ );
	result.r1 = Vector4_Set( _,        _,        A,  _ );
	result.r2 = Vector4_Set( _,        scaleZ,   _,  _ );
	result.r3 = Vector4_Set( offsetX,  offsetZ,  B,  1 );

#undef _

/*
In our coordinate system, the projection matrix
transforms X coordinate from [left, right] => [-1..+1]
transforms Z coordinate from [bottom, top] => [-1..+1]
transforms Y coordinate from [ near, far ] => [0..1] (Direct3D) or [-1..+1] (OpenGL)

Multiplication by our projection matrix
(NOTE: z corresponds to height, y - to depth):

| sx  0   0   0 |   | x |   | x * sx + ox   |
| 0   0   A   0 | * | y | = | z * sy + oy   |
| 0   sy  0   0 |   | z |   | y * A + w * B |  ==(Ay+B)
| ox  oy  B   1 |   |w=1|   | w             |  ==(1)
*/
	return result;
}

#if 0
// Creates a transformation matrix from my engine's space to OpenGL space:
// rotates PI/2 clockwise around X axis.
void MakeM44_MySpace_To_OpenGL( glm::mat4 & M )// right-handed to left-handed (OpenGL)
{
	/*
	Assuming column-major scheme...
	1  0  0  0
	0  0  1  0
	0 -1  0  0
	0  0  0  1
	...This matrix will set the y value to the z value, and the z value to -y.
	*/
	M[0][0] =  1.0f;	M[0][1] =  0.0f;	M[0][2] =  0.0f;	M[0][3] =  0.0f;
	M[1][0] =  0.0f;	M[1][1] =  0.0f;	M[1][2] =  1.0f;	M[1][3] =  0.0f;
	M[2][0] =  0.0f;	M[2][1] = -1.0f;	M[2][2] =  0.0f;	M[2][3] =  0.0f;
	M[3][0] =  0.0f;	M[3][1] =  0.0f;	M[3][2] =  0.0f;	M[3][3] =  1.0f;
}

// Creates a transformation matrix from my engine's space to OpenGL space:
// rotates PI/2 counterclockwise around
void MakeM44_OpenGL_To_MySpace( glm::mat4 & M )
{
	/*
	Assuming column-major scheme...
	1  0  0  0
	0  0 -1  0
	0  1  0  0
	0  0  0  1
	...This matrix will set the y value to the -z value, and the z value to y.
	*/
	M[0][0] =  1.0f;	M[0][1] =  0.0f;	M[0][2] =  0.0f;	M[0][3] =  0.0f;
	M[1][0] =  0.0f;	M[1][1] =  0.0f;	M[1][2] = -1.0f;	M[1][3] =  0.0f;
	M[2][0] =  0.0f;	M[2][1] =  1.0f;	M[2][2] =  0.0f;	M[2][3] =  0.0f;
	M[3][0] =  0.0f;	M[3][1] =  0.0f;	M[3][2] =  0.0f;	M[3][3] =  1.0f;
}

const M44f M44_PerspectiveOGL( float fovyRadians, float aspect, float zNear, float zFar )
{
	M44f	result;
	float tanHalfFovy = tan(fovyRadians / float(2));
	result.c[0] = Vector4_Set( 1.0f / (aspect * tanHalfFovy), 0.0f, 0.0f, 0.0f );
	result.c[1] = Vector4_Set( 0.0f, 1.0f / tanHalfFovy, 0.0f, 0.0f );
	result.c[2] = Vector4_Set( 0.0f, 0.0f, -(zFar + zNear) / (zFar - zNear), -1.0f );
	result.c[3] = Vector4_Set( 0.0f, 0.0f, -(2.0f * zFar * zNear) / (zFar - zNear), 0.0f );
	return result;
}
// This code snippet implements:  D3DXMatrixPerspectiveFovLH : http://msdn.microsoft.com/en-us/library/windows/desktop/bb205350(v=vs.85).aspx
inline void vp_matrixPerspectiveFovLH(float matrix[16],float fov,float aspectRatio,float zn,float zf)
{
	float yScale = 1/tanf(fov/2);
	float xScale = yScale / aspectRatio;

	matrix[0]  = xScale;
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;

	matrix[4] = 0;
	matrix[5]  = yScale;
	matrix[6] = 0;
	matrix[7] = 0;

	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = zf / (zf-zn);
	matrix[11] = 1;

	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = -zn*zf/(zf-zn);
	matrix[15] = 0;
}
const M44f M44_PerspectiveD3D( float fovyRadians, float aspect, float zNear, float zFar )
{
	const float H = 1.0f / Tan( fovyRadians * 0.5f );
	const float V = H / aspect;
	const float q = zFar / (zFar-zNear);

	M44f	result;

	result.m00 = H;		result.m01 = 0.0f;		result.m02 = 0.0f;	result.m03 = 0.0f;
	result.m10 = 0.0f;	result.m11 = q;			result.m12 = 0.0f;	result.m13 = 1.0f;
	result.m20 = 0.0f;	result.m21 = 0.0f;		result.m22 = V;		result.m23 = 0.0f;
	result.m30 = 0.0f;	result.m31 = -zNear*q;	result.m32 = 0.0f;	result.m33 = 0.0f;

	//result.a[0 ]	= xScale;
	//result.a[1 ]	= 0.0f;
	//result.a[2 ]	= 0.0f;
	//result.a[3 ]	= 0.0f;

	//result.a[4 ] 	= 0.0f;
	//result.a[5 ] 	= 0.0f;
	//result.a[6 ] 	= q;
	//result.a[7 ] 	= 1.0f;

	//result.a[8 ]	= 0.0f;
	//result.a[9 ]	= yScale;
	//result.a[10]	= 0.0f;
	//result.a[11]	= 0.0f;

	//result.a[12] 	= 0.0f;
	//result.a[13] 	= 0.0f;
	//result.a[14] 	= -zNear*q;
	//result.a[15] 	= 0.0f;

	return result;
}
#endif

const M44f M44_BuildTS( const V3f& _translation, const V3f& scaling )
{
	M44f	result;
	result.r0 = Vector4_Set( scaling.x, 0.0f, 0.0f, 0.0f );
	result.r1 = Vector4_Set( 0.0f, scaling.y, 0.0f, 0.0f );
	result.r2 = Vector4_Set( 0.0f, 0.0f, scaling.z, 0.0f );
	result.r3 = Vector4_Set( _translation.x, _translation.y, _translation.z, 1.0f );
	return result;
}

const M44f M44_BuildTRS( const V3f& translation, const M33f& rotation, const V3f& scaling )
{
	M44f	result;
	result.r0 = Vector4_Set( rotation.r0.x * scaling.x, rotation.r0.y,             rotation.r0.z,             0.0f );
	result.r1 = Vector4_Set( rotation.r1.x,             rotation.r1.y * scaling.y, rotation.r1.z,             0.0f );
	result.r2 = Vector4_Set( rotation.r2.x,             rotation.r2.y,             rotation.r2.z * scaling.z, 0.0f );
	result.r3 = Vector4_Set( translation.x,             translation.y,             translation.z,             1.0f );
	return result;
}

#if 0
const M44f M44_BuildTRS( const V3f& translation, const V4f& rotation, const V3f& scaling )
{
	M44f	RotM = M44_FromQuaternion( rotation );

	M44f	result;
	result.r0 = Vector4_Set( RotM.v0.x * scaling.x, RotM.v0.y,             RotM.v0.z,             0.0f );
	result.r1 = Vector4_Set( RotM.v1.x,             RotM.v1.y * scaling.y, RotM.v1.z,             0.0f );
	result.r2 = Vector4_Set( RotM.v2.x,             RotM.v2.y,             RotM.v2.z * scaling.z, 0.0f );
	result.r3 = Vector4_Set( translation.x, translation.y, translation.z, 1.0f );
	return result;
}
#endif
const M44f M44_BuildTS( const V3f& _translation, const float _scaling )
{
	M44f	result;
	result.r0 = Vector4_Set( _scaling, 0.0f, 0.0f, 0.0f );
	result.r1 = Vector4_Set( 0.0f, _scaling, 0.0f, 0.0f );
	result.r2 = Vector4_Set( 0.0f, 0.0f, _scaling, 0.0f );
	result.r3 = Vector4_Set( _translation.x, _translation.y, _translation.z, 1.0f );
	return result;
}

#if 0
const M44f M44_BuildTR( const V3f& translation, const V4f& rotation )
{
	M44f	result;
	result = M44_FromQuaternion( rotation );
	result.r3 = Vector4_Set( translation.x, translation.y, translation.z, 1.0f );
	return result;
}
#endif

// exact compare, no epsilon
const bool M44_Equal( const M44f& A, const M44f& B )
{
	const float* pA = reinterpret_cast< const float* >( &A );
	const float* pB = reinterpret_cast< const float* >( &B );

	for( int i = 0; i < 16; i++ )
	{
		if( pA[i] != pB[i] ) {
			return false;
		}
	}
	return true;
}
// compare with epsilon
const bool M44_Equal( const M44f& A, const M44f& B, const float epsilon )
{
	const float* pA = reinterpret_cast< const float* >( &A );
	const float* pB = reinterpret_cast< const float* >( &B );

	for( int i = 0; i < 16; i++ )
	{
		if( fabs( pA[i] - pB[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

/// [ X, Y, Z, W ] => projection transform => homogeneous divide => [Px, Py, Pz, W ]
const V4f M44_Project( const M44f& m, const V4f& p )
{
	V4f pointH = M44_Transform( m, p );
	float invW = mmRcp( pointH.w );
	V4f	result;
	result.x = pointH.x * invW;
	result.y = pointH.y * invW;
	result.z = pointH.z * invW;
	result.w = pointH.w;
	return result;
}

/// Build a matrix which rotates around the X axis.
const M44f M44_RotationX( float angle )
{
	M44f M;

	FLOAT fSinAngle;
	FLOAT fCosAngle;
	mmSinCos(angle, fSinAngle, fCosAngle);

	M.m[0][0] = 1.0f;
	M.m[0][1] = 0.0f;
	M.m[0][2] = 0.0f;
	M.m[0][3] = 0.0f;

	M.m[1][0] = 0.0f;
	M.m[1][1] = fCosAngle;
	M.m[1][2] = fSinAngle;
	M.m[1][3] = 0.0f;

	M.m[2][0] = 0.0f;
	M.m[2][1] = -fSinAngle;
	M.m[2][2] = fCosAngle;
	M.m[2][3] = 0.0f;

	M.m[3][0] = 0.0f;
	M.m[3][1] = 0.0f;
	M.m[3][2] = 0.0f;
	M.m[3][3] = 1.0f;
	return M;
}

/// Build a matrix which rotates around the Y axis.
const M44f M44_RotationY( float angle )
{
	M44f M;

	FLOAT fSinAngle;
	FLOAT fCosAngle;
	mmSinCos(angle, fSinAngle, fCosAngle);

    M.m[0][0] = fCosAngle;
    M.m[0][1] = 0.0f;
    M.m[0][2] = -fSinAngle;
    M.m[0][3] = 0.0f;

    M.m[1][0] = 0.0f;
    M.m[1][1] = 1.0f;
    M.m[1][2] = 0.0f;
    M.m[1][3] = 0.0f;

    M.m[2][0] = fSinAngle;
    M.m[2][1] = 0.0f;
    M.m[2][2] = fCosAngle;
    M.m[2][3] = 0.0f;

    M.m[3][0] = 0.0f;
    M.m[3][1] = 0.0f;
    M.m[3][2] = 0.0f;
    M.m[3][3] = 1.0f;
    return M;
}

/// Build a matrix which rotates around the Z axis.
const M44f M44_RotationZ( float angle )
{
	M44f M;

	FLOAT fSinAngle;
	FLOAT fCosAngle;
	mmSinCos(angle, fSinAngle, fCosAngle);

	M.m[0][0] = fCosAngle;
	M.m[0][1] = fSinAngle;
	M.m[0][2] = 0.0f;
	M.m[0][3] = 0.0f;

	M.m[1][0] = -fSinAngle;
	M.m[1][1] = fCosAngle;
	M.m[1][2] = 0.0f;
	M.m[1][3] = 0.0f;

	M.m[2][0] = 0.0f;
	M.m[2][1] = 0.0f;
	M.m[2][2] = 1.0f;
	M.m[2][3] = 0.0f;

	M.m[3][0] = 0.0f;
	M.m[3][1] = 0.0f;
	M.m[3][2] = 0.0f;
	M.m[3][3] = 1.0f;
	return M;
}

/// Yaw around the Z axis, a pitch around the X axis, and a roll around the Y axis.

/// Build a matrix which rotates around an arbitrary axis.

mxUNDONE
//const M44f	M44_RotationRollPitchYaw( float pitch, float yaw, float roll );
//const M44f	M44_RotationRollPitchYawFromVector( const V4f& angles);
//const M44f	M44_RotationNormal( const V4f& normalAxis, float angle )
//{
//
//}
//const M44f	M44_RotationAxis( const V4f& axis, float angle );

/// Builds a rotation matrix from the given (unit-length) quaternion.
const M44f M44_FromQuaternion( Vec4Arg0 Q )
{
	const M33f	rotationMatrix = M33_FromQuaternion( Q );
	return M44_FromRotationMatrix( rotationMatrix );
}

const M44f M44_FromRotationMatrix( const M33f& m )
{
	M44f	result;
	result.r0 = Vector4_Set( m.r0.x, m.r0.y, m.r0.z, 0.0f );
	result.r1 = Vector4_Set( m.r1.x, m.r1.y, m.r1.z, 0.0f );
	result.r2 = Vector4_Set( m.r2.x, m.r2.y, m.r2.z, 0.0f );
	result.r3 = g_MM_Identity.r3;	// {0,0,0,1}
	return result;
}

const M44f M44_FromAxes( const V3f& axisX, const V3f& axisY, const V3f& axisZ )
{
	M44f	result;
	result.r0 = Vector4_Set( axisX.x, axisX.y, axisX.z, 0.0f );
	result.r1 = Vector4_Set( axisY.x, axisY.y, axisY.z, 0.0f );
	result.r2 = Vector4_Set( axisZ.x, axisZ.y, axisZ.z, 0.0f );
	result.r3 = g_MM_Identity.r3;	// {0,0,0,1}
	return result;
}

const M44f M44_From_Float3x4( const M34f& m )
{
	M44f	result;
	result.r0 = Vector4_Set( m.v[0].x, m.v[1].x, m.v[2].x, 0.0f );
	result.r1 = Vector4_Set( m.v[0].y, m.v[1].y, m.v[2].y, 0.0f );
	result.r2 = Vector4_Set( m.v[0].z, m.v[1].z, m.v[2].z, 0.0f );
	result.r3 = Vector4_Set( m.v[0].w, m.v[1].w, m.v[2].w, 1.0f );
	return result;
}

/// Constructs a scale/offset/bias matrix with a custom bias,
/// which transforms from [-1,1] post-projection space to [0,1] UV space
const M44f M44_NDC_to_TexCoords_with_Bias( const float bias )
{
	bool bOpenGL = false;
	float flipY = bOpenGL ? 0.5f : -0.5f;
	M44f textureMatrix;
	textureMatrix.r[0] = Vector4_Set( 0.5f,  0.0f,  0.0f,  0.0f );
	textureMatrix.r[1] = Vector4_Set( 0.0f,  flipY, 0.0f,  0.0f );
	textureMatrix.r[2] = Vector4_Set( 0.0f,  0.0f,  1.0f,  0.0f );
	textureMatrix.r[3] = Vector4_Set( 0.5f,  0.5f,  bias,  1.0f );
/*
//		The scale matrix to go from post-perspective space into texture space ([0,1]^2)
//		in D3D:
//		D3DXMATRIX Clip2Tex = {
//			0.5,    0,    0,   0,
//			0,	   -0.5,  0,   0,
//			0,     0,     1,   0,
//			0.5,   0.5,   0,   1
//		};
//
//             | 0.5   0   0   0 |
// [x y z w] * |  0  -0.5  0   0 | = | (x * 0.5 + 0.5)  (y *-0.5 + 0.5)  (z)  (w) |
//             |  0    0   1   0 |
//             | 0.5  0.5  0   1 |
//
// Because my HLSL shaders use row-major matrices with post-multiplication,
// we must transpose all matrices when passing them to shaders:
// 
// | 0.5   0   0  0.5 |   |x|   | x * 0.5 + 0.5 |
// |  0  -0.5  0  0.5 | * |y| = | y *-0.5 + 0.5 |
// |  0    0   1   0  |   |z|   | z             |
// |  0    0   0   1  |   |w|   | w             |
*/
	return textureMatrix;
}

const M33f M33_Identity()
{
	M33f	result;
	result.r0 = V3_Set( 1.0f, 0.0f, 0.0f );
	result.r1 = V3_Set( 0.0f, 1.0f, 0.0f );
	result.r2 = V3_Set( 0.0f, 0.0f, 1.0f );
	return result;
}
const M33f M33_Transpose( const M33f& m )
{
	M33f	result;
	result.r0 = V3_Set( m.r0.x, m.r1.x, m.r2.x );
	result.r1 = V3_Set( m.r0.y, m.r1.y, m.r2.y );
	result.r2 = V3_Set( m.r0.z, m.r1.z, m.r2.z );
	return result;
}

const bool M33_inverse( M33f & m_ )
{
	mxSTOLEN("Doom 3's idLib");
	// 18+3+9 = 30 multiplications
	//			 1 division
	M33f	inverse;
	double	det, invDet;

	inverse[0][0] = m_.m[1][1] * m_.m[2][2] - m_.m[1][2] * m_.m[2][1];
	inverse[1][0] = m_.m[1][2] * m_.m[2][0] - m_.m[1][0] * m_.m[2][2];
	inverse[2][0] = m_.m[1][0] * m_.m[2][1] - m_.m[1][1] * m_.m[2][0];

	det = m_.m[0][0] * inverse[0][0] + m_.m[0][1] * inverse[1][0] + m_.m[0][2] * inverse[2][0];

	if ( mmAbs( det ) < MATRIX_INVERSE_EPSILON ) {
		return false;
	}

	invDet = 1.0f / det;

	inverse[0][1] = m_.m[0][2] * m_.m[2][1] - m_.m[0][1] * m_.m[2][2];
	inverse[0][2] = m_.m[0][1] * m_.m[1][2] - m_.m[0][2] * m_.m[1][1];
	inverse[1][1] = m_.m[0][0] * m_.m[2][2] - m_.m[0][2] * m_.m[2][0];
	inverse[1][2] = m_.m[0][2] * m_.m[1][0] - m_.m[0][0] * m_.m[1][2];
	inverse[2][1] = m_.m[0][1] * m_.m[2][0] - m_.m[0][0] * m_.m[2][1];
	inverse[2][2] = m_.m[0][0] * m_.m[1][1] - m_.m[0][1] * m_.m[1][0];

	m_.m[0][0] = inverse[0][0] * invDet;
	m_.m[0][1] = inverse[0][1] * invDet;
	m_.m[0][2] = inverse[0][2] * invDet;

	m_.m[1][0] = inverse[1][0] * invDet;
	m_.m[1][1] = inverse[1][1] * invDet;
	m_.m[1][2] = inverse[1][2] * invDet;

	m_.m[2][0] = inverse[2][0] * invDet;
	m_.m[2][1] = inverse[2][1] * invDet;
	m_.m[2][2] = inverse[2][2] * invDet;

	return true;
}

const M33f M33_Multiply( const M33f& a, const M33f& b )
{
	M33f	result;
	result.r0 = M33_Transform( b, a.r0 );
	result.r1 = M33_Transform( b, a.r1 );
	result.r2 = M33_Transform( b, a.r2 );
	return result;
}

const V3f M33_Transform( const M33f& m, const V3f& p )
{
	V3f	result;
	result.x = (m.r0.x * p.x) + (m.r1.x * p.y) + (m.r2.x * p.z);
	result.y = (m.r0.y * p.x) + (m.r1.y * p.y) + (m.r2.y * p.z);
	result.z = (m.r0.z * p.x) + (m.r1.z * p.y) + (m.r2.z * p.z);
	return result;
}

const double M33_determinant( const M33f& m )
{
	const double tmp00 = m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1];
	const double tmp10 = m.m[1][2] * m.m[2][0] - m.m[1][0] * m.m[2][2];
	const double tmp20 = m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0];

	const double m00 = m.m[0][0];
	const double m01 = m.m[0][1];
	const double m02 = m.m[0][2];

	return m00 * tmp00 + m01 * tmp10 + m02 * tmp20;
}

const bool M33_tryInverse( M33f & m, const float matrix_inverse_epsilon /*= MATRIX_INVERSE_EPSILON*/ )
{
	// 18+3+9 = 30 multiplications
	//			 1 division
	M33f	inverse;
	double det, invDet;

	inverse[0][0] = m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1];
	inverse[1][0] = m.m[1][2] * m.m[2][0] - m.m[1][0] * m.m[2][2];
	inverse[2][0] = m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0];

	det = m.m[0][0] * inverse[0][0] + m.m[0][1] * inverse[1][0] + m.m[0][2] * inverse[2][0];

	if ( mmAbs( det ) < matrix_inverse_epsilon ) {
		return false;
	}

	invDet = 1.0f / det;

	inverse[0][1] = m.m[0][2] * m.m[2][1] - m.m[0][1] * m.m[2][2];
	inverse[0][2] = m.m[0][1] * m.m[1][2] - m.m[0][2] * m.m[1][1];
	inverse[1][1] = m.m[0][0] * m.m[2][2] - m.m[0][2] * m.m[2][0];
	inverse[1][2] = m.m[0][2] * m.m[1][0] - m.m[0][0] * m.m[1][2];
	inverse[2][1] = m.m[0][1] * m.m[2][0] - m.m[0][0] * m.m[2][1];
	inverse[2][2] = m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0];

	m.m[0][0] = inverse[0][0] * invDet;
	m.m[0][1] = inverse[0][1] * invDet;
	m.m[0][2] = inverse[0][2] * invDet;

	m.m[1][0] = inverse[1][0] * invDet;
	m.m[1][1] = inverse[1][1] * invDet;
	m.m[1][2] = inverse[1][2] * invDet;

	m.m[2][0] = inverse[2][0] * invDet;
	m.m[2][1] = inverse[2][1] * invDet;
	m.m[2][2] = inverse[2][2] * invDet;

	return true;
}

const bool M33_equal( const M33f& a, const M33f& b, const float epsilon )
{
	if( a.r0.equals( a.r0, epsilon ) &&
		a.r1.equals( a.r1, epsilon ) &&
		a.r2.equals( a.r2, epsilon ) )
	{
		return true;
	}

	return false;
}

const bool M33_isValidRotationMatrix( const M33f& m, const float epsilon /*= MATRIX_EPSILON*/ )
{
	M33f	inverse_matrix;
	if( !M33_tryInverse( inverse_matrix ) ) {
		return false;
	}

	const M33f	transposed = M33_Transpose( m );
	if( !M33_equal( transposed, inverse_matrix, epsilon ) ) {
		return false;
	}

	if( mmAbs( M33_determinant( m ) - 1 ) > epsilon ) {
		return false;
	}

	return true;
}

// returns a 3x3 skew symmetric matrix which is equivalent to the cross product
const M33f M33_GetSkewSymmetricMatrix( const V3f& v )
{
	M33f	result;
	result.r0 = V3_Set(  0,   -v.z,  v.y );
	result.r1 = V3_Set(  v.z,  0,   -v.x );
	result.r2 = V3_Set( -v.y,  v.x,  0   );
	return result;
}

const M33f M33_RotationX( float angle )
{
	float s, c;
	mmSinCos(angle, s, c);

	M33f M;
	M.r0.x = 1;
	M.r0.y = 0;
	M.r0.z = 0;

	M.r1.x = 0;
	M.r1.y = c;
	M.r1.z = -s;

	M.r2.x = 0;
	M.r2.y = s;
	M.r2.z = c;
	return M;
}
const M33f M33_RotationY( float angle )
{
	float s, c;
	mmSinCos(angle, s, c);

	M33f M;
	M.r0.x = c;
	M.r0.y = 0.0f;
	M.r0.z = -s;

	M.r1.x = 0.0f;
	M.r1.y = 1.0f;
	M.r1.z = 0.0f;

	M.r2.x = s;
	M.r2.y = 0.0f;
	M.r2.z = c;
	return M;
}
const M33f M33_RotationZ( float angle )
{
	float s, c;
	mmSinCos(angle, s, c);

	M33f M;
	M.r0.x = c;
	M.r0.y = s;
	M.r0.z = 0.0f;

	M.r1.x = -s;
	M.r1.y = c;
	M.r1.z = 0.0f;

	M.r2.x = 0.0f;
	M.r2.y = 0.0f;
	M.r2.z = 1.0f;
	return M;
}

const M33f M33_RotationPitchRollYaw( float pitch, float roll, float yaw )
{
mxUNDONE;//THIS MATRIX IS INCORRECT!
	float sr, sp, sy, cr, cp, cy;

	mmSinCos( pitch, sp, cp );
	mmSinCos( roll, sr, cr );
	mmSinCos( yaw, sy, cy );

	M33f M;
	M.r0 = V3_Set( cp * cy, cp * sy, -sp );
	M.r1 = V3_Set( sr * sp * cy + cr * -sy, sr * sp * sy + cr * cy, sr * cp );
	M.r2 = V3_Set( cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp );
	return M;
}

/// Constructs a rotation matrix from the given (unit-length) quaternion.
const M33f M33_FromQuaternion( Vec4Arg0 Q )
{
#if 1
	const V4f& q = (V4f&)Q;
	mxSTOLEN("Sony math library");
    const float qx = q.x;
    const float qy = q.y;
    const float qz = q.z;
    const float qw = q.w;
    const float qx2 = ( qx + qx );
    const float qy2 = ( qy + qy );
    const float qz2 = ( qz + qz );
    const float qxqx2 = ( qx * qx2 );
    const float qxqy2 = ( qx * qy2 );
    const float qxqz2 = ( qx * qz2 );
    const float qxqw2 = ( qw * qx2 );
    const float qyqy2 = ( qy * qy2 );
    const float qyqz2 = ( qy * qz2 );
    const float qyqw2 = ( qw * qy2 );
    const float qzqz2 = ( qz * qz2 );
    const float qzqw2 = ( qw * qz2 );

	M33f	result;
    result.r0 = V3_Set( ( ( 1.0f - qyqy2 ) - qzqz2 ), ( qxqy2 + qzqw2 ), ( qxqz2 - qyqw2 ) );
    result.r1 = V3_Set( ( qxqy2 - qzqw2 ), ( ( 1.0f - qxqx2 ) - qzqz2 ), ( qyqz2 + qxqw2 ) );
    result.r2 = V3_Set( ( qxqz2 + qyqw2 ), ( qyqz2 - qxqw2 ), ( ( 1.0f - qxqx2 ) - qyqy2 ) );
	return result;
#else
	M33f	result;
	result.r0 = V3_Set(
		1 - 2 * q.y * q.y - 2 * q.z * q.z,
		2 * q.x * q.y + 2 * q.w * q.z,
		2 * q.x * q.z - 2 * q.w * q.y
		);
	result.r1 = V3_Set(
		2 * q.x * q.y - 2 * q.w * q.z,
		1 - 2 * q.x * q.x - 2 * q.z * q.z,
		2 * q.y * q.z + 2 * q.w * q.x
		);
	result.r2 = V3_Set(
		2 * q.x * q.z + 2 * q.w * q.y,
		2 * q.y * q.z - 2 * q.w * q.x,
		1 - 2 * q.x * q.x - 2 * q.y * q.y
		);
	return result;
#endif
}

const bool M33_isSymmetric( const M33f& m, const float epsilon /*= MATRIX_EPSILON*/ )
{
	if ( mmAbs( m.m[0][1] - m.m[1][0] ) > epsilon ) {
		return false;
	}
	if ( mmAbs( m.m[0][2] - m.m[2][0] ) > epsilon ) {
		return false;
	}
	if ( mmAbs( m.m[1][2] - m.m[2][1] ) > epsilon ) {
		return false;
	}
	return true;
}

const bool M33_rowsNormalized( const M33f& m, const float epsilon /*= MATRIX_EPSILON*/ )
{
	if ( mmAbs( V3_Length( m.r0 ) - 1 ) > epsilon ) {
		return false;
	}
	if ( mmAbs( V3_Length( m.r1 ) - 1 ) > epsilon ) {
		return false;
	}
	if ( mmAbs( V3_Length( m.r2 ) - 1 ) > epsilon ) {
		return false;
	}
	return true;
}

/*
	/// Computes the dyadic product (aka 'outer product', 'vector direct product', 'tensor product') between the given two vectors.
	mxFORCEINLINE const Tuple3 Dyadic( const Tuple3& a, const Tuple3& b )
	{
		Tuple3 result;	// returns a second order tensor
		result.x = (a.y * b.z) - (a.z * b.y);
		result.y = (a.z * b.x) - (a.x * b.z);
		result.z = (a.x * b.y) - (a.y * b.x);
		return result;
	}
*/

// Builds a matrix that rotates around an arbitrary normal vector.
const M33f M33_RotationNormal( const V3f& axis, float angle )
{
	mxASSERT(V3_IsNormalized(axis));
	//derivation:
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/
	// Rodrigues' rotation formula
	// http://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula

	float s, c;
	mmSinCos( angle, s, c );

	float t = 1.0 - c;

	mxSTOLEN("Sony math library");
    float xy = ( axis.x * axis.y );
    float yz = ( axis.y * axis.z );
    float zx = ( axis.z * axis.x );

	M33f	result;
	result.r0 = V3_Set( ( ( axis.x * axis.x ) * t ) + c, ( xy * t ) + ( axis.z * s ), ( zx * t ) - ( axis.y * s ) );
    result.r1 = V3_Set( ( xy * t ) - ( axis.z * s ), ( ( axis.y * axis.y ) * t ) + c, ( yz * t ) + ( axis.x * s ) );
    result.r2 = V3_Set( ( zx * t ) + ( axis.y * s ), ( yz * t ) - ( axis.x * s ), ( ( axis.z * axis.z ) * t ) + c );
	return result;
}

// extracts Euler angles from the given rotation matrix
// http://nghiaho.com/?page_id=846
const V3f M33_ExtractPitchYawRoll( const M33f& m )
{
	float x = mmATan2(m.m[2][1], m.m[2][2]);
	float y = mmATan2(-m.m[2][0], mmSqrt(Square(m.m[2][1]) + Square(m.m[2][2])));
	float z = mmATan2(m.m[1][0], m.m[0][0]);
	return V3_Set(x,y,z);
}


const M34f M34_Identity()
{
	M34f	result;
	result = M34_Pack( M44_Identity() );
	return result;
}
const M34f M34_Pack( const M44f& m )
{
	mxASSERT( m.v0.w == 0.0f );
	mxASSERT( m.v1.w == 0.0f );
	mxASSERT( m.v2.w == 0.0f );
	mxASSERT( m.v3.w == 1.0f );

	M34f	result;

	result.v[0] = m.v3;	// 3rd row - translation
	result.v[1] = m.v0;	// 0st row
	result.v[2] = m.v1;	// 1st row

	// 2nd row
	result.v[0].w = m.v2.x;
	result.v[1].w = m.v2.y;
	result.v[2].w = m.v2.z;

	return result;
}
const M44f M34_Unpack( const M34f& m )
{
	M44f result;

	result.v[0] = m.v[1];
	result.v[1] = m.v[2];
	result.r[2] = Vector4_Set( m.v[0].w, m.v[1].w, m.v[2].w, 0.0f );
	result.v[3] = m.v[0];

	result.v[0].w = 0.0f;
	result.v[1].w = 0.0f;
	result.v[3].w = 1.0f;

	return result;
}
const V3f M34_GetTranslation( const M34f& m )
{
	return Vector4_As_V3( m.r[0] );
}
void M34_SetTranslation( M34f & m, const V3f& T )
{
	m.v[0].x = T.x;
	m.v[0].y = T.y;
	m.v[0].z = T.z;
}

// Field-of-View calculation utility, adapted from: http://gamedev.stackexchange.com/a/53604/15235
// This helps to find an aspect-adjusted horizontal FOV that maintains vertical FOV irrespective of resolution and aspect ratio.
// fov_y = CalcFovY( playerAdjustableFOV, 4, 3 ); // this is your base aspect that adjusted FOV should be relative to
// fov_x = CalcFovX( fov_y, width, height ); // this is your actual window width and height

float Get_Field_of_View_X_Degrees( float FoV_Y_degrees, float width, float height )
{
	float   a;
	float   y;

	FoV_Y_degrees = clampf(FoV_Y_degrees, 1.0f, 179.0f);

	y = height / tanf( DEG2RAD(FoV_Y_degrees * 0.5f) );
	a = atanf( width / y );
	a = RAD2DEG( a ) * 2.0f;

	return a;
}

float Get_Field_of_View_Y_Degrees( float FoV_X_degrees, float width, float height )
{
	float   a;
	float   x;

	FoV_X_degrees = clampf(FoV_X_degrees, 1.0f, 179.0f);

	x = width / tanf( DEG2RAD(FoV_X_degrees * 0.5f));
	a = atanf( height / x );
	a = RAD2DEG( a ) * 2.0f;

	return a;
}

const M44f M44_BuildView(
							 const V3f& rightDirection,
							 const V3f& lookDirection,
							 const V3f& upDirection,
							 const V3f& eyePosition
							 )
{
	mxASSERT(V3_IsNormalized(rightDirection));
	mxASSERT(V3_IsNormalized(lookDirection));
	mxASSERT(V3_IsNormalized(upDirection));
	M44f	cameraWorldMatrix;
	cameraWorldMatrix.v0 = V4f::set( rightDirection, 0.0f );	// X
	cameraWorldMatrix.v1 = V4f::set( lookDirection,  0.0f );	// Y
	cameraWorldMatrix.v2 = V4f::set( upDirection,    0.0f );	// Z
	cameraWorldMatrix.v3 = V4f::set( eyePosition,    1.0f );	// T
	return M44_OrthoInverse( cameraWorldMatrix );
}

#if MX_DEVELOPER

void runUnitTests_M44f()
{
	float FoV_Y_radians = DEG2RAD(90);
	float aspect_ratio = 1024.0f / 768;
	float near_clip = 1.0f;

	M44f	projection_matrix, inverse_projection_matrix;

	M44_ProjectionAndInverse_D3D_ReverseDepth_InfiniteFarPlane(
		&projection_matrix, &inverse_projection_matrix
		, FoV_Y_radians
		, aspect_ratio
		, near_clip
	);

	//
	const V3f	point_on_near_plane = CV3f(0, 1, 0);
	const V3f	point_very_far_away = CV3f(0, 99999999, 0);

	//
	const V4f	projected_point_on_near_plane = M44_Project(
		projection_matrix,
		V4f::set( point_on_near_plane, 1.0f )
		);
	const V4f	projected_point_very_far_away = M44_Project(
		projection_matrix,
		V4f::set( point_very_far_away, 1.0f )
		);

	//
	M44f	manually_computed_inverse_projection_patrix = M44_Inverse( projection_matrix );
	mxASSERT(
		M44_Equal( manually_computed_inverse_projection_patrix, inverse_projection_matrix, 1e-4f )
		);

	mxASSERT(
		fabs(projected_point_on_near_plane.z - 1.0f) < 1e-5f
		);
	mxASSERT(
		fabs(projected_point_very_far_away.z) < 1e-5f
		);
}

#endif // MX_DEVELOPER
