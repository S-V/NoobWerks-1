// 
#include <Base/Base.h>
#pragma hdrstop

// mt19937
#include <random>

//
#include <Base/Math/Interpolate.h>
#include <Core/Memory/MemoryHeaps.h>
//
#include <ProcGen/Noise/NwNoiseFunctions.h>


namespace Noise
{

NwPerlinNoise::NwPerlinNoise( int seed )
{
	this->setSeed( seed );
}

void NwPerlinNoise::setSeed( int seed )
{

	// Fix for Visual Studio 9 (2008)
#if defined(_MSC_VER) && _MSC_VER <= 1500
	std::tr1::mt19937						rnd_engine;
	std::tr1::uniform_real<RealT>			distr(-1.0, 1.0);
#else
	std::mt19937_64							rnd_engine;
	std::uniform_real_distribution<RealT>	distr(-1.0, 1.0);
#endif

	rnd_engine.seed((unsigned long)seed);

	/// Fill the gradients list with random normalized vectors
	for( int i = 0; i < mxCOUNT_OF(_random_gradients); i++ )
	{
		const RealT random_x = distr(rnd_engine);
		const RealT random_y = distr(rnd_engine);
		const RealT random_z = distr(rnd_engine);

		_random_gradients[i] = Vec3T::make( random_x, random_y, random_z )
			.normalized();
	}
}

NwPerlinNoise::RealT NwPerlinNoise::evalulate2D(
	const RealT x, const RealT y
	, unsigned offset
	) const
{
	// Get the nearest lattice indices
	const int x0 = (int)floor(x);	// Integer part of x
	const int y0 = (int)floor(y);	// Integer part of y
	const int x1 = x0 + 1;
	const int y1 = y0 + 1;

	// Get local deltas to lattice positions
    const RealT dx0 = x - x0;	// Fractional part of x
    const RealT dy0 = y - y0;	// Fractional part of y
	// Get vectors from the cell's corners to the point inside the unit cube
	const RealT dx1 = dx0 - RealT(1);
    const RealT dy1 = dy0 - RealT(1);

    // Get gradient projection for each local lattice point
    const RealT g00 = _projectOntoRandomGradient2D( dx0, dy0, x0, y0, offset );
    const RealT g10 = _projectOntoRandomGradient2D( dx1, dy0, x1, y0, offset );
    const RealT g01 = _projectOntoRandomGradient2D( dx0, dy1, x0, y1, offset );
    const RealT g11 = _projectOntoRandomGradient2D( dx1, dy1, x1, y1, offset );

	// Remap linear interpolation to quintic (see Perlin's Improved Noise paper).
	//const RealT tx = interpQuintic(dx0);
	//const RealT ty = interpQuintic(dy0);
	const RealT tx = interpHermite(dx0);
	const RealT ty = interpHermite(dy0);

	// Bilinear interpolation
	return TBilerp(
		g00, g10,
		g01, g11,
		tx, ty
		);
}

mxSTOLEN(
"'Very Fast, High-Precision SIMD/GPU Gradient Noise' by Don Williamson [December 17, 2012]"
"http://donw.io/post/high-prec-gradient-noise/"
);

NwPerlinNoise::RealT NwPerlinNoise::evalulate3D(
	const RealT x, const RealT y, const RealT z
	, unsigned offset
	) const
{
	// Get the nearest lattice indices
	const int x0 = (int)floor(x);	// Integer part of x
	const int y0 = (int)floor(y);	// Integer part of y
	const int z0 = (int)floor(z);	// Integer part of z
	const int x1 = x0 + 1;
	const int y1 = y0 + 1;
	const int z1 = z0 + 1;

	// Get local deltas to lattice positions
    const RealT dx0 = x - x0;	// Fractional part of x
    const RealT dy0 = y - y0;	// Fractional part of y
    const RealT dz0 = z - z0;	// Fractional part of z

	// Get vectors from the cell's corners to the point inside the unit cube
	const RealT dx1 = dx0 - RealT(1);
    const RealT dy1 = dy0 - RealT(1);
    const RealT dz1 = dz0 - RealT(1);

    // Get gradient projection for each local lattice point
    const RealT g0 = _projectOntoRandomGradient3D( dx0, dy0, dz0, x0, y0, z0, offset );
    const RealT g1 = _projectOntoRandomGradient3D( dx1, dy0, dz0, x1, y0, z0, offset );
    const RealT g2 = _projectOntoRandomGradient3D( dx0, dy1, dz0, x0, y1, z0, offset );
    const RealT g3 = _projectOntoRandomGradient3D( dx1, dy1, dz0, x1, y1, z0, offset );
    const RealT g4 = _projectOntoRandomGradient3D( dx0, dy0, dz1, x0, y0, z1, offset );
    const RealT g5 = _projectOntoRandomGradient3D( dx1, dy0, dz1, x1, y0, z1, offset );
    const RealT g6 = _projectOntoRandomGradient3D( dx0, dy1, dz1, x0, y1, z1, offset );
    const RealT g7 = _projectOntoRandomGradient3D( dx1, dy1, dz1, x1, y1, z1, offset );

	// Remap linear interpolation to quintic (see Perlin's Improved Noise paper).
	//const RealT tx = interpQuintic(dx0);
	//const RealT ty = interpQuintic(dy0);
	//const RealT tz = interpQuintic(dz0);
	const RealT tx = interpHermite(dx0);
	const RealT ty = interpHermite(dy0);
	const RealT tz = interpHermite(dz0);

	// Trilinear interpolation

	// Interpolate along x for the contributions from each of the gradients
	const RealT gx0 = TLerp( g0, g1, tx );
	const RealT gx1 = TLerp( g2, g3, tx );
	const RealT gx2 = TLerp( g4, g5, tx );
	const RealT gx3 = TLerp( g6, g7, tx );
	// Interpolate along y for the contributions from each of the gradients
	const RealT gy0 = TLerp( gx0, gx1, ty );
	const RealT gy1 = TLerp( gx2, gx3, ty );
	// Interpolate along z for the contributions from each of the gradients
	return TLerp( gy0, gy1, tz );
}

NwPerlinNoise::RealT NwPerlinNoise::_projectOntoRandomGradient2D(
	RealT dx, RealT dy
	, int ix, int iy
	, unsigned offset
	) const
{
	// A series of primes for the hash function
	const unsigned NOISE_HASH_X = 1213;
	const unsigned NOISE_HASH_Y = 6203;
	const unsigned NOISE_HASH_SEED = 1039;
	const unsigned NOISE_HASH_SHIFT = 13;

	// Hash the lattice indices to get a gradient vector index
	unsigned index
		= NOISE_HASH_X * unsigned(ix)
		+ NOISE_HASH_Y * unsigned(iy)
		+ NOISE_HASH_SEED + offset
		;
	index ^= (index >> NOISE_HASH_SHIFT);

	//index = index % mxCOUNT_OF(_random_gradients);
	mxSTATIC_ASSERT(mxCOUNT_OF(_random_gradients) == 0x100);
	index &= 0xFF;

	// Lookup the gradient vector
	const Vec3T& random_gradient = _random_gradients[ index ];

	// Project the local point onto the gradient vector
	return (random_gradient.x * dx) + (random_gradient.y * dy);
}

NwPerlinNoise::RealT NwPerlinNoise::_projectOntoRandomGradient3D(
	// The vector points from the cell's corner to the sample point and is dotted with the random gradient:
	RealT dx, RealT dy, RealT dz
	// these are used for selecting a random gradient:
	, int ix, int iy, int iz
	// for randomness
	, unsigned offset
	) const
{
	// A series of primes for the hash function
	const unsigned NOISE_HASH_X = 1213;
	const unsigned NOISE_HASH_Y = 6203;
	const unsigned NOISE_HASH_Z = 5237;
	const unsigned NOISE_HASH_SEED = 1039;
	const unsigned NOISE_HASH_SHIFT = 13;

	// Hash the lattice indices to get a gradient vector index
	unsigned index
		= NOISE_HASH_X * unsigned(ix)
		+ NOISE_HASH_Y * unsigned(iy)
		+ NOISE_HASH_Z * unsigned(iz)
		+ NOISE_HASH_SEED + offset
		;
	index ^= (index >> NOISE_HASH_SHIFT);

	//index = index % mxCOUNT_OF(_random_gradients);
	mxSTATIC_ASSERT(mxCOUNT_OF(_random_gradients) == 0x100);
	index &= 0xFF;

	// Lookup the gradient vector
	const Vec3T& random_gradient = _random_gradients[ index ];

	// Project the local point onto the gradient vector
	return (random_gradient.x * dx) + (random_gradient.y * dy) + (random_gradient.z * dz);
}


mxDEFINE_CLASS(NwTerrainNoise2D);
mxBEGIN_REFLECTION(NwTerrainNoise2D)
	mxMEMBER_FIELD( _max_octaves ),
	mxMEMBER_FIELD( _min_octaves ),
	mxMEMBER_FIELD( _lacunarity ),
	mxMEMBER_FIELD( _gain ),
	mxMEMBER_FIELD( _coarsest_frequency ),
	mxMEMBER_FIELD( _result_scale ),
	//mxMEMBER_FIELD( _exponents ),
mxEND_REFLECTION;

NwTerrainNoise2D::NwTerrainNoise2D()
{
	setDefaults();
}

void NwTerrainNoise2D::setDefaults()
{
	_max_octaves = 7;
	_min_octaves = 1;
	_lacunarity = 2;
	_gain = 0.5;
	_coarsest_frequency = 1;
	_result_scale = 1;

	recalculateExponents();
}

void NwTerrainNoise2D::recalculateExponents()
{
	//for( int i = 0; i < (int)_max_octaves; i++ )
	//{
	//	// compute spectral weight for each frequency */
	//	_exponents[i] = pow( _lacunarity, -_H * i );
	//}
}

mxBIBREF("Texturing & Modeling: A Procedural Approach (3rd edition) [2003], CHAPTER 16 Procedural Fractal Terrains, P.537");
Real NwTerrainNoise2D::evaluate2D(
	const Real x, const Real y
	, const NwPerlinNoise& perlin_noise
	, const unsigned int num_octaves
	) const
{
	Real sum = 0;
	Real amplitude = Real(1);

	Real frequency = _coarsest_frequency;

	// Compute sum of octaves of noise.
	for( unsigned int i = 0; i < num_octaves; i++ )
	{
		Real n = perlin_noise.evalulate2D(
			x * frequency,
			y * frequency,
			0//i	// add offset for more randomness
			);

		n = Real(1) - fabs( n );
		// get absolute value of signal (this creates the ridges)
		//n = fabs( n );

		// square the signal, to increase “sharpness” of ridges'
		n *= n;

		//
		sum += amplitude * n;

		amplitude *= _gain;

		// increase frequency
		frequency *= _lacunarity;

	}//For each octave.

	return sum * _result_scale;
}

int NwTerrainNoise2D::calculateNumberOfNoiseOctaves(
	const Real voxel_size
	) const
{
	const Real noise_sample_size_at_coarsest_octave = 1 / _coarsest_frequency;

	const int num_octaves_for_antialiased_noise = Noise::calculateNumberOfNoiseOctaves(
		voxel_size,
		noise_sample_size_at_coarsest_octave
		);

	return Clamp<int>(
		num_octaves_for_antialiased_noise
		, _min_octaves
		, _max_octaves
		);
}

int calculateNumberOfNoiseOctaves(
	const Real voxel_size
	, const Real coarsest_wave_size
	)
{
	mxBIBREF(
	"http://www.pbr-book.org/3ed-2018/Texture/Noise.html#fragment-ComputenumberofoctavesforantialiasedFBm-0"
	);

	mxSTOLEN(
		"Antialiased fBm:"
		"https://gitlab.cg.tuwien.ac.at/ndodik/pcbpt-mitsuba/-/blob/a31775f51664ae3442e3fab57a76c4d1221f3edc/src/librender/noise.cpp#L106"
		);

	// Let
	// d - voxel size,
	// s - sampling rate in noise space ( s = 1 / noise_frequency_at_coarsest_level ),
	// n - number of octaves for antialiased fractal noise.
	// 
	// Then, to avoid aliasing,
	// (d / 2) <= s / (lacunarity ^ n), where lacunarity = 2.
	// Therefore,
	// n <= 1 + log2( s / d ).

	const Real f_octaves = 1 + largest(
		log2( coarsest_wave_size / voxel_size ),
		0
	);
	return floor( f_octaves );
}

}//namespace Noise

/*
Fast Perlin noise:
https://www.gamedev.net/blogs/entry/1382657-fast-perlin-noise/
*/
