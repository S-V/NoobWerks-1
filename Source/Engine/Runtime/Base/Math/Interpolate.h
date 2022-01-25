/*
=============================================================================
	File:	Interpolate.h
	Desc:	Interpolation.
=============================================================================
*/
#pragma once

#include <Base/Math/MiniMath.h>


/// Linear interpolation
template< typename T, typename Scalar >
static mxFORCEINLINE
T TLerp( const T& a, const T& b, const Scalar amount )
{
	// the same as "a * (1 - amount) + b * amount"
	return a + ( b - a ) * amount;
}

/// Bilinear interpolation
template< typename T, typename Scalar >
static mxFORCEINLINE
T TBilerp(
	const T& p00, const T& p10,
	const T& p01, const T& p11,
	const Scalar fracX, const Scalar fracY
){
	return TLerp(
		TLerp( p00, p10, fracX ),	// Interpolate along x
		TLerp( p01, p11, fracX ),	// Interpolate along x
		fracY	// Interpolate along y
	);
}



/// cubic Hermite curve.  Same as SmoothStep()
template< typename Scalar >
static mxFORCEINLINE
const Scalar interpHermite( const Scalar t )
{
	return t * t * (
		Scalar(3) - Scalar(2) * t
		);
}

/// quintic interpolation curve
template< typename Scalar >
static mxFORCEINLINE
const Scalar interpQuintic( const Scalar t )
{
	return t * t * t * (
		t * ( t * Scalar(6) - Scalar(15) )
		+ Scalar(10)
		);
}





// http://en.wikipedia.org/wiki/Bilinear_interpolation
float InterpolateBilinear(
						  const V2f& xy,
						  const V2f& min,
						  const V2f& max,
						  const float Q12, const float Q22,	// upper values
						  const float Q11, const float Q21	// lower values
						  );


inline float GetLerpParam( float min, float max, float x )
{
	mxASSERT( x >= min && x <= max );
	return (x - min) / (max - min);
}
inline float Float_Lerp01( float min, float max, float t )
{
	mxASSERT( t >= 0.0f && t <= 1.0f );
	return min + (max - min) * t;
}

// http://en.wikipedia.org/wiki/Bilinear_interpolation
inline
float InterpolateBilinear2(
	const V2f& xy,
	const V2f& min,
	const V2f& max,
	const float Q12, const float Q22,	// upper values
	const float Q11, const float Q21	// lower values
	)
{
	float tx = GetLerpParam( min.x, max.x, xy.x );
	float xl = Float_Lerp01( Q11, Q21, tx );
	float xu = Float_Lerp01( Q12, Q22, tx );

	float ty = GetLerpParam( min.y, max.y, xy.y );
	float result = Float_Lerp01( xl, xu, ty );
	return result;
}

float InterpolateTrilinear(
	const V3f& point,
	const V3f& min, const V3f& max,
	const float values[8]
);



// Cubic interpolation.
// P - end points
// T - tangent directions at end points
// Alpha - distance along spline
mxSTOLEN("Unreal Engine");
template< class T, class U > T CubicInterp( const T& P0, const T& T0, const T& P1, const T& T1, const U& A )
{
	FLOAT A3 = mmPow( A, 3 );
	FLOAT A2 = mmPow( A, 2 );

	return (T)(((2*A3)-(3*A2)+1) * P0) + ((A3-(2*A2)+A) * T0) + ((A3-A2) * T1) + (((-2*A3)+(3*A2)) * P1);
}

/**
 *  Performs Catmull-Rom interpolation between 4 values.
 *  @param _R Result type
 *  @param _A Value type
 *  @param _T Time type
 */
template<typename _R, typename _A, typename _T>
_R const math_catmull_rom( _A const& a0, _A const& a1, _A const& a2, _A const& a3, _T const& t )
{
	_T tt = t*t;
	_T ttt = t*tt;
	return _R( (a0 * ( -t     + tt+tt   - ttt      ) +
				a1 * ( 2.0f   - 5.0f*tt + ttt*3.0f ) +
				a2 * (  t     + 4.0f*tt - ttt*3.0f ) +
				a3 * (        - tt      + ttt      ) ) * 0.5f );
};


/**
 *  Smoothing function.
 *
 *  Uses critically damped spring for ease-in/ease-out smoothing. Stable
 *  at any time intervals. Based on GPG4 article.
 *
 *  @param from	Current value.
 *  @param to	Target value (may be moving).
 *	@param vel	Velocity (updated by the function, should be maintained between calls).
 *	@param smoothTime	Time in which the target should be reached, if travelling at max. speed.
 *	@param dt	Delta time.
 *	@return Updated value.
 */
template< typename T >
inline T smoothCD( const T& from, const T& to, T& vel, float smoothTime, float dt )
{
	float omega = 2.0f / smoothTime;
	float x = omega * dt;
	// approximate exp()
	float exp = 1.0f / (1.0f + x + 0.48f*x*x + 0.235f*x*x*x );
	T change = from - to;
	T temp = ( vel + omega * change ) * dt;
	vel = ( vel - omega * temp ) * exp;
	return to + ( change + temp ) * exp;
}

template<class T>
inline T cosineInterpolate(T const &start, T const &end, FLOAT const t)
{
    if (t <= 0)
    {
            return start;
    }
    else
    if (t >= 1)
    {
            return end;
    }

    T output;
    for (unsigned int i = 0; i < sizeof(T) / sizeof(float); ++i)
    {
            FLOAT mu2 = (1.0f - mmCos(t * M_PI)) / 2.0f;
            output[i] = start[i] * (1 - mu2) + end[i] * mu2;
    }

    return (output);
}

template<class T>
inline T linearInterpolate(T const &start, T const &end, FLOAT const t)
{
    if (t <= 0)
    {
            return start;
    }
    else
    if (t >= 1)
    {
            return end;
    }


    T output;
    for (unsigned int i = 0; i < sizeof(T) / sizeof(float); ++i)
    {
            output[i] = start[i] * (1 - t) + end[i] * t;
    }

    return (output);
}

template<class T>
inline T cubicInterpolate(T const &before, T const &start, T const &end, T const &after, FLOAT const t)
{
    if (t <= 0)
    {
            return start;
    }
    else
    if (t >= 1)
    {
            return end;
    }


    T output;
    for (unsigned int i = 0; i < sizeof(T) / sizeof(float); ++i)
    {
            FLOAT mu2 = t * t;
            FLOAT a0 = after[i] - end[i] - before[i] + start[i];
            FLOAT a1 = before[i] - start[i] - a0;
            FLOAT a2 = end[i] - before[i];
            FLOAT a3 = start[i];

            output[i] = a0 * t * mu2 + a1 * mu2 + a2 * t + a3;
    }

    return (output);
}

template<class T>
inline T bestInterpolate(T* const vec, size_t const size, FLOAT const t)
{
    if (size >= 4)
    {
            return cubicInterpolate(vec[size - 4], vec[size - 3], vec[size - 2], vec[size - 1], t);
    } else
    if (size == 3)
    {
            if (t > 1)
            {
                    return cosineInterpolate(vec[1], vec[2], t);
            }
            else
            {
                    return cosineInterpolate(vec[0], vec[1], t);
            }
    } else
    if (size == 2)
    {
            return cosineInterpolate(vec[0], vec[1], t);
    } else
    if (size == 1)
    {
            return vec[0];
    }

    return CV3f(0, 0, 0);
}
