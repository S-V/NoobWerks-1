//=====================================================================
//	SCALAR OPERATIONS
//=====================================================================

mmINLINE float mmRcp( float x )
{
	return 1.0f / x;
}
mmINLINE float mmAbs( float x )
{
	return ::fabs( x );
}
mmINLINE double mmAbs( double x )
{
	return ::abs( x );
}
mmINLINE float mmFloor( float x )
{
	return ::floorf( x );
}
mmINLINE float mmCeiling( float x )
{
	return ::ceilf( x );
}
// Returns the floating-point remainder of x/y (rounded towards zero):
// fmod = x - floor(x/y) * y
mmINLINE float mmModulo( float x, float y )
{
	return ::fmod( x, y );
}
mmINLINE int mmRoundToInt( float x )
{
	return (int) x;
}
// Computes 2 raised to the given power n.
mmINLINE float mmExp2f( float power )
{
	return ::pow( 2.0f, power );
}
// Produces a value with the magnitude of 'magnitude' and the sign of 'sign'
mmINLINE float mmCopySign( float magnitude, float sign )
{
	magnitude = mmAbs( magnitude );
	return sign > 0.0f ? magnitude : -magnitude;
}
mmINLINE int mmFloorToInt( float x )
{
	return (int)::floorf( x );
}
mxSTOLEN("Doom3 BFG edition");
mmINLINE BYTE mmFloatToByte( float f )
{
	__m128 x = _mm_load_ss( &f );
	// If a converted result is negative the value (0) is returned and if the
	// converted result is larger than the maximum byte the value (255) is returned.
	x = _mm_max_ss( x, g_MM_Zero );
	x = _mm_min_ss( x, g_MM_255 );
	return static_cast<BYTE>( _mm_cvttss_si32( x ) );
}
mmINLINE float mmSqrt( float x )
{
	return ::sqrtf( x );
}
mmINLINE float mmInvSqrt( float x )
{
	return 1.0f / ::sqrtf( x );
}
mmINLINE float mmInvSqrtEst( float x )
{
	return 1.0f / ::sqrtf( x );
}

mmINLINE double mmInvSqrt( double x )
{
	return 1.0 / ::sqrt( x );
}

mmINLINE void mmSinCos( float x, float &s, float &c )
{
	s = sinf(x);
	c = cosf(x);
}
mmINLINE float mmSin( float x )
{
	return sinf(x);
}
mmINLINE float mmCos( float x )
{
	return cosf(x);
}
mmINLINE float mmTan( float x )
{
	return tanf(x);
}
// The asin function returns the arcsine of x in the range –Pi/2 to Pi/2 radians.
mmINLINE float mmASin( float x )
{
	return asinf(x);
}
// The acos function returns the arccosine of x in the range 0 to Pi radians.
mmINLINE float mmACos( float x )
{
	return acosf(x);
}
// atan returns the arctangent of x in the range –Pi/2 to Pi/2 radians.
mmINLINE float mmATan( float x )
{
	return atanf(x);
}
// atan2 returns the arctangent of y/x in the range –Pi to Pi radians.
// For any real number (e.g., floating point) arguments x and y not both equal to zero,
// atan2(y, x) is the angle in radians between the positive x-axis of a plane
// and the point given by the coordinates (x, y) on it.
// The angle is positive for counter-clockwise angles (upper half-plane, y > 0),
// and negative for clockwise angles (lower half-plane, y < 0).
mmINLINE float mmATan2( float y, float x )
{
	return ::atan2f( y, x );
}

mmINLINE float mmExp( float x )
{
	return expf(x);
}
mmINLINE float mmLn( float x )
{
	return logf(x);
}
mmINLINE float mmLog10( float x )
{
	return log10f(x);
}
// _base ^ ? = value
mmINLINE float mmLog( float value, float _base )
{
	// B ^ x = V, x = ln(V) / ln(B)
	return logf(value) / logf(_base);
}

mmINLINE
double log2( double x )
{
	return ::log( x ) * mxINV_LN2;
}

mmINLINE float mmPow( float x, float y )
{
	return ::powf( x, y );
}
mmINLINE float mmLerp( float from, float to, float amount )
{
	// Precise method which guarantees v = v1 when t = 1.
	return from * (1.0f - amount) + to * amount;
}
// Robust Lerp, see: http://tulrich.com/geekstuff/lerp.html
inline float mmLerp3( float a, float b, float t )
{
	float bma = b - a;
	return (t < 0.5) ? (a + bma * t) : (b - bma * (1 - t));
}
mmINLINE static float Int_To_Float( int x )
{
	return (float) x;
}

//=====================================================================
//	DATA CONVERSION OPERATIONS
//=====================================================================

mmINLINE const V2f& V3_As_V2( const V3f& v )
{
	return *reinterpret_cast< const V2f* >( &v );
}
mmINLINE const V4f& Vector4_As_V4( const Vector4& v )
{
	return *reinterpret_cast< const V4f* >( &v );
}
mmINLINE const V3f& Vector4_As_V3( const Vector4& v )
{
	return *reinterpret_cast< const V3f* >( &v );
}
mmINLINE const V3f& V4_As_V3( const V4f& v )
{
	return *reinterpret_cast< const V3f* >( &v );
}

