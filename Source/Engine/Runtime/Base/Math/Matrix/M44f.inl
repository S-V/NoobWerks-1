
/// Transforms a 4D vector by a 4x4 matrix: p' = p * M.
mmINLINE const Vector4 M44_TransformVector4( Mat4Arg M, Vec4Arg0 p )
{
#if !!MM_NO_INTRINSICS
	V4f	result;
	// p' = p * M
	result.x = (M.r0.x * p.x) + (M.r1.x * p.y) + (M.r2.x * p.z) + (M.r3.x * p.w);
	result.y = (M.r0.y * p.x) + (M.r1.y * p.y) + (M.r2.y * p.z) + (M.r3.y * p.w);
	result.z = (M.r0.z * p.x) + (M.r1.z * p.y) + (M.r2.z * p.z) + (M.r3.z * p.w);
	result.w = (M.r0.w * p.x) + (M.r1.w * p.y) + (M.r2.w * p.z) + (M.r3.w * p.w);
	return result;
#else
	// copied from XMVector4Transform():
	// Splat x,y,z and w
	Vector4 vTempX = mmSPLAT_X( p );
	Vector4 vTempY = mmSPLAT_Y( p );
	Vector4 vTempZ = mmSPLAT_Z( p );
	Vector4 vTempW = mmSPLAT_W( p );
	// Mul by the matrix
	vTempX = _mm_mul_ps( vTempX, M.r[0] );
	vTempY = _mm_mul_ps( vTempY, M.r[1] );
	vTempZ = _mm_mul_ps( vTempZ, M.r[2] );
	vTempW = _mm_mul_ps( vTempW, M.r[3] );
	// add them all together
	vTempX = _mm_add_ps( vTempX, vTempY );
	vTempZ = _mm_add_ps( vTempZ, vTempW );
	vTempX = _mm_add_ps( vTempX, vTempZ );
	return vTempX;
#endif
}

//XMVector4TransformStream
mmINLINE void M44_TransformVector4_Array(
	const Vector4* __restrict input, const UINT _count, Mat4Arg _M,
	Vector4 *__restrict _output
	)
{
#if 0
	for( UINT i = 0; i < _count; i++ )
	{
		_output[ i ] = M44_TransformVector4( _M, input[ i ] );
	}
#else	// 4x unrolled
	for( UINT i = 0; i < (_count & ~3); i += 4 )
	{
		_output[ i+0 ] = M44_TransformVector4( _M, input[ i+0 ] );
		_output[ i+1 ] = M44_TransformVector4( _M, input[ i+1 ] );
		_output[ i+2 ] = M44_TransformVector4( _M, input[ i+2 ] );
		_output[ i+3 ] = M44_TransformVector4( _M, input[ i+3 ] );
	}
	for( UINT i = (_count & ~3); i < _count; i++ )
	{
		_output[ i ] = M44_TransformVector4( _M, input[ i ] );
	}
#endif
}

/// Planes can be transformed as ordinary 4D vectors by the inverse-transpose of the matrix (transpose(inverse(M)).
mmINLINE void M44_TransformPlane_Array(
	const Vector4* __restrict input, const UINT _count, Mat4Arg _M,
	Vector4 *__restrict _output
	)
{
#if 0
	for( UINT i = 0; i < _count; i++ )
	{
		_output[ i ] = Vector4_Normalized( M44_TransformVector4( _M, input[ i ] ) );
	}
#else	// 4x unrolled
	for( UINT i = 0; i < (_count & ~3); i += 4 )
	{
		_output[ i+0 ] = Vector4_Normalized( M44_TransformVector4( _M, input[ i+0 ] ) );
		_output[ i+1 ] = Vector4_Normalized( M44_TransformVector4( _M, input[ i+1 ] ) );
		_output[ i+2 ] = Vector4_Normalized( M44_TransformVector4( _M, input[ i+2 ] ) );
		_output[ i+3 ] = Vector4_Normalized( M44_TransformVector4( _M, input[ i+3 ] ) );
	}
	for( UINT i = (_count & ~3); i < _count; i++ )
	{
		_output[ i ] = Vector4_Normalized( M44_TransformVector4( _M, input[ i ] ) );
	}
#endif
}

