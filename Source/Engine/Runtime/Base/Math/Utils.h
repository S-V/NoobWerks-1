/*
=============================================================================
	File:	Utils.h
	Desc:	Math helpers.
=============================================================================
*/

#ifndef __MATH_UTILS_H__
#define __MATH_UTILS_H__


mxSTOLEN("ICE");
// From Redon's thesis:
// [a,b] + [c,d] = [a+c, b+d]
// [a,b] - [c,d] = [a-d, b-c]
// [a,b] * [c,d] = [min(ac, ad, bc, bd), max(ac, ad, bc, bd)]
// 1 / [a,b] = [1/b, 1/a] if a>0 or b<0
// [a,b] / [c,d] = [a,b] * (1, [c,d]) if c>0 or d<0
// [a,b] <= [c,d] if b<=c

class Interval
{
public:
	mxINLINE				Interval()										{}
	mxINLINE				Interval(const Interval& it) : a(it.a), b(it.b)	{}
	mxINLINE				Interval(float f) : a(f), b(f)					{}
	mxINLINE				Interval(float _a, float _b) : a(_a), b(_b)		{}
	mxINLINE				~Interval()										{}

	mxINLINE	float		Width()								const		{ return b - a;									}
	mxINLINE	float		MidPoint()							const		{ return (a+b)*0.5f;							}

	// Arithmetic operators

	//! Operator for Interval Plus = Interval + Interval.
	mxINLINE	Interval	operator+(const Interval& it)		const		{ return Interval(a + it.a, b + it.b);			}

	//! Operator for Interval Plus = Interval + float.
	mxINLINE	Interval	operator+(float f)					const		{ return Interval(a + f, b + f);				}

	//! Operator for Interval Minus = Interval - Interval.
	mxINLINE	Interval	operator-(const Interval& it)		const		{ return Interval(a - it.b, b - it.a);			}

	//! Operator for Interval Minus = Interval - float.
	mxINLINE	Interval	operator-(float f)					const		{ return Interval(a - f, b - f);				}

	//! Operator for Interval Mul = Interval * Interval.
	mxINLINE	Interval	operator*(const Interval& it)		const
							{
								float ac = a*it.a;
								float ad = a*it.b;
								float bc = b*it.a;
								float bd = b*it.b;
#ifdef INTERVAL_USE_FCOMI
								float Min = FCMin4(ac, ad, bc, bd);
								float Max = FCMax4(ac, ad, bc, bd);
#else
								float Min = ac;
								if(ad<Min)	Min = ad;
								if(bc<Min)	Min = bc;
								if(bd<Min)	Min = bd;

								float Max = ac;
								if(ad>Max)	Max = ad;
								if(bc>Max)	Max = bc;
								if(bd>Max)	Max = bd;
#endif
								return Interval(Min, Max);
							}

	//! Operator for Interval Scale = Interval * float.
	mxINLINE	Interval	operator*(float s)					const
							{
								float Min = a*s;
								float Max = b*s;
								if(Min>Max)	TSwap(Min,Max);

								return Interval(Min, Max);
							}

	//! Operator for Interval Scale = float * Interval.
	mxINLINE friend	Interval operator*(float s, const Interval& it)
							{
								float Min = it.a*s;
								float Max = it.b*s;
								if(Min>Max)	TSwap(Min,Max);

								return Interval(Min, Max);
							}

	//! Operator for Interval Scale = float / Interval.
	mxINLINE	friend	Interval operator/(float s, const Interval& it)			{ return Interval(s/it.b, s/it.a);					}

	//! Operator for Point Scale = Point / float.
//		mxINLINE	Point			operator/(float s)					const		{ s = 1.0f / s; return Point(x * s, y * s, z * s);	}

	//! Operator for Interval Div = Interval / Interval.
	mxINLINE	Interval		operator/(const Interval& it)		const		{ return (*this) * (1.0f / it);						}



	//! Operator for Interval += Interval.
	mxINLINE	Interval&		operator+=(const Interval& it)					{ a += it.a; b += it.b;			return *this;		}

	//! Operator for Interval += float.
	mxINLINE	Interval&		operator+=(float f)								{ a += f; b += f;				return *this;		}

	//! Operator for Interval -= Interval.
	mxINLINE	Interval&		operator-=(const Interval& it)					{ a -= it.b; b -= it.a;			return *this;		}

	//! Operator for Interval -= float.
	mxINLINE	Interval&		operator-=(float f)								{ a -= f; b -= f;				return *this;		}

	//! Operator for Interval *= Interval.
	mxINLINE	Interval&		operator*=(const Interval& it)
							{
								float ac = a*it.a;
								float ad = a*it.b;
								float bc = b*it.a;
								float bd = b*it.b;
#ifdef INTERVAL_USE_FCOMI
								a = FCMin4(ac, ad, bc, bd);
								b = FCMax4(ac, ad, bc, bd);
#else
								a = ac;
								if(ad<a)	a = ad;
								if(bc<a)	a = bc;
								if(bd<a)	a = bd;

								b = ac;
								if(ad>b)	b = ad;
								if(bc>b)	b = bc;
								if(bd>b)	b = bd;
#endif
								return *this;
							}

