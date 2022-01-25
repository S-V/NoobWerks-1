#ifndef FBM_HLSLI
#define FBM_HLSLI

#include <Noise/SimplexNoise3D.hlsli>

/// Fractional Brownian Motion
/// https://thebookofshaders.com/13/
/// http://iquilezles.org/www/articles/fbm/fbm.htm
///

struct fBM_SimplexNoise3D
{
	int		_num_octaves;
	float	_frequency;
	float	_amplitude;
	float	_lacunarity;
	float	_gain;

	void SetDefaults()
	{
		_num_octaves	= 1;
		_frequency	= 1.0f;
		_amplitude	= 0.5f;
		_lacunarity	= 2.0f;
		_gain		= 0.5f;
	}
	
	float Evaluate(
		in vec3 P

		// essentially an fBm, but constructed from the absolute value of a signed noise to create sharp valleys in the function
		, in bool turbulence
		)
	{
		float value = 0.0f;

		float frequency = _frequency;
		float amplitude = _amplitude;

		[unroll]
		for( int i = 0; i < _num_octaves; i++ )
		{
			float noise_value = snoise( P * frequency );	// [-1..+1]
			if( turbulence ) {
				noise_value = abs(noise_value);
			}
			value += noise_value * amplitude;
			amplitude *= _gain;
			frequency *= _lacunarity;
		}

		return value;
	}
};

#endif // FBM_HLSLI