mmINLINE const bool V3_Equal( const V3f& a, const V3f& b )
{
	return ( a.x == b.x && a.y == b.y && a.z == b.z );
}

mmINLINE const bool V3_Equal( const V3f& a, const V3f& b, const float epsilon )
{
	if( fabs( a.x - b.x ) > epsilon ) {
		return false;
	}
	if( fabs( a.y - b.y ) > epsilon ) {
		return false;
	}
	if( fabs( a.z - b.z ) > epsilon ) {
		return false;
	}
	return true;
}

mmINLINE const bool V4_Equal( const V4f& a, const V4f& b )
{
	return ( a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w );
}

mmINLINE const bool V4_Equal( const V4f& a, const V4f& b, const float epsilon )
{
	if( fabs( a.x - b.x ) > epsilon ) {
		return false;
	}
	if( fabs( a.y - b.y ) > epsilon ) {
		return false;
	}
	if( fabs( a.z - b.z ) > epsilon ) {
		return false;
	}
	if( fabs( a.w - b.w ) > epsilon ) {
		return false;
	}
	return true;
}

/// The direction of the plane's normal follows the right hand rule.
mmINLINE const V4f Plane_FromThreePoints( const V3f& a, const V3f& b, const V3f& c )
{
	const V3f V1 = V3_Subtract( b, a );
	const V3f V2 = V3_Subtract( c, a );
	const V3f N = V3_Normalized( V3_Cross( V1, V2 ) );
	const float dist = V3_Dot( N, a );	//<= we could use any one of the three points
	V4f plane = { N.x, N.y, N.z, -dist };
	return plane;
}
mmINLINE const V4f Plane_FromPointNormal( const V3f& point, const V3f& normal )
{
	mxASSERT(V3_IsNormalized(normal));
	V4f plane = { normal.x, normal.y, normal.z, -V3_Dot( normal, point ) };
	return plane;
}
mmINLINE const V3f Plane_CalculateNormal( const V4f& plane )
{
	const V3f N = *reinterpret_cast< const V3f* >( &plane );
	return V3_Normalized( N );
}
mmINLINE const V3f& Plane_GetNormal( const V4f& plane )
{
	const V3f& N = *reinterpret_cast< const V3f* >( &plane );
	mxASSERT(V3_IsNormalized( N ));
	return N;
}
/// only normalizes the plane normal, does not adjust d
mmINLINE void Plane_Normalize( V4f & plane )
{
	V3f & N = *reinterpret_cast< V3f* >( &plane );
	N = V3_Normalized( N );
}
mmINLINE const float Plane_PointDistance( const V4f& plane, const V3f& point )
{
	mxASSERT(V3_IsNormalized( Plane_GetNormal( plane ) ));
	return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
}

mmINLINE Vector4 Vector4_Select(
    Vec4Arg0 a,
    Vec4Arg0 b,
    Vec4Arg0 mask
)
{
    Vector4 tmp1 = _mm_andnot_ps(mask, a);
    Vector4 tmp2 = _mm_and_ps(b, mask);
    return _mm_or_ps(tmp1, tmp2);
}

/// Returns the dot product (aka 'scalar product') of a and b.
mmINLINE const Vector4 Vector4_Dot( Vec4Arg0 A, Vec4Arg0 B )
{
#if MM_NO_INTRINSICS
	return (A.x * B.x) + (A.y * B.y) + (A.z * B.z) + (A.w * B.w);
#else
	#if defined(__SSE4_1__)
		// 0xFF: lower 4 bits = broadcast mask, high 4 bits = multiplication mask (zero bit = 0.0f).
		// The four resulting (after conditional multiplication) single-precision values are summed into an intermediate result
		// which is conditionally broadcasted to the destination using a broad-cast mask specified by bits [3:0] of the immediate byte.
		return _mm_dp_ps( A, B, 0xFF );
	#else
		// SSE2 version
		#if 1
			Vector4 t0 = _mm_mul_ps(A, B);
			Vector4 t1 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(1,0,3,2));
			Vector4 t2 = _mm_add_ps(t0, t1);
			Vector4 t3 = _mm_shuffle_ps(t2, t2, _MM_SHUFFLE(2,3,0,1));
			return _mm_add_ps(t3, t2);
		#else
		// from XNA Math:
			Vector4 vTemp2 = B;
			Vector4 vTemp = _mm_mul_ps(A,vTemp2);
			vTemp2 = _mm_shuffle_ps(vTemp2,vTemp,_MM_SHUFFLE(1,0,0,0)); // Copy X to the Z position and Y to the W position
			vTemp2 = _mm_add_ps(vTemp2,vTemp);          // add Z = X+Z; W = Y+W;
			vTemp = _mm_shuffle_ps(vTemp,vTemp2,_MM_SHUFFLE(0,3,0,0));  // Copy W to the Z position
			vTemp = _mm_add_ps(vTemp,vTemp2);           // add Z and W together
			return mmPERMUTE_PS(vTemp,_MM_SHUFFLE(2,2,2,2));    // Splat Z and return
		#endif
	#endif
#endif
}

