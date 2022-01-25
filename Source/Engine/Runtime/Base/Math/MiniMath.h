/*
=============================================================================
	File:	MiniMath.h
	Desc:	The Mini-Math library.
	Based on XNAMath/DirectXMath.
=============================================================================
*/
#pragma once



#define MM_ENABLE_REFLECTION	(1)

// overload [] and arithmetic operators for convenience
#define MM_OVERLOAD_OPERATORS	(1)

//#define MM_OVERLOAD_ARITHMETIC_OPERATORS	(1)

// overload << and >>
#define MM_DEFINE_STREAM_OPERATORS	(1)

/// this is used to avoid warning C4316 : object allocated on the heap may not be aligned 16
#define MM_OVERRIDE_NEW_AND_DELETE	(1)

//#define MM_FORBID_NEW_AND_DELETE	(1)

// std::min<>, std::max<>
#include <limits>
#include <algorithm>


// reflection
#if MM_ENABLE_REFLECTION
#include <Base/Object/BaseType.h>
#include <Base/Object/Reflection/ClassDescriptor.h>
#endif


/*
=======================================================================
	DEFINES
=======================================================================
*/

#if MM_OVERRIDE_NEW_AND_DELETE
	// this is used to avoid warning C4316 : object allocated on the heap may not be aligned 16
	#define mmDECLARE_ALIGNED_ALLOCATOR16( TYPE )\
		  void * operator new      ( size_t size )             { return mxALLOC(size, 16); }\
		  void   operator delete   ( void * ptr )              { return mxFREE(ptr, sizeof(TYPE)); }\
		  void * operator new[]    ( size_t size )             { return mxALLOC(size, 16); }\
		  void   operator delete[] ( void* ptr )               { return mxFREE(ptr, sizeof(TYPE)); }\
		  void * operator new      ( size_t, void* _where )    { return (_where); }\
		  void  operator delete    ( void*, void*)             {}\
		  void* operator new[]     ( size_t, void* _where )    { return (_where); }\
		  void  operator delete[]  ( void*, void* )            {}
#else
	#define mmDECLARE_ALIGNED_ALLOCATOR16( TYPE )
#endif


template< typename Scalar >
Scalar getNormalEpsilon();

template<>
mmINLINE float getNormalEpsilon<float>() { return 1e-6f; }

template<>
mmINLINE double getNormalEpsilon<double>() { return 1e-12f; }


/*
=======================================================================
	LOW-LEVEL TYPES
=======================================================================
*/

typedef U16 Half;


/*
=======================================================================
	LOW-LEVEL FUNCTION PROTOTYPES
=======================================================================
*/

//=====================================================================
//	SCALAR OPERATIONS
//=====================================================================

float mmRcp( float x );	// reciprocal = 1/x
float mmAbs( float x );
double mmAbs( double x );
float mmFloor( float x );
float mmCeiling( float x );
float mmModulo( float x, float y );
int mmRoundToInt( float x );
int mmFloorToInt( float x );
float mmExp2f( float power );
float mmCopySign( float magnitude, float sign );
BYTE mmFloatToByte( float x );


float mmSqrt( float x );
/// reciprocal square root
float mmInvSqrt( float x );
float mmInvSqrtEst( float x );
/// reciprocal square root
double mmInvSqrt( double x );

void mmSinCos( float x, float &s, float &c );
float mmSin( float x );
float mmCos( float x );
float mmTan( float x );
float mmASin( float x );
float mmACos( float x );
float mmATan( float x );
float mmATan2( float y, float x );

float mmExp( float x );
float mmLn( float x );
float mmLog10( float x );
float mmLog( float value, float _base );
float mmPow( float x, float y );
float mmLerp( float from, float to, float amount );
float mmLerp3( float a, float b, float t );

/// Returns the base 2 logarithm of x.
double log2( double x );

float Int_To_Float( int x );

// Floored Signed Integer division.
static inline
int floor_div( int a, int b )
{
	int q = a / b;
	int r = a % b;
	if( r != 0 && ((r < 0) != (b < 0)) ) {
		q--;
	}
	return q;
}

/// Signed Integer division with ceiling
/// X can be both positive and negative.
/// Y must be positive.
static inline
int ceil_div_posy( int x, int y ) {
	return x / y + (x % y > 0);	// the modulo and the division can be performed using the same CPU instruction
	// Taken from: http://stackoverflow.com/a/30824434
	// If x is positive then division is towards zero, and we should add 1 if reminder is not zero.
	// If x is negative then division is towards zero, that's what we need, and we will not add anything because x % y is not positive.
}
static inline
unsigned div_ceil( unsigned x, unsigned y ) {
	return (x + y - 1) / y;
}


template< typename Scalar >
struct Tuple2
{
	union {
		struct {
			Scalar	x;
			Scalar	y;
		};
		Scalar		a[2];
	};
public:
	mmINLINE Tuple2()
	{
	}
	mmINLINE Tuple2( Scalar x, Scalar y )
	{
		this->x = x;
		this->y = y;
	}
	mmINLINE explicit Tuple2( Scalar value )
	{
		this->x = value;
		this->y = value;
	}

	mmINLINE static const Tuple2 min( const Tuple2& a, const Tuple2& b )
	{
		return Tuple2(
			std::min( a.x, b.x ),
			std::min( a.y, b.y )
		);
	}
	mmINLINE static const Tuple2 max( const Tuple2& a, const Tuple2& b )
	{
		return Tuple2(
			std::max( a.x, b.x ),
			std::max( a.y, b.y )
		);
	}

#if MM_OVERLOAD_OPERATORS

	mmINLINE Scalar& operator [] ( int i )		{ return a[i]; }
	mmINLINE Scalar operator [] ( int i ) const	{ return a[i]; }

	friend mmINLINE bool operator == ( const Tuple2& a, const Tuple2& b ) {
		return (a.x == b.x) && (a.y == b.y);
	}
	friend mmINLINE bool operator != ( const Tuple2& a, const Tuple2& b ) {
		return !(a == b);
	}

#endif	// MM_OVERLOAD_OPERATORS
};

// FIXME: it's struct and not union, because it's used as a base class
template< typename Scalar >
struct Tuple3
{
	union {
		struct {
			Scalar	x;
			Scalar	y;
			Scalar	z;
		};
		Scalar		a[3];
	};
public:
	// use CTuple3<> for convenient ctors

	mmINLINE static Tuple3 make( Scalar x, Scalar y, Scalar z )
	{
		const Tuple3 result = { x, y, z };
		return result;
	}

	mxDEPRECATED
	mmINLINE static Tuple3 set( Scalar x, Scalar y, Scalar z ) {
		const Tuple3 result = { x, y, z };
		return result;
	}
	mmINLINE static Tuple3 setAll( const Scalar value ) {
		const Tuple3 result = { value, value, value };
		return result;
	}
	mmINLINE static Tuple3 zero() {
		return setAll( 0 );
	}

	template< class XYZ >
	mmINLINE void setFrom( const XYZ& xyz ) {
		this->x = xyz.x;
		this->y = xyz.y;
		this->z = xyz.z;
	}
	template< class XYZ >
	mmINLINE static Tuple3 fromXYZ( const XYZ& xyz ) {
		return set( xyz.x, xyz.y, xyz.z );
	}

	mmINLINE static Tuple3 fromArray( const Scalar xyz[3] ) {
		return Tuple3::set( xyz[0], xyz[1], xyz[2] );
	}

	/// returns the volume of a box with dimensions X,Y,Z
	mmINLINE const Scalar boxVolume() const {
		return x * y * z;
	}


	mmINLINE const Scalar minComponent() const {
		return Min3( x, y, z );
	}
	mmINLINE const Scalar maxComponent() const {
		return Max3( x, y, z );
	}

	// Swizzling
	mmINLINE const Tuple3 xyz() const {
		return *this;
	}
	mmINLINE const Tuple3 yzx() const {
		return Tuple3( y, z, x );
	}
	mmINLINE const Tuple3 zxy() const {
		return Tuple3( z, x, y );
	}

	mmINLINE bool allLessThan( const Scalar value ) const {
		return x < value && y < value && z < value;
	}
	mmINLINE bool AnyLessThan( const Scalar& value ) const {
		return (x < value) || (y < value) || (z < value);
	}
	mmINLINE bool AnyGreaterThan( const Scalar& value ) const {
		return (x > value) || (y > value) || (z > value);
	}
	mmINLINE bool allLessOrEqualTo( const Scalar value ) const {
		return x <= value && y <= value && z <= value;
	}
	mmINLINE bool allGreaterThan( const Scalar value ) const {
		return x > value && y > value && z > value;
	}
	mmINLINE bool allInRangeInc( const Scalar _min, const Scalar _max ) const {
		return x >= _min && y >= _min && z >= _min
			&& x <= _max && y <= _max && z <= _max;
	}

	mmINLINE static bool lessThan( const Tuple3& a, const Tuple3& b ) {
		return a.x < b.x && a.y < b.y && a.z < b.z;
	}
	mmINLINE static bool lessOrEqualTo( const Tuple3& a, const Tuple3& b ) {
		return a.x <= b.x && a.y <= b.y && a.z <= b.z;
	}
	mmINLINE static bool greaterThan( const Tuple3& a, const Tuple3& b ) {
		return a.x > b.x && a.y > b.y && a.z > b.z;
	}
	mmINLINE static bool greaterOrEqualTo( const Tuple3& a, const Tuple3& b ) {
		return a.x >= b.x && a.y >= b.y && a.z >= b.z;
	}

	mmINLINE static const Tuple3 min( const Tuple3& a, const Tuple3& b ) 
	{
		return set(
			std::min( a.x, b.x ),
			std::min( a.y, b.y ),
			std::min( a.z, b.z )
		);
	}

	mmINLINE static const Tuple3 max( const Tuple3& a, const Tuple3& b )
	{
		return set(
			std::max( a.x, b.x ),
			std::max( a.y, b.y ),
			std::max( a.z, b.z )
		);
	}