/// Performs a 4x4 matrix multiply by a 4x4 matrix.
/// Matrix multiplication order: left-to-right (as in DirectX), i.e. the transform 'b' is applied after 'a'.
mmINLINE const M44f M44_Multiply( Mat4Arg A, Mat4Arg B )
{
#if !!MM_NO_INTRINSICS
	M44f	result;
	result.r0 = M44_TransformVector4( B, A.r0 );
	result.r1 = M44_TransformVector4( B, A.r1 );
	result.r2 = M44_TransformVector4( B, A.r2 );
	result.r3 = M44_TransformVector4( B, A.r3 );
	return result;
#else
    M44f mResult;
    // Use vW to hold the original row
    Vector4 vW = A.r[0];
    // Splat the component X,Y,Z then W
    Vector4 vX = mmPERMUTE_PS(vW,_MM_SHUFFLE(0,0,0,0));
    Vector4 vY = mmPERMUTE_PS(vW,_MM_SHUFFLE(1,1,1,1));
    Vector4 vZ = mmPERMUTE_PS(vW,_MM_SHUFFLE(2,2,2,2));
    vW = mmPERMUTE_PS(vW,_MM_SHUFFLE(3,3,3,3));
    // Perform the operation on the first row
    vX = _mm_mul_ps(vX,B.r[0]);
    vY = _mm_mul_ps(vY,B.r[1]);
    vZ = _mm_mul_ps(vZ,B.r[2]);
    vW = _mm_mul_ps(vW,B.r[3]);
    // Perform a binary add to reduce cumulative errors
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    mResult.r[0] = vX;
    // Repeat for the other 3 rows
    vW = A.r[1];
    vX = mmPERMUTE_PS(vW,_MM_SHUFFLE(0,0,0,0));
    vY = mmPERMUTE_PS(vW,_MM_SHUFFLE(1,1,1,1));
    vZ = mmPERMUTE_PS(vW,_MM_SHUFFLE(2,2,2,2));
    vW = mmPERMUTE_PS(vW,_MM_SHUFFLE(3,3,3,3));
    vX = _mm_mul_ps(vX,B.r[0]);
    vY = _mm_mul_ps(vY,B.r[1]);
    vZ = _mm_mul_ps(vZ,B.r[2]);
    vW = _mm_mul_ps(vW,B.r[3]);
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    mResult.r[1] = vX;
    vW = A.r[2];
    vX = mmPERMUTE_PS(vW,_MM_SHUFFLE(0,0,0,0));
    vY = mmPERMUTE_PS(vW,_MM_SHUFFLE(1,1,1,1));
    vZ = mmPERMUTE_PS(vW,_MM_SHUFFLE(2,2,2,2));
    vW = mmPERMUTE_PS(vW,_MM_SHUFFLE(3,3,3,3));
    vX = _mm_mul_ps(vX,B.r[0]);
    vY = _mm_mul_ps(vY,B.r[1]);
    vZ = _mm_mul_ps(vZ,B.r[2]);
    vW = _mm_mul_ps(vW,B.r[3]);
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    mResult.r[2] = vX;
    vW = A.r[3];
    vX = mmPERMUTE_PS(vW,_MM_SHUFFLE(0,0,0,0));
    vY = mmPERMUTE_PS(vW,_MM_SHUFFLE(1,1,1,1));
    vZ = mmPERMUTE_PS(vW,_MM_SHUFFLE(2,2,2,2));
    vW = mmPERMUTE_PS(vW,_MM_SHUFFLE(3,3,3,3));
    vX = _mm_mul_ps(vX,B.r[0]);
    vY = _mm_mul_ps(vY,B.r[1]);
    vZ = _mm_mul_ps(vZ,B.r[2]);
    vW = _mm_mul_ps(vW,B.r[3]);
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    mResult.r[3] = vX;
    return mResult;
#endif
}