	//! Operator for Interval /= Interval.
	mxINLINE	Interval&		operator /= (const Interval& it)
							{
								*this *= 1.0f / it;
								return *this;
							}

	//! Operator for "Interval A = Interval B"
	mxINLINE	void			operator = (const Interval& interval)
							{
								a = interval.a;
								b = interval.b;
							}

	// Logical operators

	//! Operator for "if(Interval<=Interval)"
	mxINLINE	bool			operator<=(const Interval& it)		const		{ return b <= it.a;	}
	//! Operator for "if(Interval<Interval)"
	mxINLINE	bool			operator<(const Interval& it)		const		{ return b < it.a;	}
	//! Operator for "if(Interval>=Interval)"
	mxINLINE	bool			operator>=(const Interval& it)		const		{ return a >= it.b;	}
	//! Operator for "if(Interval>Interval)"
	mxINLINE	bool			operator>(const Interval& it)		const		{ return a > it.b;	}

	float	a, b;
};

#if 0
// http://www.gamasutra.com/features/19991018/Gomez_4.htm (retrieved 2007/04/27)
// [1] J. Arvo. A simple method for box-sphere intersection testing. In A. Glassner, editor, Graphics Gems, pp. 335-339, Academic Press, Boston, MA, 1990.
//
inline FASTBOOL AabbSphereIntersection( const AABB& aabb, const Sphere& sphere )
{
	FLOAT s;
	FLOAT d = 0.0f; 

	// find the square of the distance from the sphere to the box
	for( UINT i = 0; i < 3; i++ )
	{
		if( sphere.center[i] < aabb.mPoints[0][i] )
		{
			s = sphere.center[i] - aabb.mPoints[0][i];
			d += s * s; 
		}
		else if( sphere.center[i] > aabb.mPoints[1][i] )
		{
			s = sphere.center[i] - aabb.mPoints[1][i];
			d += s * s; 
		}
	}
	return ( d <= squaref( sphere.Radius ) );
}
#endif


#if 0
inline bool rayAABBIntersection( const V3f &rayOrig, const V3f &rayDir, 
								const V3f &mins, const V3f &maxs )
{
	// SLAB based optimized ray/AABB intersection routine
	// Idea taken from http://ompf.org/ray/

	float l1 = (mins.x - rayOrig.x) / rayDir.x;
	float l2 = (maxs.x - rayOrig.x) / rayDir.x;
	float lmin = minf( l1, l2 );
	float lmax = maxf( l1, l2 );

	l1 = (mins.y - rayOrig.y) / rayDir.y;
	l2 = (maxs.y - rayOrig.y) / rayDir.y;
	lmin = maxf( minf( l1, l2 ), lmin );
	lmax = minf( maxf( l1, l2 ), lmax );

	l1 = (mins.z - rayOrig.z) / rayDir.z;
	l2 = (maxs.z - rayOrig.z) / rayDir.z;
	lmin = maxf( minf( l1, l2 ), lmin );
	lmax = minf( maxf( l1, l2 ), lmax );

	if( (lmax >= 0.0f) & (lmax >= lmin) )
	{
		// Consider length
		const V3f rayDest = rayOrig + rayDir;
		V3f rayMins( minf( rayDest.x, rayOrig.x), minf( rayDest.y, rayOrig.y ), minf( rayDest.z, rayOrig.z ) );
		V3f rayMaxs( maxf( rayDest.x, rayOrig.x), maxf( rayDest.y, rayOrig.y ), maxf( rayDest.z, rayOrig.z ) );
		return 
			(rayMins.x < maxs.x) && (rayMaxs.x > mins.x) &&
			(rayMins.y < maxs.y) && (rayMaxs.y > mins.y) &&
			(rayMins.z < maxs.z) && (rayMaxs.z > mins.z);
	}
	else
		return false;
}
#endif
mxFORCEINLINE
void MulTexCoords( V2f& uv, const V2f& scale )
{
	uv.x *= scale.x;
	uv.y *= scale.y;
}

// pyramid scaling function
template< typename TYPE >
TYPE PYR( const TYPE& n ) {
	return (n*(n+1)/2);
}

// computes the cubic root of the given argument
mxFORCEINLINE
FLOAT CubeRoot( FLOAT x ) {
	return mmExp( FLOAT(1.0/3.0) * mmLn(x) );
}


mxFORCEINLINE float CalcDamping( float damping_factor, float delta_time )
{
	return maxf(1.f - (damping_factor * delta_time), 0);
}


#endif /* !__MATH_UTILS_H__ */