	mmINLINE static const Tuple3 clamped( const Tuple3& _o, const Tuple3& mins, const Tuple3& maxs )
	{
		return set(
			Clamp( _o.x, mins.x, maxs.x ),
			Clamp( _o.y, mins.y, maxs.y ),
			Clamp( _o.z, mins.z, maxs.z )
		);
	}

	mmINLINE static Tuple3 abs( const Tuple3& v )
	{
		const Tuple3 result = { Abs(v.x), Abs(v.y), Abs(v.z) };
		return result;
	}

	/// Returns the dot product (aka 'scalar product') of a and b.
	mmINLINE static const Scalar dot( const Tuple3& a, const Tuple3& b ) {
		return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
	}

	/// The direction of the cross product follows the right hand rule.
	///definition: http://www.euclideanspace.com/maths/algebra/vectors/vecAlgebra/cross/index.htm
	mmINLINE static const Tuple3 cross( const Tuple3& a, const Tuple3& b ) {
		Tuple3 result;
		result.x = (a.y * b.z) - (a.z * b.y);
		result.y = (a.z * b.x) - (a.x * b.z);
		result.z = (a.x * b.y) - (a.y * b.x);
		return result;
	}

	mmINLINE static const Tuple3 scale( const Tuple3& v, const Scalar scaling ) {
		return Tuple3::set( v.x * scaling, v.y * scaling, v.z * scaling );
	}
	mmINLINE static const Tuple3 mul( const Tuple3& a, const Tuple3& b ) {
		return Tuple3::set( a.x * b.x, a.y * b.y, a.z * b.z );
	}

	// used for dividing integer coords by two
	mmINLINE static const Tuple3 div( const Tuple3& a, const Tuple3& b ) {
		return Tuple3::set( a.x / b.x, a.y / b.y, a.z / b.z );
	}

	mmINLINE static const Tuple3 div( const Tuple3& a, const Scalar s ) {
		return Tuple3::set( a.x / s, a.y / s, a.z / s );
	}

	mmINLINE const Scalar lengthSquared() const {
		return dot( *this, *this );
	}

	mmINLINE const Scalar length() const {
		return sqrt( lengthSquared() );
	}

	mmINLINE const bool isNormalized( Scalar epsilon = getNormalEpsilon<Scalar>() ) const
	{
		return ::fabs( this->lengthSquared() - Scalar(1) ) < epsilon;
	}

	// returns length
	mmINLINE Scalar normalize()
	{
		const Scalar squared_length = this->lengthSquared();
		if( squared_length == (Scalar)0 ) {
			return (Scalar)0;
		}
		const Scalar recip_length = mmInvSqrt( squared_length );
		*this *= recip_length;
		return recip_length * squared_length;
	}

	mmINLINE const Tuple3 normalized() const
	{
		const Scalar squared_length = this->lengthSquared();
		if( squared_length == (Scalar)0 ) {
			return Tuple3::setAll( 0 );
		}
		const Scalar recip_length = mmInvSqrt( squared_length );
		return *this * recip_length;
	}

	mmINLINE Scalar normalizeNonZeroLength()
	{
		const Scalar squared_length = this->lengthSquared();
		const Scalar recip_length = mmInvSqrt( squared_length );
		*this *= recip_length;
		return recip_length * squared_length;
	}

	mmINLINE static bool equal( const Tuple3& a, const Tuple3& b, const Scalar epsilon )
	{
		if( mmAbs( a.x - b.x ) > epsilon ) {
			return false;
		}
		if( mmAbs( a.y - b.y ) > epsilon ) {
			return false;
		}
		if( mmAbs( a.z - b.z ) > epsilon ) {
			return false;
		}
		return true;
	}

	mmINLINE bool equals( const Tuple3& other, const Scalar epsilon ) const
	{
		return equal( *this, other, epsilon );
	}

#if MM_OVERLOAD_OPERATORS
	mmINLINE Scalar& operator [] ( int i )		{ return a[i]; }
	mmINLINE Scalar operator [] ( int i ) const	{ return a[i]; }

	friend mmINLINE bool operator == ( const Tuple3& a, const Tuple3& b ) {
		return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
	}
	friend mmINLINE bool operator != ( const Tuple3& a, const Tuple3& b ) {
		return !(a == b);
	}

	/// Returns the dot product (aka 'scalar product').
	mmINLINE Scalar operator | ( const Tuple3& other ) const {
		return (this->x * other.x) + (this->y * other.y) + (this->z * other.z);
	}
	/// cross product (also called 'Wedge product').
	/// The direction of the cross product follows the right hand rule.
	mmINLINE Tuple3 operator ^ ( const Tuple3& other ) const {
		return Tuple3::set(
			(this->y * other.z) - (this->z * other.y),
			(this->z * other.x) - (this->x * other.z),
			(this->x * other.y) - (this->y * other.x)
		);
	}

	/// Lexicographic comparison
	friend const bool operator < ( const Tuple3& a, const Tuple3& b ) {
		if( a.z < b.z ) {
			return true;
		} else if( a.z > b.z ) {
			return false;
		}
		if( a.y < b.y ) {
			return true;
		} else if( a.y > b.y ) {
			return false;
		}
		if( a.x < b.x ) {
			return true;
		} else if( a.x > b.x ) {
			return false;
		}
		return false;
	}
	friend const bool operator > ( const Tuple3& a, const Tuple3& b ) {
		if( a.z > b.z ) {
			return true;
		} else if( a.z < b.z ) {
			return false;
		}
		if( a.y > b.y ) {
			return true;
		} else if( a.y < b.y ) {
			return false;
		}
		if( a.x > b.x ) {
			return true;
		} else if( a.x < b.x ) {
			return false;
		}
		return false;
	}

	// Overload Unary + and - operators.
	mmINLINE const Tuple3 operator - () const {
		return Tuple3::set( -x, -y, -z );
	}
	mmINLINE const Tuple3 operator + () const {
		return Tuple3::set( +x, +y, +z );
	}
#endif // MM_OVERLOAD_OPERATORS
};

template< typename Scalar >
struct CTuple3: Tuple3< Scalar >
{
	mmINLINE CTuple3() {
	}
	mmINLINE CTuple3( const Tuple3< Scalar >& other ) {
		this->x = other.x;
		this->y = other.y;
		this->z = other.z;
	}
	mmINLINE CTuple3( const Tuple2< Scalar >& xy, Scalar z ) {
		this->x = xy.x;
		this->y = xy.y;
		this->z = z;
	}
	mmINLINE CTuple3( Scalar x, Scalar y, Scalar z ) {
		this->x = x;
		this->y = y;
		this->z = z;
	}
	mmINLINE explicit CTuple3( Scalar value ) {
		this->x = value;
		this->y = value;
		this->z = value;
	}
};

template< typename Scalar >
struct Tuple4
{
	union {
		struct {
			Scalar	x;
			Scalar	y;
			Scalar	z;
			Scalar	w;
		};
		Scalar		a[4];
	};
public:
//	mmDECLARE_ALIGNED_ALLOCATOR16(Tuple4);

	// Constructors are disabled, because vec4's are put into unions.
#if 0
	mmINLINE Tuple4()
	{
	}
	mmINLINE Tuple4( Scalar x, Scalar y, Scalar z, Scalar w )
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}
	mmINLINE explicit Tuple4( Scalar value )
	{
		this->x = value;
		this->y = value;
		this->z = value;
		this->w = value;
	}
#endif
	mmINLINE static const Tuple4 set( Scalar x, Scalar y, Scalar z, Scalar w ) {
		Tuple4	result = { x, y, z, w };
		return result;
	}
	mmINLINE static const Tuple4 set( const Tuple3<Scalar>& _xyz, Scalar w ) {
		Tuple4	result = { _xyz.x, _xyz.y, _xyz.z, w };
		return result;
	}
	mmINLINE static const Tuple4 replicate( const Scalar value ) {
		Tuple4	result = { value, value, value, value };
		return result;
	}
	mmINLINE static const Tuple4 Zero() {
		return replicate( 0 );
	}
	template< class XYZW >
	mmINLINE static Tuple4 fromXYZW( const XYZW& _xyzw ) {
		return Tuple4::set( _xyzw.x, _xyzw.y, _xyzw.z, _xyzw.w );
	}
	template< class XYZW >
	mmINLINE void setFrom( const XYZW& _xyzw ) {
		this->x = _xyzw.x;
		this->y = _xyzw.y;
		this->z = _xyzw.z;
		this->w = _xyzw.w
	}

	/// Returns the dot product (aka 'scalar product') of a and b.
	mmINLINE static const Scalar dot( const Tuple4& a, const Tuple4& b ) {
		return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
	}

	mmINLINE Tuple3<Scalar>& asVec3() {
		return *reinterpret_cast< Tuple3<Scalar>* >( this );
	}
	mmINLINE const Tuple3<Scalar>& asVec3() const {
		return *reinterpret_cast< const Tuple3<Scalar>* >( this );
	}

	mmINLINE const Tuple4 negated() const {
		return Tuple4::set( -x, -y, -z, -w );
	}

	mmINLINE static const Tuple4 add( const Tuple4& a, const Tuple4& b ) {
		return CTuple4<Scalar>( a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w );
	}
	mmINLINE static const Tuple4 sub( const Tuple4& a, const Tuple4& b ) {
		return CTuple4<Scalar>( a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w );
	}
	mmINLINE static const Tuple4 mul( const Tuple4& a, const Tuple4& b ) {
		return CTuple4<Scalar>( a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w );
	}
	mmINLINE static const Tuple4 scale( const Tuple4& a, const Scalar s ) {
		return CTuple4<Scalar>( a.x * s, a.y * s, a.z * s, a.w * s );
	}
	mmINLINE static const Tuple4 div( const Tuple4& a, const Tuple4& b ) {
		return CTuple4<Scalar>( a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w );
	}

	mmINLINE static const Tuple4 lerp( const Tuple4& a, const Tuple4& b
		, const Scalar fraction )
	{
		if ( fraction <= 0.0f ) {
			return a;
		} else if ( fraction >= 1.0f ) {
			return b;
		} else {
			return add( a, scale( sub( b, a ), fraction ) );
		}
	}

