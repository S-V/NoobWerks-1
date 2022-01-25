// 
#include <Base/Base.h>
#pragma hdrstop

//
#include <Core/Memory/MemoryHeaps.h>
//
#include <Planets/Noise.h>


namespace Noise
{

mxDEFINE_CLASS(HybridMultiFractal);
mxBEGIN_REFLECTION(HybridMultiFractal)
	mxMEMBER_FIELD( _num_octaves ),
	mxMEMBER_FIELD( _lacunarity ),
	mxMEMBER_FIELD( _gain ),
	mxMEMBER_FIELD( _H ),
	mxMEMBER_FIELD( _offset ),
	mxMEMBER_FIELD( _result_scale ),
	mxMEMBER_FIELD( _exponents ),
mxEND_REFLECTION;

HybridMultiFractal::HybridMultiFractal()
{
	_num_octaves = 7;
	_lacunarity = 2;
	_gain = 2.0f;
	_offset = 0.8f;
	_H = 1;//0.25;
	_result_scale = 1;

	recalculateExponents();
}

void HybridMultiFractal::recalculateExponents()
{
	for( int i = 0; i < (int)_num_octaves; i++ )
	{
		// compute spectral weight for each frequency */
		_exponents[i] = pow( _lacunarity, -_H * i );
	}
}



//#define MAP_FROM_MINUS_PLUS_ONE_TO_ZERO_ONE( X )	((X) * 0.5f + 0.5f)
#define MAP_FROM_MINUS_PLUS_ONE_TO_ZERO_ONE( X )	(1.0f - abs(X))


float HybridMultiFractal::evaluate3D(
	const float x, const float y, const float z
	, const FastNoise& fast_noise
	) const
{
	// get first octave of function
	float result = (
		MAP_FROM_MINUS_PLUS_ONE_TO_ZERO_ONE(fast_noise.GetNoise( x, y, z ))
		+ _offset
		) * _exponents[0];

	// alley floors should be smooth, as compared to the mountains.
	// this  could  be  accomplished  by  scaling  higher frequencies
	// in  the  summation  by  the  value  of  the  previous  frequency.
	float weight = result;
	float frequency = _lacunarity;

	for( unsigned int i = 1; i < _num_octaves; i++ )
	{
		// prevent divergence
		if( weight > 1 ) {
			weight = 1;
		}

		// get next higher frequency
		float signal = (
				MAP_FROM_MINUS_PLUS_ONE_TO_ZERO_ONE(fast_noise.GetNoise(
					x * frequency,
					y * frequency,
					z * frequency
				))
				+ _offset
			) * _exponents[i];

		// add it in, weighted by previous freq's local result
		result += weight * signal;

		// update the (monotonically decreasing) weighting result
		// (this is why H must specify a high fractal dimension)
		weight *= signal;

		// increase frequency
		frequency *= _lacunarity;
	}

	return result * _result_scale;
}


float HybridMultiFractal::evaluate2D(
	const float x, const float y
	, const FastNoise& fast_noise
	) const
{
	// get first octave of function
	float result = (
		MAP_FROM_MINUS_PLUS_ONE_TO_ZERO_ONE(fast_noise.GetNoise( x, y ))
		+ _offset
		) * _exponents[0];

	// alley floors should be smooth, as compared to the mountains.
	// this  could  be  accomplished  by  scaling  higher frequencies
	// in  the  summation  by  the  value  of  the  previous  frequency.
	float weight = result;
	float frequency = _lacunarity;

	for( unsigned int i = 1; i < _num_octaves; i++ )
	{
		// prevent divergence
		if( weight > 1 ) {
			weight = 1;
		}

		// get next higher frequency
		float signal = (
				MAP_FROM_MINUS_PLUS_ONE_TO_ZERO_ONE(fast_noise.GetNoise(
					x * frequency,
					y * frequency
				))
				+ _offset
			) * _exponents[i];

		// add it in, weighted by previous freq's local result
		result += weight * signal;

		// update the (monotonically decreasing) weighting result
		// (this is why H must specify a high fractal dimension)
		weight *= signal;

		// increase frequency
		frequency *= _lacunarity;
	}

	return result * _result_scale;
}

mxBIBREF("Texturing & Modeling: A Procedural Approach (3rd edition) [2003], CHAPTER 16 Procedural Fractal Terrains, P.440");
float HybridMultiFractal::calcMultifractal2D(
	const float x, const float y
	, const FastNoise& fBm
	) const
{
	float value = 1;
	float frequency = 1;

	for( unsigned int i = 0; i < _num_octaves; i++ )
	{
		value *= ( fBm.GetNoise( x * frequency, y * frequency ) + _offset ) * _exponents[i];
		// increase frequency
		frequency *= _lacunarity;
	}

	return value * _result_scale;
}

mxBIBREF("Texturing & Modeling: A Procedural Approach (3rd edition) [2003], CHAPTER 16 Procedural Fractal Terrains, P.537");
float HybridMultiFractal::calcRidgedMultifractal2D(
	const float x, const float y
	, const FastNoise& perlin
	) const
{

#if 0

	// get first octave of function
	float signal = perlin.GetNoise( x, y );


	//

	// get absolute value of signal (this creates the ridges)
	signal = fabs(signal);

	// invert and translate (note that "offset" should be ~= 1.0)
	signal = _offset - signal;

	// square the signal, to increase “sharpness” of ridges'
	signal *= signal;

	//



	// assign initial values

	float result = signal;
	float weight = 1;
	float frequency = 1;

	for( unsigned int i = 1; i < _num_octaves; i++ )
	{
		// increase frequency
		frequency *= _lacunarity;

		// weight successive contributions by previous signal
		weight = signal * _gain;

		//
		weight = clampf( weight, 0, 1 );

		//
		signal = perlin.GetNoise( x * frequency, y * frequency );

		//

		// get absolute value of signal (this creates the ridges)
		signal = fabs(signal);

		// invert and translate (note that "offset" should be ~= 1.0)
		signal = _offset - signal;

		// square the signal, to increase “sharpness” of ridges'
		signal *= signal;

		//


		// weight the contribution

		signal *= weight;

		result += signal * _exponents[i];

	}//For each octave.

	return result * _result_scale;

#else

	// lacunarity = 2
	// gain = 0.5

	float sum=0;
	float amp=1.0;

	float frequency = 1;

	for(unsigned int i=0; i<_num_octaves; ++i)
	{
		float n=perlin.GetNoise( x * frequency, y * frequency );

		n=1.0-fabs(n);

		n *= n;

		//
		sum+=amp*n;

		amp*=_gain;

		// increase frequency
		frequency *= _lacunarity;
	}
	return sum * _result_scale;

#endif

}

float HybridMultiFractal::calcRidgedMultifractal3D(
	const float x, const float y, const float z
	, const FastNoise& perlin
	) const
{
	// lacunarity = 2
	// gain = 0.5

	float sum=0;
	float amp=1.0;

	float frequency = 1;

	for(unsigned int i=0; i<_num_octaves; ++i)
	{
		float n=perlin.GetNoise( x * frequency, y * frequency, z * frequency );

		n=1.0-fabs(n);

		n *= n;

		//
		sum+=amp*n;

		amp*=_gain;

		// increase frequency
		frequency *= _lacunarity;
	}

	return sum * _result_scale;
}

// Fractal Brownian Motion
// The Book of Shaders: Fractal Brownian Motion
// https://thebookofshaders.com/13/

float fBm_2D(
		  V2f p,

		  int octaves,

		  // how frequency increases with each octave
		  float lacunarity = 2.0,

		  // how amplitude decreases with each octave
		  float gain = 0.5
		  )
{
	float amplitude = 1;
	float frequency = 1;
	float sum = 0;
UNDONE;
#if 0
	for( int i = 0; i < octaves; i++ )
	{
		sum += inoise(p*frequency) * amplitude;
		frequency *= lacunarity;
		amplitude *= gain;
	}
#endif

	return sum;
}


}//namespace Noise