/// Returns the dot product (aka 'scalar product') of a and b.
mmINLINE const Vector4 Vector4_Dot3( Vec4Arg0 A, Vec4Arg0 B )
{
#if MM_NO_INTRINSICS
	return (A.x * B.x) + (A.y * B.y) + (A.z * B.z);
#else
	#if defined(__SSE4_1__)
		// 0xFF: lower 4 bits = broadcast mask, high 4 bits = multiplication mask (zero bit = 0.0f).
		// The four resulting (after conditional multiplication) single-precision values are summed into an intermediate result
		// which is conditionally broadcasted to the destination using a broad-cast mask specified by bits [3:0] of the immediate byte.
		return _mm_dp_ps( A, B, 0x7F );
	#else
		// SSE2 version
		const unsigned int mask_array[] = { 0xffffffff, 0xffffffff, 0xffffffff, 0 };
		const Vector4 mask = _mm_load_ps((const float*)mask_array);
		const Vector4 m = _mm_mul_ps(lhs, rhs);
		const Vector4 s0 = _mm_and_ps(m, mask);
		const Vector4 s1 = _mm_add_ps(s0, _mm_movehl_ps(s0, s0));
		const Vector4 s2 = _mm_add_ss(s1, _mm_shuffle_ps(s1, s1, 1));
		return _mm_shuffle_ps(s2,s2, 0);
	#endif
#endif
}

mmINLINE const Vector4 Vector4_Min( Vec4Arg0 A, Vec4Arg0 B )
{
	return _mm_min_ps( A, B );
}
mmINLINE const Vector4 Vector4_Max( Vec4Arg0 A, Vec4Arg0 B )
{
	return _mm_max_ps( A, B );
}
mmINLINE const Vector4 Plane_DotCoord( Vec4Arg0 plane, Vec4Arg0 xyz )
{
	// result = P[0] * X + P[1] * Y + P[2] * Z + P[3]
	Vector4 xyz1 = Vector4_Select( g_MM_One, xyz, g_MM_Select1110.m128 );
	Vector4 result = Vector4_Dot( plane, xyz1 );
	return result;
}
mmINLINE const bool Plane_ContainsPoint( const V4f& plane, const V3f& point, float epsilon )
{
	const float distance = Plane_PointDistance( plane, point );
	return distance >= -epsilon && distance <= +epsilon;
}
mmINLINE const V4f Plane_Translate( const V4f& plane, const V3f& translation )
{
	V4f result = plane;
	result.w -= V3_Dot( Plane_GetNormal(plane), translation );
	return result;
}
//mmINLINE const V4f Plane_Transform( const V4f& plane, const M44f& transform )
//{
//	return M44_Transform(
//		M44_Transpose( M44_Inverse( transform ) ),
//		plane
//	);
//}
mmINLINE const bool Plane_RayIntersection( const V4f& plane, const V3f& start, const V3f& dir, float &_fraction )
{
	const V3f& planeNormal = Plane_GetNormal( plane );
	const float d1 = V3_Dot( start, planeNormal ) + plane.w;
	const float d2 = V3_Dot( dir, planeNormal );
	if ( d2 == 0.0f ) {
		return false;
	}
	_fraction = -( d1 / d2 );
	return true;
}
mmINLINE const bool Plane_LineIntersection( const V4f& plane, const V3f& start, const V3f& end, float &_fraction )
{
	const float d1 = Plane_PointDistance( plane, start );
	const float d2 = Plane_PointDistance( plane, end );
	if ( d1 == d2 ) {
		return false;
	}
	if ( d1 > 0.0f && d2 > 0.0f ) {
		return false;
	}
	if ( d1 < 0.0f && d2 < 0.0f ) {
		return false;
	}
	_fraction = ( d1 / ( d1 - d2 ) );
	return ( _fraction >= 0.0f && _fraction <= 1.0f );
}
mmINLINE const Half2 V2_To_Half2( const V2f& xy )
{
	Half2 result = { Float_To_Half(xy.x), Float_To_Half(xy.y) };
	return result;
}
mmINLINE const V2f Half2_To_V2F( const Half2& xy )
{
	return V2f( Half_To_Float(xy.x), Half_To_Float(xy.y) );
}

#if MM_OVERLOAD_OPERATORS

mmINLINE const V2f operator + ( const V2f& a, const V2f& b ) {
	return V2f( a.x + b.x, a.y + b.y );
}
mmINLINE const V2f operator - ( const V2f& a, const V2f& b ) {
	return V2f( a.x - b.x, a.y - b.y );
}
mmINLINE const V2f operator * ( const V2f& v, const float scale ) {
	return V2f( v.x * scale, v.y * scale );
}
mmINLINE const V2f operator / ( const V2f& v, const float scale ) {
	float inverse = mmRcp( scale );
	return V2f( v.x * inverse, v.y * inverse );
}

#endif // MM_OVERLOAD_OPERATORS