#if MM_OVERLOAD_OPERATORS
	mmINLINE Scalar& operator [] ( int i )		{ return a[i]; }
	mmINLINE Scalar operator [] ( int i ) const	{ return a[i]; }

	friend mmINLINE bool operator == ( const Tuple4& a, const Tuple4& b ) {
		return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
	}
	friend mmINLINE bool operator != ( const Tuple4& a, const Tuple4& b ) {
		return !(a == b);
	}
	/// Lexicographic comparison
	friend const bool operator < ( const Tuple4& a, const Tuple4& b )
	{
		if( a.w < b.w ) {
			return true;
		} else if( a.w > b.w ) {
			return false;
		}
		if( a.z < b.z ) {
			return true;
		} else if( a.z > b.z ) {
			return false;
		}
		if( a.y < b.y ) {
			return true;
		} else if( a.y > b.y ) {
			return false;
		}
		if( a.x < b.x ) {
			return true;
		} else if( a.x > b.x ) {
			return false;
		}
		return false;
	}
	friend const bool operator > ( const Tuple4& a, const Tuple4& b )
	{
		if( a.w > b.w ) {
			return true;
		} else if( a.w < b.w ) {
			return false;
		}
		if( a.z > b.z ) {
			return true;
		} else if( a.z < b.z ) {
			return false;
		}
		if( a.y > b.y ) {
			return true;
		} else if( a.y < b.y ) {
			return false;
		}
		if( a.x > b.x ) {
			return true;
		} else if( a.x < b.x ) {
			return false;
		}
		return false;
	}
#endif // MM_OVERLOAD_OPERATORS
};

template< typename Scalar >
struct CTuple4: Tuple4< Scalar >
{
	mmINLINE CTuple4() {
	}
	mmINLINE CTuple4( const Tuple4< Scalar >& other ) {
		this->x = other.x;
		this->y = other.y;
		this->z = other.z;
		this->w = other.w;
	}
	mmINLINE CTuple4( Scalar x, Scalar y, Scalar z, Scalar w ) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}
	mmINLINE explicit CTuple4( const Tuple3< Scalar >& xyz, Scalar w) {
		this->x = xyz.x;
		this->y = xyz.y;
		this->z = xyz.z;
		this->w = w;
	}
	mmINLINE explicit CTuple4( Scalar value ) {
		this->x = value;
		this->y = value;
		this->z = value;
		this->w = value;
	}
};


#if MM_OVERLOAD_OPERATORS

template< typename T >
mmINLINE const Tuple3<T> operator + ( const Tuple3<T>& a, const Tuple3<T>& b ) {
	return CTuple3<T>( a.x + b.x, a.y + b.y, a.z + b.z );
}
template< typename T >
mmINLINE const Tuple3<T> operator - ( const Tuple3<T>& a, const Tuple3<T>& b ) {
	return CTuple3<T>( a.x - b.x, a.y - b.y, a.z - b.z );
}

template< typename T >
mmINLINE const Tuple3<T> operator * ( const Tuple3<T>& v, const T value ) {
	return CTuple3<T>( v.x * value, v.y * value, v.z * value );
}
template< typename T >
mmINLINE const Tuple3<T> operator / ( const Tuple3<T>& v, const T value ) {
	return CTuple3<T>( v.x / value, v.y / value, v.z / value );
}

template< typename T >
mmINLINE Tuple3<T>& operator += ( Tuple3<T> & a, const Tuple3<T>& b ) {
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	return a;
}

template< typename T >
mmINLINE Tuple3<T>& operator -= ( Tuple3<T> & a, const Tuple3<T>& b ) {
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	return a;
}

template< typename T >
mmINLINE Tuple3<T>& operator *= ( Tuple3<T> & v, const T scale ) {
	v.x *= scale;
	v.y *= scale;
	v.z *= scale;
	return v;
}
template< typename T >
mmINLINE Tuple3<T>& operator /= ( Tuple3<T>& v, const T value ) {
	v.x /= value;
	v.y /= value;
	v.z /= value;
	return v;
}

//-----------------------------------------------------------------
// Specializations for float vectors
//-----------------------------------------------------------------
template<>
mmINLINE const Tuple3<float> operator / ( const Tuple3<float>& v, const float value ) {
	float inverse = mmRcp( value );
	return CTuple3<float>(
		v.x * inverse,
		v.y * inverse,
		v.z * inverse
	);
}
template<>
mmINLINE Tuple3<float>& operator /= ( Tuple3<float> & v, const float value ) {
	float inverse = mmRcp( value );
	v.x *= inverse;
	v.y *= inverse;
	v.z *= inverse;
	return v;
}

#endif // MM_OVERLOAD_OPERATORS


/// 2D Vector; 32 bit floating point components
typedef Tuple2< float >		V2f;
typedef Tuple2< double >	V2d;

/// 3D Vector; 32 bit floating point components
typedef Tuple3< float >		V3f;
typedef CTuple3< float >	CV3f;

/// 4D Vector; 32 bit floating point components
typedef Tuple4< float >		V4f;
typedef CTuple4< float >	CV4f;

// double-precision vectors are used for large worlds (like planets)
typedef Tuple2< double >	V2d;
typedef Tuple3< double >	V3d;
typedef Tuple4< double >	V4d;

typedef CTuple3< double >	CV3d;
typedef CTuple4< double >	CV4d;

/*
=======================================================================
	HARDWARE VECTOR TYPE
=======================================================================
*/

/// 8D Vector; 32 bit floating point components
mxPREALIGN(32) struct V8F4
{
	float	x, y, z, w;
	float	p, q, r, s;

#if MM_OVERLOAD_OPERATORS
	mmINLINE float& operator [] ( int i ) { return (&x)[i]; }
	mmINLINE float operator [] ( int i ) const { return (&x)[i]; }
#endif
	//mmDECLARE_ALIGNED_ALLOCATOR16(V8F4);
};

#if !MM_NO_INTRINSICS

	// Define 128-bit wide 16-byte aligned hardware register type.
	typedef __m128			Vector4;

	// Fall back to 256-bit wide 4D floating point vector type.
	typedef V8F4			Vector8;

#else

	// Fall back to 128-bit wide 4D floating point vector type.
	typedef V4f				Vector4;

	// Fall back to 256-bit wide 4D floating point vector type.
	typedef V8F4			Vector8;

#endif


// Typedefs for vector parameter types (for passing vectors to functions).
// Microsoft recommends passing the __m128 data type by reference to functions.
// This is the behavior of the XNA Math Library.

// Fix-up for (1st-3rd) Vector4 parameters that are pass-in-register for x86, ARM, and Xbox 360; by reference otherwise
#if ( defined(_M_IX86) || defined(_M_ARM) || defined(_mmVMX128_INTRINSICS_) ) && !defined(_mmNO_INTRINSICS_)
typedef const Vector4 Vec4Arg0;
#else
typedef const Vector4& Vec4Arg0;
#endif

// Fix-up for (4th) Vector4 parameter to pass in-register for ARM and Xbox 360; by reference otherwise
#if ( defined(_M_ARM) || defined(_mmVMX128_INTRINSICS_) ) && !defined(_mmNO_INTRINSICS_)
typedef const Vector4 Vec4Arg4;
#else
typedef const Vector4& Vec4Arg4;
#endif

// Fix-up for (5th+) Vector4 parameters to pass in-register for Xbox 360 and by reference otherwise
#if defined(_mmVMX128_INTRINSICS_) && !defined(_mmNO_INTRINSICS_)
typedef const Vector4 Vec4Arg5;
#else
typedef const Vector4& Vec4Arg5;
#endif

#if !MM_NO_INTRINSICS
#define mmPERMUTE_PS( V, C )	_mm_shuffle_ps( V, V, C )
#define mmSPLAT_X( V )			mmPERMUTE_PS( V, _MM_SHUFFLE(0,0,0,0) )
#define mmSPLAT_Y( V )			mmPERMUTE_PS( V, _MM_SHUFFLE(1,1,1,1) )
#define mmSPLAT_Z( V )			mmPERMUTE_PS( V, _MM_SHUFFLE(2,2,2,2) )
#define mmSPLAT_W( V )			mmPERMUTE_PS( V, _MM_SHUFFLE(3,3,3,3) )
#endif

mxPREALIGN(16) union Vector4i
{
	I32		i[4];	//!< [0..3]: [X,Y,Z,W] NOTE: must be first!
    __m128	m128;
	__m128i	m128i;
	__m128d	m128d;    
};
mxPREALIGN(16) union Vector4u
{
	U32		i[4];	//!< [0..3]: [X,Y,Z,W] NOTE: must be first!
    Vector4	q;    
};
mxPREALIGN(16) union Vector4f
{
	float	f[4];	//!< [0..3]: [X,Y,Z,W] NOTE: must be first!
	Vector4	v;	
};






template< typename Scalar >
struct TSIMDVector
{
	typedef Tuple4< Scalar >	Vector4Type;
};

template<>
struct TSIMDVector< float >
{
	typedef Vector4	Vector4Type;
};






union UF64 {
	U64	i;
	F64	f;
	struct {
		unsigned long long mantissa : 52;
		unsigned long long exponent : 11;
		unsigned long long sign : 1;
	} parts;
};

struct V2I4
{
	I32	x;
	I32	y;
};
struct V2U4
{
	U32	x;
	U32	y;
};
typedef Tuple3< U16 >	UShort3;
typedef CTuple3< U16 >	CUShort3;

union UShort4
{
	struct {
		U16	x, y, z, w;
	};
	U64	v;
};

union U11U11U10
{
	struct {
		unsigned x : 11;
		unsigned y : 11;
		unsigned z : 10;
	};
	U32	v;
};
mxSTATIC_ASSERT(sizeof(U11U11U10)==sizeof(U32));

