// 
#pragma once

#include <ProcGen/FastNoiseSupport.h>


namespace Noise
{

enum { MAX_OCTAVES = 32 };

// Hybrid additive/multiplicative multifractal terrain model.

struct HybridMultiFractal: CStruct
{
	//Number of frequencies in the fBm.
	//default: 7.0, min: 0.0, max: 8.0
	unsigned int _num_octaves;

	/// Gap between successive frequencies
	/// (how frequency increases with each octave).
	/// default: 2.0, min: 0.0, max: 6.0
	float	_lacunarity;

	/// how amplitude decreases with each octave
	float	_gain;

	/// Fractal increment parameter
	/// determines the highest fractal dimension
	/// (controls the roughness of the fractal).
	/// default: 0.25, min: 0.0, max: 2.0
	float	_H;

	/// This value is added to the noise before multiplications.
	/// 0.7 - 0.8 are good values.
	float	_offset;


	float	_result_scale;

	/// cached spectral weights for each frequency,
	/// the first is 1, the rest are monotonically decreasing
	float	_exponents[MAX_OCTAVES];

public:
	mxDECLARE_CLASS(HybridMultiFractal, CStruct);
	mxDECLARE_REFLECTION;
	HybridMultiFractal();

	/// precomputes and stores spectral weights
	void recalculateExponents();

	float evaluate3D(
		const float x, const float y, const float z
		, const FastNoise& fast_noise
		) const;

	float evaluate2D(
		const float x, const float y
		, const FastNoise& fast_noise
		) const;

	/// Procedural multifractal evaluated at "point".
	float calcMultifractal2D(
		const float x, const float y
		, const FastNoise& fBm
		) const;

	/// Ridged multifractal terrain model.
	float calcRidgedMultifractal2D(
		const float x, const float y
		, const FastNoise& perlin
		) const;

	float calcRidgedMultifractal3D(
		const float x, const float y, const float z
		, const FastNoise& perlin
		) const;
};

}//namespace Noise