mmINLINE const M44f M44_Identity()
{
	//M44f	result;
	//result.r0 = Vector4_Set( 1.0f, 0.0f, 0.0f, 0.0f );
	//result.r1 = Vector4_Set( 0.0f, 1.0f, 0.0f, 0.0f );
	//result.r2 = Vector4_Set( 0.0f, 0.0f, 1.0f, 0.0f );
	//result.r3 = Vector4_Set( 0.0f, 0.0f, 0.0f, 1.0f );
	//return result;
	return g_MM_Identity;
}
mmINLINE const M44f M44_uniformScaling( float uniform_scaling )
{
	return M44_Scaling( uniform_scaling, uniform_scaling, uniform_scaling );
}
mmINLINE const M44f M44_scaling( const V3f& scaling )
{
	return M44_Scaling( scaling.x, scaling.y, scaling.z );
}
mmINLINE const M44f M44_Scaling( float x, float y, float z )
{
	M44f	result;

	result.r0 = Vector4_Set( x,    0.0f, 0.0f, 0.0f );
	result.r1 = Vector4_Set( 0.0f, y,    0.0f, 0.0f );
	result.r2 = Vector4_Set( 0.0f, 0.0f, z,    0.0f );
	result.r3 = g_MM_Identity.r3;

	return result;
}
mmINLINE const M44f M44_buildTranslationMatrix( float x, float y, float z )
{
	M44f	result;
	result.r0 = Vector4_Set( 1.0f, 0.0f, 0.0f, 0.0f );
	result.r1 = Vector4_Set( 0.0f, 1.0f, 0.0f, 0.0f );
	result.r2 = Vector4_Set( 0.0f, 0.0f, 1.0f, 0.0f );
	result.r3 = Vector4_Set( x,    y,    z,    1.0f );
	return result;
}
mmINLINE const M44f M44_buildTranslationMatrix( const V3f& xyz )
{
	M44f	result;
	result.r0 = Vector4_Set( 1.0f,  0.0f,  0.0f,  0.0f );
	result.r1 = Vector4_Set( 0.0f,  1.0f,  0.0f,  0.0f );
	result.r2 = Vector4_Set( 0.0f,  0.0f,  1.0f,  0.0f );
	result.r3 = Vector4_Set( xyz.x, xyz.y, xyz.z, 1.0f );
	return result;
}

mmINLINE void M44_SetTranslation( M44f *m, const V3f& T )
{
	m->v3.x = T.x;
	m->v3.y = T.y;
	m->v3.z = T.z;
}
mmINLINE const V3f M44_getTranslation( const M44f& m )
{
	return Vector4_As_V3( m.r3 );
}

mmINLINE const V3f M44_getScaling( const M44f& m )
{
	return CV3f( m.m[0][0], m.m[1][1], m.m[2][2] );
}

/// Transforms a 3D point (W=1) by a 4x4 matrix.
mmINLINE const V3f M44_TransformPoint( const M44f& m, const V3f& p )
{
	V3f	result;
	result.x = (m.v0.x * p.x) + (m.v1.x * p.y) + (m.v2.x * p.z) + (m.v3.x);
	result.y = (m.v0.y * p.x) + (m.v1.y * p.y) + (m.v2.y * p.z) + (m.v3.y);
	result.z = (m.v0.z * p.x) + (m.v1.z * p.y) + (m.v2.z * p.z) + (m.v3.z);
	return result;
}
/// Transforms a 3D direction (W=0) by a 4x4 matrix.
mmINLINE const V3f M44_TransformNormal( const M44f& m, const V3f& v )
{
	V3f	result;
	result.x = (m.v0.x * v.x) + (m.v1.x * v.y) + (m.v2.x * v.z);
	result.y = (m.v0.y * v.x) + (m.v1.y * v.y) + (m.v2.y * v.z);
	result.z = (m.v0.z * v.x) + (m.v1.z * v.y) + (m.v2.z * v.z);
	return result;
}
///// Transforms a 3D direction (W=0) by a 4x4 matrix: v' = v * M.
//mmINLINE const V3f M44_RotatePoint( const V3f& P, const M44f& M )
//{
//	V3f	result;
//	result.x = (M.v0.x * P.x) + (M.v1.x * P.y) + (M.v2.x * P.z);
//	result.y = (M.v0.y * P.x) + (M.v1.y * P.y) + (M.v2.y * P.z);
//	result.z = (M.v0.z * P.x) + (M.v1.z * P.y) + (M.v2.z * P.z);
//	return result;
//}