union R10G10B1A2
{
	//TODO: pack into mask; x is not guaranteed to be LSB.
	struct {
		unsigned x : 10;
		unsigned y : 10;
		unsigned z : 10;
		unsigned w : 2;
	};
	U32	v;
};

typedef Tuple2< I32 >	Int2;
typedef Tuple2< U32 >	UInt2;

typedef CTuple3< I32 >	Int3;
typedef CTuple3< U32 >	UInt3;

typedef Tuple4< I32 >	Int4;
typedef Tuple4< U32 >	UInt4;

typedef CTuple3< bool >	Bool3;

/// 2D Vector; 16 bit floating point components
union Half2
{
	struct {
		Half	x;
		Half	y;
	};
	U32		v;
};

/// 2D Vector; 16 bit floating point components
union Half4
{
	struct {
		Half	x;
		Half	y;
		Half	z;
		Half	w;
	};
	U32		v;
};


struct V3F2
{
	Half	x, y, z;
};
struct V4F2
{
	Half	x, y, z, w;
};

/// 2D Vector; 16 bit signed integer components
struct V2I2 {
	I16	x;
	I16	y;
};
/// 2D Vector; 16 bit unsigned integer components
struct V2U2 {
	U16	x;
	U16	y;
};
/// 2D Vector; 16 bit signed normalized integer components
struct V2I2N {
	I16	x;
	I16	y;
};
/// 2D Vector; 16 bit unsigned normalized integer components
struct V2U2N {
	U16	x;
	U16	y;
};

/// 4D Vector of 8-bit unsigned integer components.
/// This is often used for storing RGBA colors or compressed normals.
struct UByte4 {
	union {
		struct {
			U8 x, y, z, w;
		};
		struct {
			U8 r, g, b, a;
		};
		struct {
			U8 c[4];
		};
		U32	v;
	};
};

/*
=======================================================================
	HIGH-LEVEL TYPES
=======================================================================
*/

//=====================================================================
//	DATA CONVERSION OPERATIONS
//=====================================================================

const V2f& V3_As_V2( const V3f& v );

const V4f& Vector4_As_V4( const Vector4& v );
const V3f& Vector4_As_V3( const Vector4& v );
const V3f& V4_As_V3( const V4f& v );

//=====================================================================
//	LOAD OPERATIONS
//=====================================================================

//=====================================================================
//	STORE OPERATIONS
//=====================================================================

//=====================================================================
//	GENERAL VECTOR OPERATIONS
//=====================================================================

//=====================================================================
//	2D VECTOR OPERATIONS
//=====================================================================

const V2f	V2_Zero();
const V2f 	V2_Set( float x, float y );
const V2f 	V2_Replicate( float value );
const V2f 	V2_Scale( const V2f& v, float s );
const float V2_Length( const V2f& v );
const float	V2_Dot( const V2f& a, const V2f& b );
const V2f	V2_Lerp( const V2f& from, const V2f& to, float amount );
const V2f	V2_Lerp01( const V2f& from, const V2f& to, float amount );

const Half2	V2_To_Half2( const V2f& xy );

const V2f	Half2_To_V2F( const Half2& xy );

//=====================================================================
//	3D VECTOR OPERATIONS
//=====================================================================

const V3f	V3_Zero();
const V3f 	V3_SetAll( float value );
const V3f 	V3_Set( const float* xyz );
const V3f 	V3_Set( float x, float y, float z );
const V3f	V3_Scale( const V3f& v, float s );
const float V3_LengthSquared( const V3f& v );
const float V3_Length( const V3f& v );
const V3f 	V3_Normalized( const V3f& v );
const V3f 	V3_Normalized( const V3f& v, float &_length );
const float	V3_Normalize( V3f * v );
const V3f 	V3_Abs( const V3f& v );
const V3f 	V3_Negate( const V3f& v );
const V3f 	V3_Reciprocal( const V3f& v );
const bool	V3_IsInfinite( const V3f& v );
const bool	V3_Equal( const V3f& a, const V3f& b );
const bool	V3_Equal( const V3f& a, const V3f& b, const float epsilon );
const float	V3_Mins( const V3f& xyz );
const float	V3_Maxs( const V3f& xyz );
const V3f 	V3_Mins( const V3f& a, const V3f& b );
const V3f 	V3_Maxs( const V3f& a, const V3f& b );
const V3f 	V3_Add( const V3f& a, const V3f& b );
const V3f 	V3_Subtract( const V3f& a, const V3f& b );
const V3f 	V3_Multiply( const V3f& a, const V3f& b );
const V3f 	V3_Divide( const V3f& a, const V3f& b );
const float	V3_Dot( const V3f& a, const V3f& b );
const V3f	V3_Cross( const V3f& a, const V3f& b );
const V3f	V3_FindOrthogonalTo( const V3f& v );
const V3f	V3_Blend( const V3f& x, const V3f& y, float a );
const V3f	V3_Lerp( const V3f& from, const V3f& to, float amount );
const V3f	V3_Lerp01( const V3f& from, const V3f& to, float amount );
const bool 	V3_IsNormalized( const V3f& v, float epsilon = 1e-4f );
const bool 	V3_AllGreaterThan(  const V3f& xyz, float value );
const bool 	V3_AllGreaterOrEqual(  const V3f& xyz, float value );
const bool 	V3_AllInRangeInclusive(  const V3f& xyz, float _min, float _max );
const V3f 	V3f_Floor( const V3f& xyz );
const V3f 	V3f_Ceiling( const V3f& xyz );
const Int3 	V3_Truncate( const V3f& xyz );
const V3f	V3_Clamped( const V3f& _xyz, const V3f& mins, const V3f& maxs );

const Int3 	Int3_Set( I32 x, I32 y, I32 z );
const V3f 	Int3_To_V3F( const Int3& xyz );

const UShort3	UShort3_Set( U16 x, U16 y, U16 z );

const U11U11U10	U11U11U10_Set( unsigned x, unsigned y, unsigned z );

//=====================================================================
//	4D VECTOR OPERATIONS
//=====================================================================

const bool	V4_Equal( const V4f& a, const V4f& b );
const bool	V4_Equal( const V4f& a, const V4f& b, const float epsilon );

const Vector4	Vector4_Zero();
const Vector4	Vector4_Replicate( float value );
const Vector4	Vector4_Set( const V3f& xyz, float w );
const Vector4	Vector4_Set( const V4f& _xyzw );
const Vector4	Vector4_Set( float x, float y, float z, float w );
const Vector4	Vector4_Load( const float* _xyzw );
const Vector4	Vector4_LoadFloat3_Unaligned( const float* _xyz );
const float		Vector4_Get_X( Vec4Arg0 v );
const float		Vector4_Get_Y( Vec4Arg0 v );
const float 	Vector4_Get_Z( Vec4Arg0 v );
const float 	Vector4_Get_W( Vec4Arg0 v );
const Vector4 	Vector4_SplatX( Vec4Arg0 v );
const Vector4 	Vector4_SplatY( Vec4Arg0 v );
const Vector4 	Vector4_SplatZ( Vec4Arg0 v );
const Vector4 	Vector4_SplatW( Vec4Arg0 v );
const bool 		Vector4_IsNaN( Vec4Arg0 v );
const bool 		Vector4_IsInfinite( Vec4Arg0 v );
const bool 		Vector4_Equal( Vec4Arg0 a, Vec4Arg0 b );
const bool 		Vector4_NotEqual( Vec4Arg0 a, Vec4Arg0 b );
const Vector4	Vector4_LengthSquared( Vec4Arg0 v );
//const Vector4	Vector4_LengthV( Vec4Arg0 v );
//const Vector4	Vector4_SqrtV( Vec4Arg0 v );
//const Vector4	Vector4_ReciprocalSqrtV( Vec4Arg0 v );
//const Vector4 	Vector4_ReciprocalLengthEstV( Vec4Arg0 v );
//const Vector4 	Vector4_ReciprocalLengthV( Vec4Arg0 v );
const Vector4 	Vector4_Normalized( Vec4Arg0 _v );
const Vector4	Vector4_Negate( Vec4Arg0 v );
const Vector4	Vector4_Scale( Vec4Arg0 _v, float scale );
const Vector4 	Vector4_Add( Vec4Arg0 _A, Vec4Arg0 _B );
const Vector4 	Vector4_Subtract( Vec4Arg0 a, Vec4Arg0 b );
const Vector4 	Vector4_Multiply( Vec4Arg0 a, Vec4Arg0 b );
const Vector4 	Vector4_MultiplyAdd( Vec4Arg0 a, Vec4Arg0 b, Vec4Arg0 c );
const Vector4	Vector4_Cross3( Vec4Arg0 a, Vec4Arg0 b );
const Vector4	Vector4_Dot( Vec4Arg0 A, Vec4Arg0 B );
const Vector4	Vector4_Dot3( Vec4Arg0 A, Vec4Arg0 B );
const Vector4	Vector4_Min( Vec4Arg0 A, Vec4Arg0 B );
const Vector4	Vector4_Max( Vec4Arg0 A, Vec4Arg0 B );
void			Vector4_SinCos( Vec4Arg0 v, Vector4 *s, Vector4 *c );


