// Atmosphere rendering.
#pragma once


namespace Atmosphere_Rendering
{

/*
===============================================================================

	ATMOSPHERE / LIGHT SCATTERING

===============================================================================
*/

struct NwAtmosphereParameters: CStruct
{
	/// The inner (planetary) radius (excluding the atmosphere), in meters.
	float	inner_radius;

	/// The outer (atmosphere) radius, in meters.
	float	outer_radius;


	//

	/// Rayleigh scattering coefficient at sea level ("Beta R") in m^-1, default = (3.8e-6f, 13.5e-6f, 33.1e-6f)//( 5.5e-6, 13.0e-6, 22.4e-6 )
	/// affects the color of the sky
	/// the amount Rayleigh scattering scatters the colors (for earth: causes the blue atmosphere)
	V3f	beta_Rayleigh;

	/// Mie scattering coefficient at sea level ("Beta M") in m^-1, default = 21e-6
	/// affects the color of the blob around the sun
	/// the amount mie scattering scatters colors
	V3f	beta_Mie;


	// the heights - how far to go up before the scattering has no effect:

	/// Rayleigh scale height (atmosphere thickness if density were uniform) in meters, default = 7994
	/// How high do you have to go before there is no Rayleigh scattering?
	float	height_Rayleigh;

	/// Mie scale height (atmosphere thickness if density were uniform) in meters, default = 1.2e3
	/// How high do you have to go before there is no Mie scattering?
	float	height_Mie;

	/// Mie preferred scattering direction (aka "mean cosine" or the Mie phase asymmetry constant), default = 0.8
	/// The Mie phase function includes a term g (the Rayleigh phase function doesn't)
	/// which controls the anisotropy of the medium. Aerosols exhibit a strong forward directivity.
	/// the direction mie scatters the light in (like a cone). closer to -1 means more towards a single direction
	float	g;


	// Ozone layer:

//#define HEIGHT_ABSORPTION 30e3 /* at what height the absorption is at it's maximum */
//#define ABSORPTION_FALLOFF 3e3 /* how much the absorption decreases the further away it gets from the maximum height */

	//

	/// The Sun's brightness, default = 22
	/// affects the brightness of the atmosphere
	float	sun_intensity;

	/// how much extra the atmosphere blocks light
	float density_multiplier;

	/// the amount of scattering that always occurs, can help make the back side of the atmosphere a bit brighter
	V3f beta_ambient;


/// R = the wavelength of the Red color channel, default = 650 nm
/// G = the wavelength of the Green color channel, default = 570 nm
/// B = the wavelength of the Blue color channel, default = 475 nm
//	float wavelength[3];

public:
	mxDECLARE_CLASS( NwAtmosphereParameters, CStruct );
	mxDECLARE_REFLECTION;
	NwAtmosphereParameters();

	void preset_Earth()
	{
		inner_radius = 6371e3;
		outer_radius = 6471e3;

		//
		beta_Rayleigh = CV3f( 3.8e-6f, 13.5e-6f, 33.1e-6f );
		beta_Mie = CV3f( 21e-6 );

		height_Rayleigh = 7994.0f;
		height_Mie = 1.2e3;

		g = 0.758f;

		density_multiplier = 1;

		beta_ambient = CV3f(0);

		sun_intensity = 22.0f;
	}

	//void preset_Mars()
};


struct NwAtmosphereRenderingParameters
{
	NwAtmosphereParameters	atmosphere;

	/// eye position relative to the atmosphere (the atmosphere is centered at the origin)
	V3f	relative_eye_position;
};







enum Luminance
{
	// Render the spectral radiance at kLambdaR, kLambdaG, kLambdaB.
	NONE = 0,
	// Render the sRGB luminance, using an approximate (on the fly) conversion
	// from 3 spectral radiance values only (see section 14.3 in <a href=
	// "https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
	//  Evaluation of 8 Clear Sky Models</a>).
	APPROXIMATE = 1,
	// Render the sRGB luminance, precomputed from 15 spectral radiance values
	// (see section 4.4 in <a href=
	// "http://www.oskee.wz.cz/stranka/uploads/SCCG10ElekKmoch.pdf">Real-time
	//  Spectral Scattering in Large-scale Natural Participating Media</a>).
	PRECOMPUTED = 2
};

struct AtmosphereOptions
{
	Luminance use_luminance;
	bool use_half_precision;
	bool use_constant_solar_spectrum;
	bool use_ozone_layer;

public:
	AtmosphereOptions();
	void SetDefaults();
};



class NwAtmosphereModuleI
{
public:
	static NwAtmosphereModuleI* create(
		AllocatorI & storage
		, NwClump & storage_clump
		);
	static void destroy( NwAtmosphereModuleI * o );

public:
	virtual ERet Initialize() = 0;

protected:
	virtual ~NwAtmosphereModuleI() {}
};


}//namespace Atmosphere_Rendering