mmINLINE const V2f V2_Zero()
{
	V2f result;
	result.x = 0.0f;
	result.y = 0.0f;
	return result;
}
mmINLINE const V2f V2_Set( float x, float y )
{
	V2f result;
	result.x = x;
	result.y = y;
	return result;
}
mmINLINE const V2f V2_Replicate( float value )
{
	V2f result;
	result.x = value;
	result.y = value;
	return result;
}
mmINLINE const V2f V2_Scale( const V2f& v, float s )
{
	V2f result;
	result.x = v.x * s;
	result.y = v.y * s;
	return result;
}
mmINLINE const float V2_Length( const V2f& v )
{
	return mmSqrt( V2_Dot( v, v ) );
}
mmINLINE const float V2_Dot( const V2f& a, const V2f& b )
{
	return (a.x * b.x) + (a.y * b.y);
}
// Linearly interpolates one vector to another.
// Precise method which guarantees from = to when amount = 1.
mmINLINE const V2f V2_Lerp( const V2f& from, const V2f& to, float amount )
{
	if( amount <= 0.0f ) {
		return from;
	} else if( amount >= 1.0f ) {
		return to;
	} else {
		return V2_Lerp01( from, to, amount );
	}
}
mmINLINE const V2f V2_Lerp01( const V2f& from, const V2f& to, float amount )
{
	mxASSERT(amount >= 0.0f && amount <= 1.0f);
	return from + ( to - from ) * amount;
}

mmINLINE const V3f V3_Zero()
{
	V3f result;
	result.x = 0.0f;
	result.y = 0.0f;
	result.z = 0.0f;
	return result;
}
mmINLINE const V3f V3_SetAll( float value )
{
	V3f result;
	result.x = value;
	result.y = value;
	result.z = value;
	return result;
}
mmINLINE const V3f V3_Set( const float* xyz )
{
	V3f result;
	result.x = xyz[0];
	result.y = xyz[1];
	result.z = xyz[2];
	return result;
}
mmINLINE const V3f V3_Set( float x, float y, float z )
{
	V3f result;
	result.x = x;
	result.y = y;
	result.z = z;
	return result;
}
mmINLINE const V3f V3_Scale( const V3f& v, float s )
{
	V3f result;
	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	return result;
}
mmINLINE const float V3_LengthSquared( const V3f& v )
{
	return V3_Dot( v, v );
}
mmINLINE const float V3_Length( const V3f& v )
{
	return mmSqrt( V3_LengthSquared( v ) );
}
mmINLINE const V3f V3_Normalized( const V3f& v )
{
	float length = V3_LengthSquared( v );
	// Prevent divide by zero.
	if( length != 0.0f ) {
		length = mmInvSqrt( length );
	}
	return V3_Scale( v, length );
}
mmINLINE const V3f V3_Normalized( const V3f& v, float &_length )
{
	float length = V3_LengthSquared( v );
	// Prevent divide by zero.
	if( length != 0.0f ) {
		length = mmSqrt( length );
		_length = length;
		length = 1.0f / length;
	}
	return V3_Scale( v, length );
}
mmINLINE const float V3_Normalize( V3f * v )
{
	float length;
	*v = V3_Normalized( *v, length );
	return length;
}
mmINLINE const V3f V3_Abs( const V3f& _v )
{
	V3f result;
	result.x = mmAbs( _v.x );
	result.y = mmAbs( _v.y );
	result.z = mmAbs( _v.z );
	return result;
}
mmINLINE const V3f V3_Negate( const V3f& x )
{
	V3f result;
	result.x = -x.x;
	result.y = -x.y;
	result.z = -x.z;
	return result;
}
mmINLINE const V3f V3_Reciprocal( const V3f& v )
{
	return CV3f( mmRcp(v.x), mmRcp(v.y), mmRcp(v.z) );
}
mmINLINE const bool V3_IsInfinite( const V3f& v )
{
	return mmIS_INF(v.x) || mmIS_INF(v.y) || mmIS_INF(v.z);
}

mmINLINE const float V3_Mins( const V3f& xyz )
{
	return minf( minf( xyz.x, xyz.y ), xyz.z );
}
mmINLINE const float V3_Maxs( const V3f& xyz )
{
	return maxf( maxf( xyz.x, xyz.y ), xyz.z );
}
mmINLINE const V3f V3_Mins( const V3f& a, const V3f& b )
{
	V3f result;
	result.x = minf( a.x, b.x );
	result.y = minf( a.y, b.y );
	result.z = minf( a.z, b.z );
	return result;
}
mmINLINE const V3f V3_Maxs( const V3f& a, const V3f& b )
{
	V3f result;
	result.x = maxf( a.x, b.x );
	result.y = maxf( a.y, b.y );
	result.z = maxf( a.z, b.z );
	return result;
}
mmINLINE const V3f V3_Add( const V3f& a, const V3f& b )
{
	V3f result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}
mmINLINE const V3f V3_Subtract( const V3f& a, const V3f& b )
{
	V3f result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}