#if !MM_NO_INTRINSICS

	#define V4_REPLICATE( X )	(_mm_set_ps1( (X) ))

	#define V4_ADD( A, B )		(_mm_add_ps( (A), (B) ))
	#define V4_SUB( A, B )		(_mm_sub_ps( (A), (B) ))
	#define V4_MUL( A, B )		(_mm_mul_ps( (A), (B) ))
	#define V4_DIV( A, B )		(_mm_div_ps( (A), (B) ))

	/// Fused Multiply-add (FMA) : A * B + C
	#define V4_MAD( A, B, C )	(_mm_add_ps( _mm_mul_ps((A),(B)), (C) ))

	#define V4_AND( A, B )		(_mm_and_ps( (A), (B) ))

	#define V4_CMPLT( A, B )	(_mm_cmplt_ps( (A), (B) ))
	#define V4_CMPGT( A, B )	(_mm_cmpgt_ps( (A), (B) ))

	#define V4_CMPLE( A, B )	(_mm_cmple_ps( (A), (B) ))
	#define V4_CMPGE( A, B )	(_mm_cmpge_ps( (A), (B) ))

	/* Move ByteMask To Int: returns mask formed from most sig bits	of each vec of a */
	#define V4_MOVEMASK( X )	(_mm_movemask_ps( (X) ))

	#define V4_MIN( A, B )	(_mm_min_ps( (A), (B) ))
	#define V4_MAX( A, B )	(_mm_max_ps( (A), (B) ))

	/// Negate a floating point vector, y = -x 
	//#define V4_NEGATE( X )	(_mm_xor_ps( (X), _mm_set1_ps(-0.f) ))
	#define V4_NEGATE( X )	(_mm_xor_ps( (X), g_MM_SignMask.m128 ))

	#define V4_ABS(x)	(_mm_andnot_ps((x), g_MM_SignMask))


#if defined(__SSE4_1__)
	// Returns true if all the bits in the given float vector are zero, when interpreted as an integer.
	// Warning: this SSE1 version is more like "all finite without nans" instead of "allzero", because
	// it does not detect finite non-zero floats. Call only for inputs that are either all 0xFFFFFFFF or 0.
	#define V4_ALLZERO_PS(x)	_mm_testz_si128(_mm_castps_si128((x)), _mm_castps_si128((x)))


	// Returns true if all the bits in (a&b) are zero.
	#define V4_A_AND_B_ARE_ZEROS(a, b)	_mm_testz_si128(_mm_castps_si128(a), _mm_castps_si128(b))
#else
	#error Unimpl!
#endif

	#define V4_LOAD_PS_HEX(w, z, y, x)	_mm_castsi128_ps(_mm_set_epi32(w, z, y, x))

#endif	// #ifndef MM_NO_INTRINSICS




#define Vector4_Length3Squared(v)	(Vector4_Dot3( v, v ))



/*
=======================================================================
	TEMPLATED MATRIX TYPES
=======================================================================
*/

#define MATRIX_EPSILON	1e-6
#define MATRIX_INVERSE_EPSILON		1e-14




//=====================================================================
//	COLOR OPERATIONS
//=====================================================================

//=====================================================================
//	MISCELLANEOUS OPERATIONS
//=====================================================================

float Get_Field_of_View_X_Degrees( float FoV_Y_degrees, float width, float height );
float Get_Field_of_View_Y_Degrees( float FoV_X_degrees, float width, float height );

void buildOrthonormalBasis( const V3f& look, V3f &right_, V3f &up_ );

/*
-------------------------------------------------------------------
	These functions can be used for compressing floating-point normal vectors.

	Quote from 'Rubicon':
	http://www.gamedev.net/topic/564892-rebuilding-normal-from-2-components-with-cross-product/
	Trust me, 8 bits per component is plenty,
	especially if it gets your whole vertex to 32 bytes or lower.
	I've used it for years and nobody's ever even noticed
	- you get a perfect spread of lighting across a ball at any magnification you like
	- don't forget its interpolated per pixel
	- all this decompression happens at the VS level, not the PS.

	Excerpt from ShaderX:
	Normalized normals or tangents are usually stored as three floats (D3DVSDT_FLOAT3), but
	this precision is not needed. Each component has a range of –1.0 to 1.0 (due to normalization),
	ideal for quantization. We don’t need any integer components, so we can devote all bits to the
	fractional scale and a sign bit. Biasing the floats makes them fit nicely into an unsigned byte.
	For most normals, 8 bits will be enough (that’s 16.7 million values over the surface of a
	unit sphere). Using D3DVSDT_UBYTE4, we have to change the vertex shader to multiply by
	1.0/127.5 and subtract 1.0 to return to signed floats. I’ll come back to the optimizations that
	you may have noticed, but for now, I’ll accept the one cycle cost for a reduction in memory
	and bandwidth saving of one-fourth for normals and tangents.
	Quantization vertex shader example:
	; v1 = normal in range 0 to 255 (integer only)
	; c0 = <1.0/127.5, -1.0, ???? , ????>
	mad r0, v1, c0.xxxx, c0.yyyy ; multiply compressed normal by 1/127.5, subtract 1

	Also, see:
	http://flohofwoe.blogspot.com/2008/03/vertex-component-packing.html
-------------------------------------------------------------------
*/

// [0..1]f => [0..255]i
static inline U8 _NormalizedFloatToUInt8( float f ) {
	return f * 255.0f;
}
// [0..255]i => [0..1]f
static inline float _UInt8ToNormalizedFloat( U8 c ) {
	return c * 1.0f/255.0f;
}

// [-1..+1] float => [0..255] int
static inline U8 _NormalToUInt8( float f ) {
	// scale and bias: ((f + 1.0f) * 0.5f) * 255.0f
	return mmFloorToInt( f * 127.5f + 127.5f );
}
// [0..255] int -> [-1..+1] float
static inline float _UInt8ToNormal( U8 c ) {
	return c * (1.0f / 127.5f) - 1.0f;
}

inline UByte4 PackNormal( float x, float y, float z, float w = 1.0f ) {
	UByte4 packed = { _NormalToUInt8(x), _NormalToUInt8(y), _NormalToUInt8(z), _NormalToUInt8(w) };
	return packed;
}
inline V3f UnpackNormal( const UByte4& v ) {
	return CV3f( _UInt8ToNormal(v.x), _UInt8ToNormal(v.y), _UInt8ToNormal(v.z) );
}

inline UByte4 PackNormal( const V3f& n ) { return PackNormal(n.x, n.y, n.z); }

// [-32768..32767] => [-1..+1]
inline float Short_To_Normal( I16 x )
{
	return (x == -32768) ? -1.f : ((float)x * (1.0f/32767.0f));
}

//NOTE: Don't forget to AND with MaxInteger mask when decoding.
template< UINT NUM_BITS >
struct TQuantize
{
	static const int MaxInteger = (1u << NUM_BITS) - 1;

	// [0..1] float => [0..max] int
	mxFORCEINLINE static UINT EncodeUNorm( float f )
	{
		mxASSERT(f >= 0.0f && f <= 1.000001f);
		return f * MaxInteger;
	}

	// [0..1] float => [0..max] int
	mxFORCEINLINE static UINT EncodeUNorm_ClampTo01( float f )
	{
		return clampf( f, 0.0f, 1.0f ) * MaxInteger;
	}

	// [0..max] int => [0..1] float
	mxFORCEINLINE static float DecodeUNorm( UINT i )
	{
//		i &= MaxInteger;
		return i * (1.0f / MaxInteger);
	}

	// [-1..+1] float => [0..max] int
	mxFORCEINLINE static UINT EncodeSNorm( float f )
	{
		mxASSERT(f >= -1.0f && f <= +1.0f);
		// scale and bias: ((f + 1.0f) * 0.5f) * MaxInteger
		float halfRange = MaxInteger * 0.5f;
		return mmFloorToInt( f * halfRange + halfRange );
	}
	// [0..max] int -> [-1..+1] float
	mxFORCEINLINE static float DecodeSNorm( UINT i )
	{
//		i &= MaxInteger;
		float invHalfRange = 2.0f / MaxInteger;
		return i * invHalfRange - 1.0f;
	}
};

//11,11,10
mxFORCEINLINE
U32 Encode_XYZ01_to_R11G11B10( float x, float y, float z ) {
	return
		(TQuantize<11>::EncodeUNorm( x )) |
		(TQuantize<11>::EncodeUNorm( y ) << 11u) |
		(TQuantize<10>::EncodeUNorm( z ) << 22u);
}

/// Quantizes normalized coords into 32-bit integer (11,11,10).
mxFORCEINLINE
U32 Encode_XYZ01_to_R11G11B10( const V3f& p ) {
	return Encode_XYZ01_to_R11G11B10( p.x, p.y, p.z );
}

/// De-quantizes normalized coords from 32-bit integer.
mxFORCEINLINE
V3f Decode_XYZ01_from_R11G11B10( U32 u ) {
	return CV3f(
		TQuantize<11>::DecodeUNorm( u & 2047 ),
		TQuantize<11>::DecodeUNorm( (u >> 11u) & 2047 ),
		TQuantize<10>::DecodeUNorm( (u >> 22u) & 1023 )
	);
}


//10,10,10
mxFORCEINLINE
U32 Encode_XYZ01_to_RGB10( float x, float y, float z ) {
	return
		(TQuantize<10>::EncodeUNorm( x )) |
		(TQuantize<10>::EncodeUNorm( y ) << 10u) |
		(TQuantize<10>::EncodeUNorm( z ) << 20u);
}

/// Quantizes normalized coords into 32-bit integer (10,10,10).
mxFORCEINLINE
U32 Encode_XYZ01_to_RGB10( const V3f& p ) {
	return Encode_XYZ01_to_RGB10( p.x, p.y, p.z );
}

/// De-quantizes normalized coords from 32-bit integer.
mxFORCEINLINE
V3f Decode_XYZ01_from_RGB10( U32 u ) {
	return CV3f(
		TQuantize<10>::DecodeUNorm( (u       ) & 1023 ),
		TQuantize<10>::DecodeUNorm( (u >> 10u) & 1023 ),
		TQuantize<10>::DecodeUNorm( (u >> 20u) & 1023 )
	);
}



mxFORCEINLINE
U11U11U10 U11U11U10_from_Normal( const V3f& normal )
{
	const V3f normal01 = V3f::scale( normal, 0.5f ) + CV3f(0.5f);
	//
	U11U11U10	result;
	result.v = Encode_XYZ01_to_R11G11B10( normal01 );
	return result;
}




