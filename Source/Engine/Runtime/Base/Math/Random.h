/*
=============================================================================
	File:	Random.h
	Desc:	Random number generator.
			Originally written by Id Software ( Random.h ) -
			- Copyright (C) 2004 Id Software, Inc. ( NwRandom )
=============================================================================
*/

#ifndef __MATH_RANDOM_H__
#define __MATH_RANDOM_H__


mxSTOLEN("idSoftware, Doom3/Prey/Quake4 SDKs");
//
//	NwRandom
//

class NwRandom
{
	int					seed;

public:
						NwRandom( int seed = 0 );

	void				SetSeed( int seed );
	int					GetSeed( void ) const;

	int					RandomInt( void );			// random integer in the range [0, MAX_RAND]
	int					RandomInt( int max );		// random integer in the range [0, max]
	float01_t			GetRandomFloat01( void );		// random number in the range [0.0f, 1.0f]
	float				GetRandomFloatMinus1Plus1( void );		// random number in the range [-1.0f, 1.0f]

	/// random number in the range [low, high]
	float	GetRandomFloatInRange( float low, float high );

	static const int	MAX_RAND = 0x7fff;
};

mxFORCEINLINE NwRandom::NwRandom( int seed ) {
	this->seed = seed;
}

mxFORCEINLINE void NwRandom::SetSeed( int seed ) {
	this->seed = seed;
}

mxFORCEINLINE int NwRandom::GetSeed( void ) const {
	return seed;
}

mxFORCEINLINE int NwRandom::RandomInt( void ) {
	seed = 69069 * seed + 1;
	return ( seed & NwRandom::MAX_RAND );
}

mxFORCEINLINE int NwRandom::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;			// avoid divide by zero error
	}
	return RandomInt() % max;
}

mxFORCEINLINE float NwRandom::GetRandomFloat01( void ) {
	return ( RandomInt() / ( float )( NwRandom::MAX_RAND + 1 ) );
}

mxFORCEINLINE float NwRandom::GetRandomFloatMinus1Plus1( void ) {
	return ( 2.0f * ( GetRandomFloat01() - 0.5f ) );
}

mxFORCEINLINE float01_t NwRandom::GetRandomFloatInRange( float low, float high ) {
	return low + GetRandomFloat01() * ( high - low );
}
#if 0
/*
=============================================================================

	Random number generator

=============================================================================
*/

//
//	mxRandom2
//
class mxRandom2 {
public:
							mxRandom2( unsigned long seed = 0 );

	void					SetSeed( unsigned long seed );
	unsigned long			GetSeed( void ) const;

	int						RandomInt( void );			// random integer in the range [0, MAX_RAND]
	int						RandomInt( int max );		// random integer in the range [0, max]
	float					RandomFloat( void );		// random number in the range [0.0f, 1.0f]
	float					CRandomFloat( void );		// random number in the range [-1.0f, 1.0f]

	static const int		MAX_RAND = 0x7fff;

private:
	unsigned long			seed;

	static const unsigned long	IEEE_ONE = 0x3f800000;
	static const unsigned long	IEEE_MASK = 0x007fffff;
};

mxFORCEINLINE mxRandom2::mxRandom2( unsigned long seed ) {
	this->seed = seed;
}

mxFORCEINLINE void mxRandom2::SetSeed( unsigned long seed ) {
	this->seed = seed;
}

mxFORCEINLINE unsigned long mxRandom2::GetSeed( void ) const {
	return seed;
}

mxFORCEINLINE int mxRandom2::RandomInt( void ) {
	seed = 1664525L * seed + 1013904223L;
	return ( (int) seed & mxRandom2::MAX_RAND );
}

mxFORCEINLINE int mxRandom2::RandomInt( int max ) {
	if ( max == 0 ) {
		return 0;		// avoid divide by zero error
	}
	return ( RandomInt() >> ( 16 - Math::BitsForInteger( max ) ) ) % max;
}

mxFORCEINLINE float mxRandom2::RandomFloat( void ) {
	unsigned long i;
	seed = 1664525L * seed + 1013904223L;
	i = mxRandom2::IEEE_ONE | ( seed & mxRandom2::IEEE_MASK );
	return ( ( *(float *)&i ) - 1.0f );
}

mxFORCEINLINE float mxRandom2::CRandomFloat( void ) {
	unsigned long i;
	seed = 1664525L * seed + 1013904223L;
	i = mxRandom2::IEEE_ONE | ( seed & mxRandom2::IEEE_MASK );
	return ( 2.0f * ( *(float *)&i ) - 3.0f );
}
#endif

#endif /* !__MATH_RANDOM_H__ */