///// Transforms a 3D direction (W=0) by a 4x4 matrix.
//mmINLINE const V4f M44_TransformNormal3( const M44f& m, const V4f& p )
//{
//	V4f	result;
//	result.x = (m.v0.x * p.x) + (m.v1.x * p.y) + (m.v2.x * p.z);
//	result.y = (m.v0.y * p.x) + (m.v1.y * p.y) + (m.v2.y * p.z);
//	result.z = (m.v0.z * p.x) + (m.v1.z * p.y) + (m.v2.z * p.z);
//	result.w = (m.v0.w * p.x) + (m.v1.w * p.y) + (m.v2.w * p.z);
//	return result;
//}

/// Transforms a 4D point by a 4x4 matrix.
mmINLINE const V4f M44_Transform( const M44f& m, const V4f& p )
{
	V4f	result;
	// p' = p * M
	result.x = (m.v0.x * p.x) + (m.v1.x * p.y) + (m.v2.x * p.z) + (m.v3.x * p.w);
	result.y = (m.v0.y * p.x) + (m.v1.y * p.y) + (m.v2.y * p.z) + (m.v3.y * p.w);
	result.z = (m.v0.z * p.x) + (m.v1.z * p.y) + (m.v2.z * p.z) + (m.v3.z * p.w);
	result.w = (m.v0.w * p.x) + (m.v1.w * p.y) + (m.v2.w * p.z) + (m.v3.w * p.w);
	return result;
}

/// Transforms a 3D point (W=1) by a 4x4 matrix.
mmINLINE const V4f M44_Transform3( const M44f& m, const V4f& p )
{
	V4f	result;
	// v' = v * M
	result.x = (m.v0.x * p.x) + (m.v1.x * p.y) + (m.v2.x * p.z) + (m.v3.x);
	result.y = (m.v0.y * p.x) + (m.v1.y * p.y) + (m.v2.y * p.z) + (m.v3.y);
	result.z = (m.v0.z * p.x) + (m.v1.z * p.y) + (m.v2.z * p.z) + (m.v3.z);
	result.w = (m.v0.w * p.x) + (m.v1.w * p.y) + (m.v2.w * p.z) + (m.v3.w);
	return result;
}

mmINLINE const M44f M44_Transpose( Mat4Arg M )
{
#if MM_NO_INTRINSICS
	M44f	result;
	result.v0 = Vector4_Set( M.v0.x, M.v1.x, M.v2.x, M.v3.x );
	result.v1 = Vector4_Set( M.v0.y, M.v1.y, M.v2.y, M.v3.y );
	result.v2 = Vector4_Set( M.v0.z, M.v1.z, M.v2.z, M.v3.z );
	result.v3 = Vector4_Set( M.v0.w, M.v1.w, M.v2.w, M.v3.w );
	return result;
#else
    // x.x,x.y,y.x,y.y
    Vector4 vTemp1 = _mm_shuffle_ps(M.r[0],M.r[1],_MM_SHUFFLE(1,0,1,0));
    // x.z,x.w,y.z,y.w
    Vector4 vTemp3 = _mm_shuffle_ps(M.r[0],M.r[1],_MM_SHUFFLE(3,2,3,2));
    // z.x,z.y,w.x,w.y
    Vector4 vTemp2 = _mm_shuffle_ps(M.r[2],M.r[3],_MM_SHUFFLE(1,0,1,0));
    // z.z,z.w,w.z,w.w
    Vector4 vTemp4 = _mm_shuffle_ps(M.r[2],M.r[3],_MM_SHUFFLE(3,2,3,2));
    M44f mResult;
    // x.x,y.x,z.x,w.x
    mResult.r[0] = _mm_shuffle_ps(vTemp1, vTemp2,_MM_SHUFFLE(2,0,2,0));
    // x.y,y.y,z.y,w.y
    mResult.r[1] = _mm_shuffle_ps(vTemp1, vTemp2,_MM_SHUFFLE(3,1,3,1));
    // x.z,y.z,z.z,w.z
    mResult.r[2] = _mm_shuffle_ps(vTemp3, vTemp4,_MM_SHUFFLE(2,0,2,0));
    // x.w,y.w,z.w,w.w
    mResult.r[3] = _mm_shuffle_ps(vTemp3, vTemp4,_MM_SHUFFLE(3,1,3,1));
    return mResult;
#endif
}