/// Component-wise multiplication.
mmINLINE const V3f V3_Multiply( const V3f& a, const V3f& b )
{
	V3f result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	return result;
}
/// Component-wise division.
mmINLINE const V3f V3_Divide( const V3f& a, const V3f& b )
{
	V3f result;
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	result.z = a.z / b.z;
	return result;
}
// Returns the dot product (aka 'scalar product') of a and b.
mmINLINE const float V3_Dot( const V3f& a, const V3f& b )
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}
// The direction of the cross product follows the right hand rule.
//definition: http://www.euclideanspace.com/maths/algebra/vectors/vecAlgebra/cross/index.htm
mmINLINE const V3f V3_Cross( const V3f& a, const V3f& b )
{
	V3f result;
	result.x = (a.y * b.z) - (a.z * b.y);
	result.y = (a.z * b.x) - (a.x * b.z);
	result.z = (a.x * b.y) - (a.y * b.x);
	return result;
}
// Finds a vector orthogonal to the given one.
// see: http://lolengine.net/blog/2013/09/21/picking-orthogonal-vector-combing-coconuts
mmINLINE const V3f V3_FindOrthogonalTo( const V3f& v )
{
	// Always works if the input is non-zero.
	// Doesn't require the input to be normalized.
	// Doesn't normalize the output.
	return mmAbs(v.x) > mmAbs(v.z) ?
		V3_Set(-v.y, v.x, 0.0f) : V3_Set(0.0f, -v.z, v.y);
}
// Returns the component-wise result of x * a + y * (1.0 - a),
// i.e., the linear blend of x and y using vector a.
// NOTE:
// The value for a is not restricted to the range [0, 1].
mmINLINE const V3f V3_Blend( const V3f& x, const V3f& y, float a )
{
	float b = 1.0f - a;
	V3f result;
	result.x = (x.x * a) + (y.x * b);
	result.y = (x.y * a) + (y.y * b);
	result.z = (x.z * a) + (y.z * b);
	return result;
}
// Linearly interpolates one vector to another.
// Precise method which guarantees from = to when amount = 1.
mmINLINE const V3f V3_Lerp( const V3f& from, const V3f& to, float amount )
{
	if( amount <= 0.0f ) {
		return from;
	} else if( amount >= 1.0f ) {
		return to;
	} else {
		return V3_Lerp01( from, to, amount );
	}
}
mmINLINE const V3f V3_Lerp01( const V3f& from, const V3f& to, float amount )
{
	mxASSERT(amount >= 0.0f && amount <= 1.0f);
	return V3_Add( from, V3_Scale( V3_Subtract(to, from), amount ) );
}
mmINLINE const bool V3_IsNormalized( const V3f& v, float epsilon )
{
	return mmAbs(V3_Length(v) - 1.0f) < epsilon;
}
mmINLINE const bool V3_AllGreaterThan(  const V3f& xyz, float value )
{
	return xyz.x > value && xyz.y > value && xyz.z > value;
}
mmINLINE const bool V3_AllGreaterOrEqual(  const V3f& xyz, float value )
{
	return xyz.x >= value && xyz.y >= value && xyz.z >= value;
}
mmINLINE const bool V3_AllInRangeInclusive(  const V3f& xyz, float _min, float _max )
{
	return xyz.x >= _min && xyz.x <= _max
		&& xyz.y >= _min && xyz.y <= _max
		&& xyz.z >= _min && xyz.z <= _max
		;
}

mmINLINE const V3f V3f_Floor( const V3f& xyz )
{
	return CV3f(
		::floorf( xyz.x ),
		::floorf( xyz.y ),
		::floorf( xyz.z )
	);
}

mmINLINE const V3f V3f_Ceiling( const V3f& xyz )
{
	return CV3f(
		::ceilf( xyz.x ),
		::ceilf( xyz.y ),
		::ceilf( xyz.z )
	);
}

mmINLINE const Int3 V3_Truncate( const V3f& xyz )
{
	return Int3(
		mmFloorToInt( xyz.x ),
		mmFloorToInt( xyz.y ),
		mmFloorToInt( xyz.z )
	);
}

mmINLINE const V3f V3_Clamped( const V3f& _xyz, const V3f& mins, const V3f& maxs )
{
	return CV3f(
		clampf( _xyz.x, mins.x, maxs.x ),
		clampf( _xyz.y, mins.y, maxs.y ),
		clampf( _xyz.z, mins.z, maxs.z )
	);
}

mmINLINE const Int3 Int3_Set( I32 x, I32 y, I32 z )
{
	return Int3( x, y, z );
}

mmINLINE const V3f Int3_To_V3F( const Int3& xyz )
{
	return CV3f( Int_To_Float(xyz.x), Int_To_Float(xyz.y), Int_To_Float(xyz.z) );
}

mmINLINE const UShort3 UShort3_Set( U16 x, U16 y, U16 z )
{
	return CUShort3( x, y, z );
}

mmINLINE const U11U11U10 U11U11U10_Set( unsigned x, unsigned y, unsigned z )
{
	const U11U11U10 result = { x, y, z };
	return result;
}

mmINLINE const Vector4 Vector4_Zero()
{
	//return g_MM_Zero;
	return _mm_setzero_ps();
}

/// Initialize a vector with a replicated floating point value.
mmINLINE const Vector4 Vector4_Replicate( float value )
{
#if MM_NO_INTRINSICS
	Vector4 result;
	result.x = value;
	result.y = value;
	result.z = value;
	result.w = value;
	return result;
#else
	return _mm_set_ps1( value );
#endif
}

mmINLINE const Vector4 Vector4_Set( const V3f& xyz, float w )
{
#if MM_NO_INTRINSICS
	Vector4 result;
	result.x = xyz.x;
	result.y = xyz.y;
	result.z = xyz.z;
	result.w = w;
	return result;
#else
	return _mm_set_ps( w, xyz.z, xyz.y, xyz.x );
#endif
}

mmINLINE const Vector4 Vector4_Set( const V4f& _xyzw )
{
	return Vector4_Set( _xyzw.x, _xyzw.y, _xyzw.z, _xyzw.w );
}

/// Loads four values, address aligned (MOVAPS).
mmINLINE const Vector4 Vector4_Load( const float* _xyzw ) {
	return _mm_load_ps( _xyzw );
}

