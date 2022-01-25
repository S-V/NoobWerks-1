/*
=============================================================================
	File:	Random_Marsaglia.h
	Desc:	 KISS (‘Keep it Simple Stupid’) is an efficient pseudo-random number generator
	originally  specified  by  G.  Marsaglia  and  A.  Zamanin 1993.
	G. Marsaglia in 1998 posted a C version to various USENET newsgroups, includingsci.crypt.
		Marsaglia’s generators are primarily intended for scientific applications suchas Monte Carlo simulations. 

	References:
	KISS: A Bit Too Simple
	https://eprint.iacr.org/2011/007.pdf
=============================================================================
*/

#pragma once

/*
Here's a small random number generator developed by George Marsaglia.
He's an expert in the field, so you can be confident the generator has good statistical properties.
Here u and v are unsigned ints. Initialize them to any non-zero values.
Each time you generate a random number, store u and v somewhere.
*/
mxFORCEINLINE U32 RandomU32_Marsaglia( U32 & v, U32 & u )
{
	v = 36969*(v & 65535) + (v >> 16);
	u = 18000*(u & 65535) + (u >> 16);
	return (v << 16) + u;
}

//
//	Random_Marsaglia
//
class Random_Marsaglia {
public:
						Random_Marsaglia( INT seed = 0 );

	void				SetSeed( INT seed );
	INT					GetSeed( void ) const;

	INT					RandomInt( void );			// random integer in the range [0, MAX_RAND]
	INT					RandomInt( INT max );		// random integer in the range [0, max]
	FLOAT				RandomFloat( void );		// random number in the range [0.0f, 1.0f]
	FLOAT				CRandomFloat( void );		// random number in the range [-1.0f, 1.0f]

	FLOAT				RandomFloat( FLOAT low, FLOAT high );	// random number in the range [low, high]

	static const INT	MAX_RAND = 0x7fff;

private:
	INT					seed;
};

mxFORCEINLINE Random_Marsaglia::Random_Marsaglia( INT seed ) {
	this->seed = seed;
}

mxFORCEINLINE void Random_Marsaglia::SetSeed( INT seed ) {
	this->seed = seed;
}

mxFORCEINLINE INT Random_Marsaglia::GetSeed( void ) const {
	return seed;
}

mxFORCEINLINE INT Random_Marsaglia::RandomInt( void ) {
	seed = 69069 * seed + 1;
	return ( seed & Random_Marsaglia::MAX_RAND );
}

mxFORCEINLINE INT Random_Marsaglia::RandomInt( INT max ) {
	if ( max == 0 ) {
		return 0;			// avoid divide by zero error
	}
	return RandomInt() % max;
}

mxFORCEINLINE FLOAT Random_Marsaglia::RandomFloat( void ) {
	return ( RandomInt() / ( FLOAT )( Random_Marsaglia::MAX_RAND + 1 ) );
}

mxFORCEINLINE FLOAT Random_Marsaglia::CRandomFloat( void ) {
	return ( 2.0f * ( RandomFloat() - 0.5f ) );
}

mxFORCEINLINE FLOAT Random_Marsaglia::RandomFloat( FLOAT low, FLOAT high ) {
	return low + RandomFloat() * ( high - low );
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