/// Compute the inverse of a 4x4 matrix, which is expected to be an affine matrix with an orthogonal upper-left 3x3 submatrix.
/// This can be used to achieve better performance (and precision, too)
/// than a general inverse when the specified 4x4 matrix meets the given restrictions.
// 
mmINLINE const M44f M44_OrthoInverse( Mat4Arg m )
{
	// Transpose the left-upper 3x3 matrix (invert the rotation part).
	V4f	axisX = V4f::set( m.v0.x, m.v1.x, m.v2.x, 0.0f );
	V4f	axisY = V4f::set( m.v0.y, m.v1.y, m.v2.y, 0.0f );
	V4f	axisZ = V4f::set( m.v0.z, m.v1.z, m.v2.z, 0.0f );

	// Compute the (negative) translation component.
	float Tx = -V4f::dot( m.v0, m.v3 );
	float Ty = -V4f::dot( m.v1, m.v3 );
	float Tz = -V4f::dot( m.v2, m.v3 );

	M44f	result;
	result.v0 = axisX;
	result.v1 = axisY;
	result.v2 = axisZ;
	result.v3 = V4f::set( Tx, Ty, Tz, 1.0f );
	return result;
}

/// calculated with (matrix in row-major format):
/// http://www.quickmath.com/webMathematica3/quickmath/matrices/inverse/basic.jsp#c=inverse_matricesinverse&v1=W%2C0%2C0%2C0%0A0%2C0%2CA%2C1%0A0%2CH%2C0%2C0%0A0%2C0%2CB%2C0%0A
mmINLINE const M44f ProjectionM44_Inverse( Mat4Arg m )
{
	const float H = m.v0.x;	// 1 / tan( FoVy / 2 ) / aspect_ratio
	const float V = m.v2.y;	// 1 / tan( FoVy / 2 )
	const float A = m.v1.z;	// far / (far - near)
	const float B = m.v3.z;	// -near * far / (far - near)

	M44f	result;
	result.v0 = V4f::set( 1.0f / H,	0.0f,	0.0f,		0.0f );
	result.v1 = V4f::set( 0.0f,		0.0f,	1.0f / V,	0.0f );
	result.v2 = V4f::set( 0.0f,		0.0f,	0.0f,		1.0f / B );
	result.v3 = V4f::set( 0.0f,		1.0f,	0.0f,		-A / B );
	return result;
}
/*
Multiplication by inverse projection matrix (P^-1):

            | 1/H   0   0    0  |
[x y z w] * |  0    0  1/V   0  | = | (x/H)   (w)   (y/V)  (z/B - w*A/B) |
            |  0    0   0   1/B |
            |  0    1   0  -A/B |
*/




// NOTE: operator overloads should be defined only for 'non-vectorizable' types (V2f, V3f).
// NOTE: operator overloads must be unambiguous!

#if MM_OVERLOAD_OPERATORS

mmINLINE const M44f operator * ( const M44f& a, const M44f& b ) {
	return M44_Multiply( a, b );
}

mmINLINE const M44f operator + ( const M44f& a, const M44f& b )
{
	M44f result;
	result.r0 = Vector4_Add( a.r0, b.r0 );
	result.r1 = Vector4_Add( a.r1, b.r1 );
	result.r2 = Vector4_Add( a.r2, b.r2 );
	result.r3 = Vector4_Add( a.r3, b.r3 );
	return result;
}
mmINLINE const M44f operator - ( const M44f& a, const M44f& b )
{
	M44f result;
	result.r0 = Vector4_Subtract( a.r0, b.r0 );
	result.r1 = Vector4_Subtract( a.r1, b.r1 );
	result.r2 = Vector4_Subtract( a.r2, b.r2 );
	result.r3 = Vector4_Subtract( a.r3, b.r3 );
	return result;
}
mmINLINE const M44f operator * ( const M44f& m, const float scale )
{
	M44f result;
	result.r0 = Vector4_Scale( m.r0, scale );
	result.r1 = Vector4_Scale( m.r1, scale );
	result.r2 = Vector4_Scale( m.r2, scale );
	result.r3 = Vector4_Scale( m.r3, scale );
	return result;
}

#endif // MM_OVERLOAD_OPERATORS