mmINLINE const Vector4 Vector4_LoadFloat3_Unaligned( const float* _xyz )
{
	__m128 x = _mm_load_ss( _xyz + 0 );
	__m128 y = _mm_load_ss( _xyz + 1 );
	__m128 z = _mm_load_ss( _xyz + 2 );
	__m128 xy = _mm_unpacklo_ps( x, y );
	return _mm_movelh_ps( xy, z );
}

mmINLINE const Vector4 Vector4_Set( float x, float y, float z, float w )
{
#if MM_NO_INTRINSICS
	Vector4 result;
	result.x = x;
	result.y = y;
	result.z = z;
	result.w = w;
	return result;
#else
	return _mm_set_ps( w, z, y, x );
#endif
}

/// Return the X component in an FPU register.
/// This causes Load/Hit/Store on VMX targets.
mmINLINE const float Vector4_Get_X( Vec4Arg0 v )
{
#if MM_NO_INTRINSICS
	return v.x;
#else
	return _mm_cvtss_f32( v );
#endif
}

/// Return the Y component in an FPU register.
/// This causes Load/Hit/Store on VMX targets.
mmINLINE const float Vector4_Get_Y( Vec4Arg0 v )
{
#if MM_NO_INTRINSICS
	return v.y;
#else
	Vector4 vTemp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(1,1,1,1));
	return _mm_cvtss_f32(vTemp);
#endif
}

/// Return the Z component in an FPU register. 
/// This causes Load/Hit/Store on VMX targets.
mmINLINE const float Vector4_Get_Z( Vec4Arg0 v )
{
#if MM_NO_INTRINSICS
	return v.z;
#else
    Vector4 vTemp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(2,2,2,2));
    return _mm_cvtss_f32(vTemp);
#endif
}

/// Return the W component in an FPU register.
/// This causes Load/Hit/Store on VMX targets.
mmINLINE const float Vector4_Get_W( Vec4Arg0 v )
{
#if MM_NO_INTRINSICS
	return v.w;
#else
    Vector4 vTemp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(3,3,3,3));
    return _mm_cvtss_f32(vTemp);
#endif
}

/// Replicate the X component of the vector.
mmINLINE const Vector4 Vector4_SplatX( Vec4Arg0 v )
{
#if MM_NO_INTRINSICS
	V4f result;
	result.x = v.x;
	result.y = v.x;
	result.z = v.x;
	result.w = v.x;
	return result;
#else
    return _mm_shuffle_ps( v, v, _MM_SHUFFLE(0, 0, 0, 0) );
#endif
}

/// Replicate the Y component of the vector.
mmINLINE const Vector4 Vector4_SplatY( Vec4Arg0 v )
{
#if MM_NO_INTRINSICS
	Vector4 result;
	result.x = v.y;
	result.y = v.y;
	result.z = v.y;
	result.w = v.y;
	return result;
#else
    return _mm_shuffle_ps( v, v, _MM_SHUFFLE(1, 1, 1, 1) );
#endif
}

/// Replicate the Z component of the vector.
mmINLINE const Vector4 Vector4_SplatZ( Vec4Arg0 v )
{
#if MM_NO_INTRINSICS
	Vector4 result;
	result.x = v.z;
	result.y = v.z;
	result.z = v.z;
	result.w = v.z;
	return result;
#else
    return _mm_shuffle_ps( v, v, _MM_SHUFFLE(2, 2, 2, 2) );
#endif
}