//
mxFORCEINLINE
U64 Encode_XYZ01_to_R21G21B22_64( float x, float y, float z ) {
	return (U64(TQuantize<21>::EncodeUNorm( x )))
		|  (U64(TQuantize<21>::EncodeUNorm( y )) << 21ull)
		|  (U64(TQuantize<22>::EncodeUNorm( z )) << 42ull)
		;
}
mxFORCEINLINE
U64 Encode_XYZ01_to_R21G21B22_64( const V3f& p ) {
	return Encode_XYZ01_to_R21G21B22_64( p.x, p.y, p.z );
}

/// De-quantizes normalized coords from 64-bit integer.
mxFORCEINLINE
const V3f Decode_XYZ01_from_R21G21B22_64( U64 u ) {
	return CV3f(
		TQuantize<21>::DecodeUNorm( u         & ((1<<21)-1) ),
		TQuantize<21>::DecodeUNorm( (u >> 21) & ((1<<21)-1) ),
		TQuantize<22>::DecodeUNorm( (u >> 42) & ((1<<22)-1) )
	);
}


mxSTOLEN("Doom3 BFG edition");

// GPU half-float bit patterns
#define HF_MANTISSA(x)	(x&1023)
#define HF_EXP(x)		((x&32767)>>10)
#define HF_SIGN(x)		((x&32768)?-1:1)

inline float Half_To_Float( Half x ) {
	int e = HF_EXP( x );
	int m = HF_MANTISSA( x );
	int s = HF_SIGN( x );

	if ( 0 < e && e < 31 ) {
		return s * powf( 2.0f, ( e - 15.0f ) ) * ( 1 + m / 1024.0f );
	} else if ( m == 0 ) {
        return s * 0.0f;
	}
    return s * powf( 2.0f, -14.0f ) * ( m / 1024.0f );
}

inline Half Float_To_Half( float a ) {
	unsigned int f = *(unsigned *)( &a );
	unsigned int signbit  = ( f & 0x80000000 ) >> 16;
	int exponent = ( ( f & 0x7F800000 ) >> 23 ) - 112;
	unsigned int mantissa = ( f & 0x007FFFFF );

	if ( exponent <= 0 ) {
		return 0;
	}
	if ( exponent > 30 ) {
		return (Half)( signbit | 0x7BFF );
	}

	return (Half)( signbit | ( exponent << 10 ) | ( mantissa >> 13 ) );
}



inline float DistanceBetween( const V3f& a, const V3f& b )
{
	return V3_Length( V3_Subtract( b, a ) );
}

/// Returns the closest point on the line to the given point.
inline V3f Ray_ClosestPoint(
							const V3f& origin,
							const V3f& direction,	// must be normalized
							const V3f& point
							)
{
	mxASSERT(V3_IsNormalized(direction));
	// find the coefficient t with the property that (point - closest_point) vector is orthogonal to the line
	const float t = V3_Dot( V3_Subtract( point, origin ), direction );	// dot(direction,direction) is 1, so no need to perform division by it
	// project vector (P-origin) onto the line then add the resulting vector to the origin.
	return V3_Add( origin, V3_Scale( direction, t ) );
}

/// Returns the squared distance between the given point and the line.
inline float Ray_PointDistanceSqr(
								  const V3f& origin,
								  const V3f& direction,	// must be normalized
								  const V3f& point
								  )
{
	return V3_LengthSquared( V3_Subtract( Ray_ClosestPoint( origin, direction, point ), point ) );
}

/// Returns the distance between the given point and the line.
inline float Ray_PointDistance(
							   const V3f& origin,
							   const V3f& direction,	// must be normalized
							   const V3f& point
							   )
{
	return DistanceBetween( Ray_ClosestPoint( origin, direction, point ), point );
}

/*
// returns:
//	OutP1 - closest point on the first ray to the second ray
//	OutP2 - closest point on the second ray to the first ray
//
inline ELineStatus IntersectLines( const Ray3D& a, const Ray3D& b, V3f &OutP1, V3f &OutP2 )
{
	const V3f	p1 = a.origin;
	const V3f	p2 = b.origin;
	const V3f	d1 = a.direction;
	const V3f	d2 = b.direction;

	const FLOAT denominator = cross(d1,d2).LengthSqr();

	// If the lines are parallel (or coincident), then the cross product of d1 and d2 is the zero vector.

	if( denominator < VECTOR_EPSILON )
	{
		return Lines_Parallel;
	}

	const FLOAT t1 = cross( (p2-p1), d2 ) * cross(d1,d2) / denominator;
	const FLOAT t2 = cross( (p2-p1), d1 ) * cross(d1,d2) / denominator;

	// If the lines are skew, then I1 and I2 are the points of closest approach.

	const V3f I1 = p1 + d1 * t1;
	const V3f I2 = p2 + d2 * t2;

	OutP1 = I1;
	OutP2 = I2;

	// To distinguish between skew and intersecting lines,
	// we examine the distance between I1 and I2.

	const FLOAT lsqr = (I1 - I2).LengthSqr();
	return (lsqr > VECTOR_EPSILON) ? Lines_Skew : Lines_Intersect;
}
*/



mxSTOLEN("PhysX SDK");
// Collide ray defined by ray origin (rayOrigin) and ray direction (rayDirection)
// with the bounding box. Returns -1 on no collision and the face index
// for first intersection if a collision is found together with
// the distance to the collision points (tnear and tfar)
//
// ptchernev:
// Even though the above is the original comment by Pierre I am quite confident 
// that the tnear and tfar parameters are parameters along rayDirection of the
// intersection points:
//
// ip0 = rayOrigin + (rayDirection * tnear)
// ip1 = rayOrigin + (rayDirection * tfar)
//
// The return code is:
// -1 no intersection
//  0 the ray first hits the plane at aabbMin.x
//  1 the ray first hits the plane at aabbMin.y
//  2 the ray first hits the plane at aabbMin.z
//  3 the ray first hits the plane at aabbMax.x
//  4 the ray first hits the plane at aabbMax.y
//  5 the ray first hits the plane at aabbMax.z
//
// The return code will be -1 if the RAY does not intersect the AABB.
// The tnear and tfar values will give the parameters of the intersection 
// points between the INFINITE LINE and the AABB.
// 
int intersectRayAABB(
					 const V3f& _minimum, const V3f& _maximum,
					 const V3f& _ro, const V3f& _rd, float& _tnear, float& _tfar);

struct Rayf
{
	V3f	origin;
	V3f	direction;

	// precomputed
	V3f inv_dir;

public:
	void RecalcDerivedValues();
};

int IntersectRayAABB(
					 const Rayf& ray,
					 const V3f& aabb_min, const V3f& aabb_max,
					 float &tnear_, float &tfar_
					 );



bool AABBf_Intersect_Ray_SSE2(
							 const V3f& _minimum, const V3f& _maximum,
							 const V3f& _ro, const V3f& _rd,
							 float& _tnear, float& _tfar
							 );


bool Sphere_IntersectsRay(
						  const V3f& center,
						  const F32 radius,
						  const V3f& origin,
						  const V3f& dir,
						  const F32 length,
						  F32 &_hit_time,
						  V3f &_hit_pos
						  );

/// the algorithm proposed by Moeller and Trumbore (1997)
/// returns 0 if no intersection
int RayTriangleIntersection(
						  const V3f& V1,  // Triangle vertices
						  const V3f& V2,
						  const V3f& V3,
						  const V3f& O,  //Ray origin
						  const V3f& D,  //Ray direction
						  float* out,
						  float epsilon = 1e-6f
						  );

mxSTOLEN("mitsuba-renderer/src/libcore/util.cpp");
//see: Numerically Stable Method for Solving Quadratic Equations
//and: https://en.wikipedia.org/wiki/Loss_of_significance#Instability_of_the_quadratic_equation
/**
 *	Computes real quadratic roots for equ. ax^2 + bx + c = 0
 *
 *	\param		a	[in] a coeff
 *	\param		b	[in] b coeff
 *	\param		c	[in] c coeff
 *	\param		x0	[out] smallest real root
 *	\param		x1	[out] largest real root
 *	\return		number of real roots
 */
template< typename REAL >
// NOTE: relies on <algorithm>
bool SolveQuadratic( REAL a, REAL b, REAL c, REAL &x0, REAL &x1 )
{
	///* Linear case */
	//if (a == 0) {
	//	if (b != 0) {
	//		x0 = x1 = -c / b;
	//		return true;
	//	}
	//	return false;
	//}

	const REAL discrim = b*b - 4.0f*a*c;

	/* Leave if there is no solution */
	if (discrim < 0) {
		return 0;	// Complex roots
	}

	const REAL sqrtDiscrim = std::sqrt(discrim);

	REAL temp;

	/* Numerically stable version of (-b (+/-) sqrtDiscrim) / (2 * a)
	*
	* Based on the observation that one solution is always
	* accurate while the other is not. Finds the solution of
	* greater magnitude which does not suffer from loss of
	* precision and then uses the identity x1 * x2 = c / a
	*/
	if (b < 0) {
		temp = (b - sqrtDiscrim) * REAL(-0.5);
	} else {
		temp = (b + sqrtDiscrim) * REAL(-0.5);
	}

	x0 = temp / a;
	x1 = c / temp;

	/* Return the results so that x0 < x1 */
	if (x0 > x1) {
		std::swap(x0, x1);
	}
	return (discrim > 0) : 2 : 1;
}

