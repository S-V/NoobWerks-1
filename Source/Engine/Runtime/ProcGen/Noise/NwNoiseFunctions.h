// 
#pragma once

#include <ProcGen/FastNoiseSupport.h>

#define nwNOISE_CFG_USE_DOUBLES		(1)



namespace Noise
{

/*
 * Real is used only in FPU mode
*/
#if nwNOISE_CFG_USE_DOUBLES
	typedef double Real;
#else
	typedef float Real;
#endif


//enum { MAX_OCTAVES = 32 };


class NwPerlinNoise: NonCopyable
{
	//typedef float RealT;
	typedef ::Noise::Real RealT;

	typedef Tuple3<RealT> Vec3T;

	/// 3D Normalized gradients table
	Vec3T	_random_gradients[256];

public:
	NwPerlinNoise( int seed = 1337 );

	void setSeed( int seed );

	RealT evalulate2D(
		const RealT x, const RealT y
		, unsigned offset = 0
		) const;

	RealT evalulate3D(
		const RealT x, const RealT y, const RealT z
		, unsigned offset = 0
		) const;

private:
	RealT _projectOntoRandomGradient2D(
		RealT dx, RealT dy
		, int ix, int iy
		, unsigned offset
		) const;

	RealT _projectOntoRandomGradient3D(
		RealT dx, RealT dy, RealT dz
		, int ix, int iy, int iz
		, unsigned offset
		) const;
};

//
struct NwTerrainNoise2D: CStruct
{
	// Number of frequencies in the fBm.
	// default: 7.0, min: 0.0, max: 8.0
	unsigned int _max_octaves;

	// default = 1
	unsigned int _min_octaves;

	/// Gap between successive frequencies
	/// (how frequency increases with each octave).
	/// default: 2.0, min: 0.0, max: 6.0
	Real	_lacunarity;

	/// how amplitude decreases with each octave
	/// default = 0.5
	Real	_gain;

	/// noise frequency at the coarsest level;
	/// determines the "spacing" (sampling rate) in the noise space.
	/// default = 1
	Real	_coarsest_frequency;

	/// The computed noise is scaled by this value.
	/// default = 1
	Real	_result_scale;

	/// cached spectral weights for each frequency,
	/// the first is 1, the rest are monotonically decreasing
	//Real	_exponents[MAX_OCTAVES];

public:
	mxDECLARE_CLASS(NwTerrainNoise2D, CStruct);
	mxDECLARE_REFLECTION;
	NwTerrainNoise2D();

	void setDefaults();

	/// precomputes and stores spectral weights
	void recalculateExponents();

	Real evaluate2D(
		const Real x, const Real y
		, const NwPerlinNoise& perlin_noise
		, const unsigned int num_octaves
		) const;

	Real calcRidgedMultifractal3D(
		const Real x, const Real y, const Real z
		, const FastNoise& perlin
		) const;

public:
	int calculateNumberOfNoiseOctaves(
		const Real voxel_size
		) const;
};

/// Computes the number of octaves for antialiased fractal noise.
int calculateNumberOfNoiseOctaves(
	const Real voxel_size
	, const Real coarsest_wave_size
	);

}//namespace Noise