/// Replicate the W component of the vector.
mmINLINE const Vector4 Vector4_SplatW( Vec4Arg0 v )
{
#if MM_NO_INTRINSICS
	Vector4 result;
	result.x = v.w;
	result.y = v.w;
	result.z = v.w;
	result.w = v.w;
	return result;
#else
    return _mm_shuffle_ps( v, v, _MM_SHUFFLE(3, 3, 3, 3) );
#endif
}
mmINLINE const Vector4 Vector4_LengthSquared( Vec4Arg0 v )
{
	return Vector4_Dot( v, v );
}
//mmINLINE const Vector4 Vector4_LengthV( Vec4Arg0 v )
//{
//	return Vector4_SqrtV( Vector4_LengthSquaredV( v ) );
//}
//mmINLINE const Vector4 Vector4_SqrtV( Vec4Arg0 v )
//{
//	mxTODO(:);
//    // if (x == +Infinity)  sqrt(x) = +Infinity
//    // if (x == +0.0f)      sqrt(x) = +0.0f
//    // if (x == -0.0f)      sqrt(x) = -0.0f
//    // if (x < 0.0f)        sqrt(x) = QNaN
//	Vector4 result;
//	result.x = sqrtf( v.x );
//	result.y = sqrtf( v.y );
//	result.z = sqrtf( v.z );
//	result.w = sqrtf( v.w );
//	return result;
//}
//mmINLINE const Vector4 Vector4_ReciprocalSqrtV( Vec4Arg0 v )
//{
//	mxTODO(:);
//    // if (x == +Infinity)  rsqrt(x) = 0
//    // if (x == +0.0f)      rsqrt(x) = +Infinity
//    // if (x == -0.0f)      rsqrt(x) = -Infinity
//    // if (x < 0.0f)        rsqrt(x) = QNaN
//	Vector4 result;
//	result.x = 1.0f / sqrtf( v.x );
//	result.y = 1.0f / sqrtf( v.y );
//	result.z = 1.0f / sqrtf( v.z );
//	result.w = 1.0f / sqrtf( v.w );
//	return result;
//}
//mmINLINE const Vector4 Vector4_ReciprocalLengthV( Vec4Arg0 v )
//{
//	float length = Vector4_LengthSquared( v );
//	// Prevent divide by zero.
//	if( length > 0.0f ) {
//		length = 1.0f/sqrtf(length);
//	}
//	return Vector4_Replicate(length);
//	//return Vector4_ReciprocalSqrtV( Vector4_LengthSquaredV( v ) );
//}
//mmINLINE const Vector4 Vector4_Normalized( Vec4Arg0 v )
//{
//	return Vector4_Multiply( Vector4_ReciprocalLengthV( v ), v );
//}
mmINLINE const Vector4 Vector4_Negate( Vec4Arg0 v )
{
#if MM_NO_INTRINSICS
	Vector4	result;
	result.x = -v.x;
	result.y = -v.y;
	result.z = -v.z;
	result.w = -v.w;
	return result;
#else
	Vector4 Z = _mm_setzero_ps();
	return _mm_sub_ps( Z, v );
#endif
}
mmINLINE const Vector4 Vector4_Scale( Vec4Arg0 _v, float _scale )
{
#if MM_NO_INTRINSICS
	Vector4	result;
	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	result.w = v.w * s;
	return result;
#else
	Vector4 vResult = _mm_set_ps1( _scale );
	return _mm_mul_ps( vResult, _v );
#endif
}
mmINLINE const Vector4 Vector4_Add( Vec4Arg0 _A, Vec4Arg0 _B )
{
#if MM_NO_INTRINSICS
	Vector4	result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	result.w = a.w + b.w;
	return result;
#else
	return _mm_add_ps( _A, _B );
#endif
}
mmINLINE const Vector4 Vector4_Subtract( Vec4Arg0 a, Vec4Arg0 b )
{
#if MM_NO_INTRINSICS
	Vector4	result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	result.w = a.w - b.w;
	return result;
#else
	return _mm_sub_ps( a, b );
#endif
}
/// Computes component-wise multiplication.
mmINLINE const Vector4 Vector4_Multiply( Vec4Arg0 a, Vec4Arg0 b )
{
#if MM_NO_INTRINSICS
	Vector4	result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	result.w = a.w * b.w;
	return result;
#else
	return _mm_mul_ps( a, b );
#endif
}
/// Computes a * b + c.
mmINLINE const Vector4 Vector4_MultiplyAdd( Vec4Arg0 a, Vec4Arg0 b, Vec4Arg0 c )
{
#if MM_NO_INTRINSICS
	Vector4	result;
	result.x = a.x * b.x + c.x;
	result.y = a.y * b.y + c.y;
	result.z = a.z * b.z + c.z;
	result.w = a.w * b.w + c.w;
	return result;
#else
	Vector4 vResult = _mm_mul_ps( a, b );
    return _mm_add_ps(vResult, c );
#endif
}

/// Returns the cross product (aka 'vector product') of a and b.
/// This operation is defined in respect to a right-handed coordinate system,
/// i.e. the direction of the resulting vector can be found by the "Right Hand Rule").
mmINLINE const Vector4 Vector4_Cross3( Vec4Arg0 a, Vec4Arg0 b )
{
	// result.x = (a.y * b.z) - (a.z * b.y);
	// result.y = (a.z * b.x) - (a.x * b.z);
	// result.z = (a.x * b.y) - (a.y * b.x);

	const __m128 a_wxzy = mmPERMUTE_PS( a, _MM_SHUFFLE(3,0,2,1) );
	const __m128 a_wyxz = mmPERMUTE_PS( a, _MM_SHUFFLE(3,1,0,2) );

	const __m128 b_wxzy = mmPERMUTE_PS( b, _MM_SHUFFLE(3,0,2,1) );
	const __m128 b_wyxz = mmPERMUTE_PS( b, _MM_SHUFFLE(3,1,0,2) );

	const __m128 result = _mm_sub_ps( _mm_mul_ps( a_wxzy, b_wyxz ), _mm_mul_ps( a_wyxz, b_wxzy ) );
	return _mm_and_ps( result, g_MM_Mask3.m128 );	// Set w to zero
}

#if 0
mmINLINE const bool Vector4_IsNaN( Vec4Arg0 v )
{
	return mmIS_NAN(v.x) || mmIS_NAN(v.y) || mmIS_NAN(v.z) || mmIS_NAN(v.w);
}
mmINLINE const bool Vector4_IsInfinite( Vec4Arg0 v )
{
	return mmIS_INF(v.x) || mmIS_INF(v.y) || mmIS_INF(v.z) || mmIS_INF(v.w);
}
mmINLINE const bool Vector4_Equal( Vec4Arg0 a, Vec4Arg0 b )
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
mmINLINE const bool Vector4_NotEqual( Vec4Arg0 a, Vec4Arg0 b )
{
	return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
}
#endif