namespace RootFinding
{
	namespace detail {
		/** Helper function to make a same sign as b. */
		template< typename Scalar >
		mmINLINE Scalar makeSameSign(Scalar a, Scalar b) {
			return b >= 0 ? std::abs(a) : -std::abs(a);
		}
	}
	/** 
		find root of function in interval.
	    
		Uses Brent's method in the given interval. Implementation taken from
		Numerical Recipes in C.
	*/
	template< class F, typename Scalar >
	bool Brent( F &f, Scalar x1, Scalar x2, Scalar tol, int maxIter, Scalar &root )
	{
		const Scalar eps = std::numeric_limits<Scalar>::epsilon();

		int iter;
		Scalar a=x1,b=x2,c=x2,d,e,min1,min2;
		Scalar fa=f(a),fb=f(b),fc,p,q,r,s,tol1,xm;

		if ((fa > 0 && fb > 0) || (fa < 0 && fb < 0))
			return false; // Root must be bracketed

		fc=fb;
		for (iter=1;iter<=maxIter;iter++) {
			if ((fb > 0 && fc > 0) || (fb < 0 && fc < 0)) {
				c=a;
				fc=fa;
				e=d=b-a;
			}
			if (std::abs(fc) < std::abs(fb)) {
				a=b;
				b=c;
				c=a;
				fa=fb;
				fb=fc;
				fc=fa;
			}
			tol1=Scalar(2)*eps*std::abs(b)+Scalar(0.5)*tol;
			xm=Scalar(0.5)*(c-b);
			if (std::abs(xm) <= tol1 || fb == 0.0) {
				root = b;
				return true;
			}
	        
			if (std::abs(e) >= tol1 && std::abs(fa) > std::abs(fb)) {
				// Attempt inverse quadratic interpolation.
				s=fb/fa;
				if (a == c) {
					p=Scalar(2.0)*xm*s;
					q=Scalar(1.0)-s;
				} else {
					q=fa/fc;
					r=fb/fc;
					p=s*(Scalar(2.0)*xm*q*(q-r)-(b-a)*(r-Scalar(1.0)));
					q=(q-Scalar(1.0))*(r-Scalar(1.0))*(s-Scalar(1.0));
				}
				if (p > 0.0)
					q = -q; // Check whether in bounds.
	            
				p=fabs(p);
				min1=Scalar(3.0)*xm*q-std::abs(tol1*q);
				min2=std::abs(e*q);
	            
				if (Scalar(2.0)*p < (min1 < min2 ? min1 : min2)) {
					e=d; // Accept interpolation.
					d=p/q;
				} else {
					d=xm; // Interpolation failed, use bisection.
					e=d;
				}
			} else {
				// Bounds decreasing too slowly, use bisection.
				d=xm;
				e=d;
			}
			a=b; // Move last best guess to a.
			fa=fb;
			if (std::abs(d) > tol1) // Evaluate new trial root.
				b += d;
			else
				b += detail::makeSameSign(tol1,xm);
			fb = f(b);
		}
		root = b;
		return false; // max number of iterations exceeded.
	}
}//namespace RootFinding

/// returns the number of roots (0 == no intersections)
/// t1 and t2 - (NOT normalized) distances along the ray, t1 <= t2
int RaySphereIntersection(
						   const V3f& ray_origin, const V3f& ray_direction,
						   const V3f& sphere_center, const float sphere_radius,
						   float *t1_, float *t2_
						   );


static inline
bool SphereContainsPoint(
						 const V3f& _sphereCenter, const float _sphereRadius,
						 const V3f& _point
						 )
{
	return DistanceBetween( _sphereCenter, _point ) <= _sphereRadius;
}

// NOTE: expects plane equations for the sides of the frustum,
// with the positive half-space outside the frustum
//
template< int NUM_PLANES >
inline int Frustum_IntersectSphere( const V4f (&_planes)[NUM_PLANES], const V3f& center, float radius )
{
	// assume the sphere is inside by default
	int	mask = 1;
	for( int iPlane = 0; (iPlane < mxCOUNT_OF(_planes)) && mask; iPlane++ )
	{
		// Test whether the sphere is on the negative half space of each frustum plane.
		// If it is on the positive half space of one plane we can reject it.
		mask &= (Plane_PointDistance( _planes[ iPlane ], center ) < radius);
	}
    return mask;
}

// viewport coordinates => normalized device coordinates (NDC)
template< typename POINT_SIZE >
static inline void ViewportToNDC(
	float invScreenWidth, float invScreenHeight,
	POINT_SIZE screenX, POINT_SIZE screenY,
	float &ndcX, float &ndcY)
{
	ndcX = (screenX * invScreenWidth) * 2.0f - 1.0f;	// [0..ScreenWidth] => [-1..+1]
	ndcY = (screenY * invScreenHeight) * 2.0f - 1.0f;	// [0..ScreenHeight] => [+1..-1]
	ndcY *= -1.0f;	// (flip Y axis)
}

/// Cohen–Sutherland clipping algorithm
enum EClipCodes
{
	CLIP_NEG_X = 0,
	CLIP_POS_X = 1,
	CLIP_NEG_Y = 2,
	CLIP_POS_Y = 3,
	CLIP_NEG_Z = 4,
	CLIP_POS_Z = 5,
};

///// projection matrix transforms vertices from world space to normalized clip space
///// where only points that fulfill the following are inside the view frustum:
///// -|w| <= x <= |w|
///// -|w| <= y <= |w|
///// -|w| <= z <= |w|
//// tests each axis against W and returns the corresponding clip mask (with bits from EClipCodes)
template< class VECTOR >
static inline
UINT ClipMask( VECTOR & v )
{
#if 0
	UINT cmask = 0;
	if( v.p.w - v.p.x < 0.0f ) cmask |= BIT( CLIP_POS_X );
	if( v.p.x + v.p.w < 0.0f ) cmask |= BIT( CLIP_NEG_X );
	if( v.p.w - v.p.y < 0.0f ) cmask |= BIT( CLIP_POS_Y );
	if( v.p.y + v.p.w < 0.0f ) cmask |= BIT( CLIP_NEG_Y );
	if( v.p.w - v.p.z < 0.0f ) cmask |= BIT( CLIP_POS_Z );
	if( v.p.z + v.p.w < 0.0f ) cmask |= BIT( CLIP_NEG_Z );
	return cmask;
#else
	return
		(( v.x < -v.w ) << CLIP_NEG_X)|
		(( v.x >  v.w ) << CLIP_POS_X)|
		(( v.y < -v.w ) << CLIP_NEG_Y)|
		(( v.y >  v.w ) << CLIP_POS_Y)|
		(( v.z < -v.w ) << CLIP_NEG_Z)|
		(( v.z >  v.z ) << CLIP_POS_Z);
#endif
}

///	For referring to the components of a vector by name rather than by an integer.
enum EVectorComponent
{
	Vector_X = 0,
	Vector_Y = 1,
	Vector_Z = 2,
	Vector_W = 3,
};

//
//	EViewAxes
//
enum EViewAxes
{
	VA_LookAt	= Vector_Y,	// Look direction vector ('forward').
	VA_Right	= Vector_X,
	VA_Up		= Vector_Z,	// 'upward'	
};

/*
#define	DEFAULT_LOOK_VECTOR		CV3f( 0.0f, 0.0f, 1.0f )
#define	WORLD_RIGHT_VECTOR		CV3f( 1.0f, 0.0f, 0.0f )
#define	WORLD_UP_VECTOR			CV3f( 0.0f, 1.0f, 0.0f )

#define	UNTRANSFORMED_NORMAL	DEFAULT_LOOK_VECTOR
*/

/// Round the floating-point coordinates to integer-valued grid coordinates.
/// @param _position world-space position
/// @param _cellSize world-space cell size
inline const Int3 RoundToGrid( const V3f& _position, float _gridSize )
{
	return Int3(
		mmRoundToInt( _position.x / _gridSize ),
		mmRoundToInt( _position.y / _gridSize ),
		mmRoundToInt( _position.z / _gridSize )
	);
}

// NOTE: the template parameter SIZE_Z is used only for checking.
template< U32 SIZE_X, U32 SIZE_Y, U32 SIZE_Z >
static inline U32 GetFlattenedIndex( U32 iX, U32 iY, U32 iZ )
{
	mxASSERT(iX >= 0 && iX < SIZE_X);
	mxASSERT(iY >= 0 && iY < SIZE_Y);
	mxASSERT(iZ >= 0 && iZ < SIZE_Z);
	return (SIZE_X*SIZE_Y)*iZ + (SIZE_X*iY) + iX;
	//return ( iZ * SIZE_Y + iY ) * SIZE_X + iX;
}

//NOTE: watch out for overflow! make sure that X,Y and Z are inside the range!
template< U32 SIZE_X, U32 SIZE_Y >
static inline U32 GetFlattenedIndex_NoCheck( U32 iX, U32 iY, U32 iZ )
{
	return (SIZE_X*SIZE_Y)*iZ + (SIZE_X*iY) + iX;
	//return ( iZ * SIZE_Y + iY ) * SIZE_X + iX;
}

/// Converts a flat index into a tuple of coordinates.
template< U32 SIZE_X, U32 SIZE_Y, U32 SIZE_Z >
inline void UnravelFlattenedIndex(
						 const U32 _flatIndex,
						 U32 &_iX, U32 &_iY, U32 &_iZ
						 )
{
	mxASSERT(_flatIndex >= 0 && _flatIndex < SIZE_X*SIZE_Y*SIZE_Z);
	U32 rem = _flatIndex;

	U32 iZ = _flatIndex / (SIZE_X*SIZE_Y);
	rem -= iZ*(SIZE_X*SIZE_Y);

	U32 iY = rem / SIZE_X;
	rem -= iY * SIZE_X;

	U32 iX = rem;

	_iZ = iZ;
	_iY = iY;
	_iX = iX;
}

/// Linearizes the given coordinates.
template< typename TYPE >
inline U32 GetFlattenedIndexV( const Tuple3< TYPE >& _xyz, const Tuple3< TYPE >& _size )
{
	mxASSERT(_xyz.x >= 0 && _xyz.x < _size.x);
	mxASSERT(_xyz.y >= 0 && _xyz.y < _size.y);
	mxASSERT(_xyz.z >= 0 && _xyz.z < _size.z);
	//return (_size.x * _size.y) * _xyz.z + (_size.x * _xyz.y) + _xyz.x;	// 3 muls and 3 adds
	return ( _size.y * _xyz.z + _xyz.y ) * _size.x + _xyz.x;	// 2 muls and 2 adds, but added pipeline dependency
}
template< typename TYPE >
inline U32 GetFlattenedIndexV_NoCheck( const Tuple3< TYPE >& _xyz, const Tuple3< TYPE >& _size )
{
	//return (_size.x * _size.y) * _xyz.z + (_size.x * _xyz.y) + _xyz.x;	// 3 muls and 3 adds
	return ( _size.y * _xyz.z + _xyz.y ) * _size.x + _xyz.x;	// 2 muls and 2 adds, but added pipeline dependency
}