mmINLINE const Vector4 Vector4_Normalized( Vec4Arg0 V )
{
#if MM_NO_INTRINSICS
#else
    // Perform the dot product on x,y and z only
    Vector4 vLengthSq = _mm_mul_ps(V,V);
    Vector4 vTemp = mmPERMUTE_PS(vLengthSq,_MM_SHUFFLE(2,1,2,1));
    vLengthSq = _mm_add_ss(vLengthSq,vTemp);
    vTemp = mmPERMUTE_PS(vTemp,_MM_SHUFFLE(1,1,1,1));
    vLengthSq = _mm_add_ss(vLengthSq,vTemp);
    vLengthSq = mmPERMUTE_PS(vLengthSq,_MM_SHUFFLE(0,0,0,0));
    // Prepare for the division
    Vector4 vResult = _mm_sqrt_ps(vLengthSq);
    // Create zero with a single instruction
    Vector4 vZeroMask = _mm_setzero_ps();
    // Test for a divide by zero (Must be FP to detect -0.0)
    vZeroMask = _mm_cmpneq_ps(vZeroMask,vResult);
    // Failsafe on zero (Or epsilon) length planes
    // If the length is infinity, set the elements to zero
    vLengthSq = _mm_cmpneq_ps(vLengthSq,g_MM_Infinity.m128);
    // Divide to perform the normalization
    vResult = _mm_div_ps(V,vResult);
    // Any that are infinity, set to zero
    vResult = _mm_and_ps(vResult,vZeroMask);
    // Select qnan or result based on infinite length
    Vector4 vTemp1 = _mm_andnot_ps(vLengthSq,g_MM_QNaN.m128);
    Vector4 vTemp2 = _mm_and_ps(vResult,vLengthSq);
    vResult = _mm_or_ps(vTemp1,vTemp2);
    return vResult;
#endif
}

mmINLINE const bool Quaternion_Equal( Vec4Arg0 a, Vec4Arg0 b )
{
	return Vector4_Equal( a, b );
}
mmINLINE const bool Quaternion_NotEqual( Vec4Arg0 a, Vec4Arg0 b )
{
	return Vector4_NotEqual( a, b );
}
mmINLINE const bool Quaternion_IsNaN( Vec4Arg0 q )
{
	return Vector4_IsNaN( q );
}
mmINLINE const bool Quaternion_IsInfinite( Vec4Arg0 q )
{
	return Vector4_IsInfinite( q );
}

//mmINLINE const Vector4 Quaternion_Dot( Vec4Arg0 a, Vec4Arg0 b )
//{
//	return Vector4_DotV( a, b );
//}
/// Rotation 'a' is followed by 'b'
mmINLINE const Vector4 Quaternion_Multiply( Vec4Arg0 a, Vec4Arg0 b )
{
	const V4f& va = (V4f&) a;
	const V4f& vb = (V4f&) b;
	return Vector4_Set(
		(vb.w * va.x) + (vb.x * va.w) + (vb.y * va.z) - (vb.z * va.y),
		(vb.w * va.y) - (vb.x * va.z) + (vb.y * va.w) + (vb.z * va.x),
		(vb.w * va.z) + (vb.x * va.y) - (vb.y * va.x) + (vb.z * va.w),
		(vb.w * va.w) - (vb.x * va.x) - (vb.y * va.y) - (vb.z * va.z)
	);
}
mmINLINE const Vector4 Quaternion_Normalize( Vec4Arg0 q )
{
	return Vector4_Normalized( q );
}
//mmINLINE const Vector4 Quaternion_Conjugate( Vec4Arg0 q )
//{
//	Vector4 result;
//	result.x = -q.x;
//	result.y = -q.y;
//	result.z = -q.z;
//	result.w = q.w;
//	return result;
//}

// NOTE: operator overloads should be defined only for 'non-vectorizable' types (V2f, V3f).
// NOTE: operator overloads must be unambiguous!

#if MM_OVERLOAD_OPERATORS

mmINLINE const V4f operator + ( const V4f& a, const V4f& b ) {
	const V4f result = {
		a.x + b.x,
		a.y + b.y,
		a.z + b.z,
		a.w + b.w
	};
	return result;
}
mmINLINE const V4f operator - ( const V4f& a, const V4f& b ) {
	const V4f result = {
		a.x - b.x,
		a.y - b.y,
		a.z - b.z,
		a.w - b.w
	};
	return result;
}

mmINLINE V4f& operator += ( V4f& a, const V4f& b ) {
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	a.w += b.w;
	return a;
}
mmINLINE V4f& operator -= ( V4f& a, const V4f& b ) {
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	a.w -= b.w;
	return a;
}
mmINLINE V4f& operator *= ( V4f& v, const float scale ) {
	v.x *= scale;
	v.y *= scale;
	v.z *= scale;
	v.w *= scale;
	return v;
}
mmINLINE V4f& operator /= ( V4f& v, const float scale ) {
	float inverse = mmRcp( scale );
	v.x *= inverse;
	v.y *= inverse;
	v.z *= inverse;
	v.w *= inverse;
	return v;
}

/// dot product.
mmINLINE const float operator * ( const V3f& a, const V3f& b ) {
	return V3_Dot( a, b );
}
///// Cross product (also called 'Wedge product').
//mmINLINE const V3f operator ^ ( const V3f& a, const V3f& b ) {
//	return V3_Cross( a, b );
//}

#endif // MM_OVERLOAD_OPERATORS