template< typename TYPE >
inline Tuple3< TYPE > UnravelFlattenedIndexV( const U32 _index, const Tuple3< TYPE >& _size )
{
	mxASSERT(_index >= 0 && _index < (_size.x * _size.y * _size.z));
	U32 rem = _index;

	U32 iZ = _index / (_size.x * _size.y);
	rem -= iZ * (_size.x * _size.y);

	U32 iY = rem / _size.x;
	rem -= iY * _size.x;

	U32 iX = rem;

	return Tuple3< TYPE >( iX, iY, iZ );
}

/// Chebyshev distance (aka "[Tchebychev|Chessboard|Center] Distance" or "maximum [metric|norm]"):
/// the maximum of the absolute rank- and file-distance of both squares.
template< typename TYPE >	// inputs must be signed, output is always unsigned
static inline
TYPE Chebyshev_Distance( const Tuple3< TYPE >& a, const Tuple3< TYPE >& b )
{
	const Tuple3< TYPE > diff = a - b;
	return Max3( Abs( diff.x ), Abs( diff.y ), Abs( diff.z ) );
}

// Manhattan distance: d=abs(xd)+abs(yd) or 1-norm;
template< typename TYPE >
static inline
TYPE Manhattan_Distance( const Tuple3< TYPE >& a, const Tuple3< TYPE >& b )
{
	const Tuple3< TYPE > diff = a - b;
	return Abs( diff.x ) + Abs( diff.y ) + Abs( diff.z );
}


/*
=======================================================================
	GLOBALS
=======================================================================
*/

// The purpose of the following global constants is to prevent redundant 
// reloading of the constants when they are referenced by more than one
// separate inline math routine called within the same function.  Declaring
// a constant locally within a routine is sufficient to prevent redundant
// reloads of that constant when that single routine is called multiple
// times in a function, but if the constant is used (and declared) in a 
// separate math routine it would be reloaded.

mmGLOBALCONST Vector4 g_MM_Zero	= { 0.0f, 0.0f, 0.0f, 0.0f };
mmGLOBALCONST Vector4 g_MM_One	= { 1.0f, 1.0f, 1.0f, 1.0f };
mmGLOBALCONST Vector4 g_MM_255	= { 255.0f, 255.0f, 255.0f, 255.0f };

mmGLOBALCONST Vector4i g_MM_Select1000        = {mmSELECT_1, mmSELECT_0, mmSELECT_0, mmSELECT_0};
mmGLOBALCONST Vector4i g_MM_Select1100        = {mmSELECT_1, mmSELECT_1, mmSELECT_0, mmSELECT_0};
mmGLOBALCONST Vector4i g_MM_Select1110        = {mmSELECT_1, mmSELECT_1, mmSELECT_1, mmSELECT_0};

mmGLOBALCONST Vector4i g_MM_Mask3             = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000};	/// Keep X,Y,Z, set W to zero
mmGLOBALCONST Vector4i g_MM_AllMask           = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};	// g_XMNegOneMask
mmGLOBALCONST Vector4i g_MM_SignMask          = {0x80000000, 0x80000000, 0x80000000, 0x80000000};

mmGLOBALCONST Vector4i g_MM_Infinity          = {0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000};
mmGLOBALCONST Vector4i g_MM_QNaN              = {0x7FC00000, 0x7FC00000, 0x7FC00000, 0x7FC00000};

mmGLOBALCONST Vector4i g_MM_FltMin            = {0x00800000, 0x00800000, 0x00800000, 0x00800000};
mmGLOBALCONST Vector4i g_MM_FltMax            = {0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF};

// This value controls vector normalization, vector comparison routines, etc.
mmGLOBALCONST FLOAT VECTOR_EPSILON = 0.001f;







#include "MiniMath.inl"


/*
=======================================================================
	THESE FUNCTIONS RELY ON OVERLOADED VECTOR OPERATORS
=======================================================================
*/
inline const V3f Triangle_Normal( const V3f& a, const V3f& b, const V3f& c ) {
	return V3_Normalized( V3_Cross( b - a, c - a ) );
}
inline float Calculate_Triangle_Area( const V3f& a, const V3f& b, const V3f& c ) {
	return 0.5f * V3_Length( V3_Cross( b - a, c - a ) );
}
// Returns 1 for equilateral triangles and 0 for degenerated triangles (i.e. with collinear vertices)
// A. Gueziec, "Surface Simplification with Variable Tolerance", MRCAS'95, 1995, pp. 132-139.
inline float Calculate_Triangle_Compactness( const V3f& a, const V3f& b, const V3f& c ) {
	const float triangleArea = Calculate_Triangle_Area( a, b, c );
	// the (squared) lengths of the edges
	const float L1 = V3_LengthSquared( b - a );
	const float L2 = V3_LengthSquared( c - a );
	const float L3 = V3_LengthSquared( c - b );
	return (4.0f * mxSQRT_3 * triangleArea) / (L1 + L2 + L3);
}

// Given a cluster with area A and perimeter P,
// we define the irregularity I of the cluster
// as a ratio of its squared perimeter to its area:
// I = P^2 / (4 * PI * A)
// A Hierarchical Face Clustering on Polygonal Surfaces, 2001.



/*
=======================================================================
	THESE ARE USED FOR INDEXING MULTIRESOLUTION VOXEL/CELL GRIDS
=======================================================================
*/
static inline
Int3 ToFinerLoD( const Int3& coords_in_coarser_LoD ) {
	return coords_in_coarser_LoD * 2;
}

static inline
Int3 ToCoarserLoD( const Int3& coords_in_finer_LoD ) {
	return Int3( 
		floor_div( coords_in_finer_LoD.x, 2 ),
		floor_div( coords_in_finer_LoD.y, 2 ),
		floor_div( coords_in_finer_LoD.z, 2 )
	);
}

static inline
Int3 ToCoarserLoD_Ceil( const Int3& coords_in_finer_LoD ) {
	return Int3(
		ceil_div_posy( coords_in_finer_LoD.x, 2 ),
		ceil_div_posy( coords_in_finer_LoD.y, 2 ),
		ceil_div_posy( coords_in_finer_LoD.z, 2 )
	);
}

static inline
Int3 SnapToCoarserLoD( const Int3& coords_in_finer_LoD ) {
	return ToFinerLoD( ToCoarserLoD( coords_in_finer_LoD ) );	// Snap the coords to the grid of the next LoD.
}

/// Unsigned division by two with ceiling.
static inline
const UInt3 CeilDivBy2( const UInt3& _u ) {
	return UInt3(
		( _u.x / 2u ) + ( _u.x % 2u ),
		( _u.y / 2u ) + ( _u.y % 2u ),
		( _u.z / 2u ) + ( _u.z % 2u )
	);
	// the modulo and the division can be performed using the same CPU instruction
}


/// Indicates a spatial relation (or result of some intersection test)
struct SpatialRelation
{
	enum Enum
	{
		Outside		= 0,	// <= Must be equal to zero!
		Intersects	= 1,
		Inside		= 2,

		FORCE_DWORD = 0xFFFFFFFF
	};
};



/*
=======================================================================
	REFLECTION
=======================================================================
*/
#if MM_ENABLE_REFLECTION

#include <Base/Object/Reflection/Reflection.h>
#include <Base/Object/Reflection/StructDescriptor.h>

#if !MM_NO_INTRINSICS
mxDECLARE_STRUCT(Vector4);
mxDECLARE_POD_TYPE(Vector4);
#endif

mxDECLARE_STRUCT(V2f);
mxDECLARE_STRUCT(V3f);
mxDECLARE_STRUCT(V4f);

mxDECLARE_STRUCT(V3d);

mxDECLARE_STRUCT(Int3);
mxDECLARE_STRUCT(UInt3);

mxDECLARE_POD_TYPE(V2f);
mxDECLARE_POD_TYPE(V3f);
mxDECLARE_POD_TYPE(V4f);

mxDECLARE_POD_TYPE(Int3);
mxDECLARE_POD_TYPE(UInt3);
mxDECLARE_POD_TYPE(Int4);
mxDECLARE_POD_TYPE(UInt4);

mxDECLARE_STRUCT(Half2);
mxDECLARE_STRUCT(Half4);
mxDECLARE_STRUCT(U11U11U10);
mxDECLARE_STRUCT(R10G10B1A2);

mxREFLECT_AS_BUILT_IN_INTEGER(UByte4);

#endif // MM_ENABLE_REFLECTION

/*
==========================================================
	LOGGING
==========================================================
*/
class ATextStream;

#if MM_DEFINE_STREAM_OPERATORS

ATextStream & operator << ( ATextStream & log, const UShort3& v );
ATextStream & operator << ( ATextStream & log, const Int3& v );
ATextStream & operator << ( ATextStream & log, const Int4& v );
ATextStream & operator << ( ATextStream & log, const V2f& v );
ATextStream & operator << ( ATextStream & log, const V3f& v );
ATextStream & operator << ( ATextStream & log, const V4f& v );
ATextStream & operator << ( ATextStream & log, const Vector4& v );
ATextStream & operator << ( ATextStream & log, const UInt3& v );
ATextStream & operator << ( ATextStream & log, const UInt4& v );
ATextStream & operator << ( ATextStream & log, const UByte4& v );

ATextStream & operator << ( ATextStream & log, const V3d& v );

#endif // MM_DEFINE_STREAM_OPERATORS
