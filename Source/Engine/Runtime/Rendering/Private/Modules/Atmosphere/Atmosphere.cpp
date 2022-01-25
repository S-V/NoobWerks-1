// Sky rendering.
#include <Base/Base.h>
#pragma hdrstop

#include <assert.h>

#include <Core/Assets/AssetManagement.h>
#include <Core/ObjectModel/Clump.h>

#include <GPU/Public/graphics_types.h>
#include <GPU/Public/render_context.h>
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_shader_system.h>
//#include <Graphics/private/backend_d3d11.h>
//
#include <Rendering/Private/Modules/Atmosphere/Atmosphere.h>
#include <Rendering/Private/Shaders/Shared/atmosphere_definitions.h>



namespace Atmosphere_Rendering
{

mxDEFINE_CLASS( NwAtmosphereParameters );
mxBEGIN_REFLECTION( NwAtmosphereParameters )
	mxMEMBER_FIELD( inner_radius ),
	mxMEMBER_FIELD( outer_radius ),

	mxMEMBER_FIELD( beta_Rayleigh ),
	mxMEMBER_FIELD( beta_Mie ),

	mxMEMBER_FIELD( height_Rayleigh ),
	mxMEMBER_FIELD( height_Mie ),

	mxMEMBER_FIELD( g ),

	mxMEMBER_FIELD( sun_intensity ),
	mxMEMBER_FIELD( density_multiplier ),
	mxMEMBER_FIELD( beta_ambient ),
mxEND_REFLECTION

NwAtmosphereParameters::NwAtmosphereParameters()
{
	this->preset_Earth();
}



AtmosphereOptions::AtmosphereOptions()
{
	this->SetDefaults();
}

void AtmosphereOptions::SetDefaults()
{
	use_luminance = Luminance::NONE;
	use_half_precision = false;
	use_ozone_layer = true;
	use_constant_solar_spectrum = false;
}





constexpr int TRANSMITTANCE_TEXTURE_WIDTH = 256;
constexpr int TRANSMITTANCE_TEXTURE_HEIGHT = 64;

constexpr int SCATTERING_TEXTURE_R_SIZE = 32;
constexpr int SCATTERING_TEXTURE_MU_SIZE = 128;
constexpr int SCATTERING_TEXTURE_MU_S_SIZE = 32;
constexpr int SCATTERING_TEXTURE_NU_SIZE = 8;

constexpr int SCATTERING_TEXTURE_WIDTH = SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
constexpr int SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
constexpr int SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;

constexpr int IRRADIANCE_TEXTURE_WIDTH = 64;
constexpr int IRRADIANCE_TEXTURE_HEIGHT = 16;

constexpr double kLambdaR = 680.0;
constexpr double kLambdaG = 550.0;
constexpr double kLambdaB = 440.0;

constexpr double kPi = 3.1415926;
constexpr double kSunAngularRadius = 0.00935 / 2.0;
constexpr double kSunSolidAngle = kPi * kSunAngularRadius * kSunAngularRadius;
constexpr double kLengthUnitInMeters = 1000.0;
















// The conversion factor between watts and lumens.
constexpr double MAX_LUMINOUS_EFFICACY = 683.0;

// Values from "CIE (1931) 2-deg color matching functions", see
// "http://web.archive.org/web/20081228084047/
//    http://www.cvrl.org/database/data/cmfs/ciexyz31.txt".
constexpr double CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[380] = {
	360, 0.000129900000, 0.000003917000, 0.000606100000,
	365, 0.000232100000, 0.000006965000, 0.001086000000,
	370, 0.000414900000, 0.000012390000, 0.001946000000,
	375, 0.000741600000, 0.000022020000, 0.003486000000,
	380, 0.001368000000, 0.000039000000, 0.006450001000,
	385, 0.002236000000, 0.000064000000, 0.010549990000,
	390, 0.004243000000, 0.000120000000, 0.020050010000,
	395, 0.007650000000, 0.000217000000, 0.036210000000,
	400, 0.014310000000, 0.000396000000, 0.067850010000,
	405, 0.023190000000, 0.000640000000, 0.110200000000,
	410, 0.043510000000, 0.001210000000, 0.207400000000,
	415, 0.077630000000, 0.002180000000, 0.371300000000,
	420, 0.134380000000, 0.004000000000, 0.645600000000,
	425, 0.214770000000, 0.007300000000, 1.039050100000,
	430, 0.283900000000, 0.011600000000, 1.385600000000,
	435, 0.328500000000, 0.016840000000, 1.622960000000,
	440, 0.348280000000, 0.023000000000, 1.747060000000,
	445, 0.348060000000, 0.029800000000, 1.782600000000,
	450, 0.336200000000, 0.038000000000, 1.772110000000,
	455, 0.318700000000, 0.048000000000, 1.744100000000,
	460, 0.290800000000, 0.060000000000, 1.669200000000,
	465, 0.251100000000, 0.073900000000, 1.528100000000,
	470, 0.195360000000, 0.090980000000, 1.287640000000,
	475, 0.142100000000, 0.112600000000, 1.041900000000,
	480, 0.095640000000, 0.139020000000, 0.812950100000,
	485, 0.057950010000, 0.169300000000, 0.616200000000,
	490, 0.032010000000, 0.208020000000, 0.465180000000,
	495, 0.014700000000, 0.258600000000, 0.353300000000,
	500, 0.004900000000, 0.323000000000, 0.272000000000,
	505, 0.002400000000, 0.407300000000, 0.212300000000,
	510, 0.009300000000, 0.503000000000, 0.158200000000,
	515, 0.029100000000, 0.608200000000, 0.111700000000,
	520, 0.063270000000, 0.710000000000, 0.078249990000,
	525, 0.109600000000, 0.793200000000, 0.057250010000,
	530, 0.165500000000, 0.862000000000, 0.042160000000,
	535, 0.225749900000, 0.914850100000, 0.029840000000,
	540, 0.290400000000, 0.954000000000, 0.020300000000,
	545, 0.359700000000, 0.980300000000, 0.013400000000,
	550, 0.433449900000, 0.994950100000, 0.008749999000,
	555, 0.512050100000, 1.000000000000, 0.005749999000,
	560, 0.594500000000, 0.995000000000, 0.003900000000,
	565, 0.678400000000, 0.978600000000, 0.002749999000,
	570, 0.762100000000, 0.952000000000, 0.002100000000,
	575, 0.842500000000, 0.915400000000, 0.001800000000,
	580, 0.916300000000, 0.870000000000, 0.001650001000,
	585, 0.978600000000, 0.816300000000, 0.001400000000,
	590, 1.026300000000, 0.757000000000, 0.001100000000,
	595, 1.056700000000, 0.694900000000, 0.001000000000,
	600, 1.062200000000, 0.631000000000, 0.000800000000,
	605, 1.045600000000, 0.566800000000, 0.000600000000,
	610, 1.002600000000, 0.503000000000, 0.000340000000,
	615, 0.938400000000, 0.441200000000, 0.000240000000,
	620, 0.854449900000, 0.381000000000, 0.000190000000,
	625, 0.751400000000, 0.321000000000, 0.000100000000,
	630, 0.642400000000, 0.265000000000, 0.000049999990,
	635, 0.541900000000, 0.217000000000, 0.000030000000,
	640, 0.447900000000, 0.175000000000, 0.000020000000,
	645, 0.360800000000, 0.138200000000, 0.000010000000,
	650, 0.283500000000, 0.107000000000, 0.000000000000,
	655, 0.218700000000, 0.081600000000, 0.000000000000,
	660, 0.164900000000, 0.061000000000, 0.000000000000,
	665, 0.121200000000, 0.044580000000, 0.000000000000,
	670, 0.087400000000, 0.032000000000, 0.000000000000,
	675, 0.063600000000, 0.023200000000, 0.000000000000,
	680, 0.046770000000, 0.017000000000, 0.000000000000,
	685, 0.032900000000, 0.011920000000, 0.000000000000,
	690, 0.022700000000, 0.008210000000, 0.000000000000,
	695, 0.015840000000, 0.005723000000, 0.000000000000,
	700, 0.011359160000, 0.004102000000, 0.000000000000,
	705, 0.008110916000, 0.002929000000, 0.000000000000,
	710, 0.005790346000, 0.002091000000, 0.000000000000,
	715, 0.004109457000, 0.001484000000, 0.000000000000,
	720, 0.002899327000, 0.001047000000, 0.000000000000,
	725, 0.002049190000, 0.000740000000, 0.000000000000,
	730, 0.001439971000, 0.000520000000, 0.000000000000,
	735, 0.000999949300, 0.000361100000, 0.000000000000,
	740, 0.000690078600, 0.000249200000, 0.000000000000,
	745, 0.000476021300, 0.000171900000, 0.000000000000,
	750, 0.000332301100, 0.000120000000, 0.000000000000,
	755, 0.000234826100, 0.000084800000, 0.000000000000,
	760, 0.000166150500, 0.000060000000, 0.000000000000,
	765, 0.000117413000, 0.000042400000, 0.000000000000,
	770, 0.000083075270, 0.000030000000, 0.000000000000,
	775, 0.000058706520, 0.000021200000, 0.000000000000,
	780, 0.000041509940, 0.000014990000, 0.000000000000,
	785, 0.000029353260, 0.000010600000, 0.000000000000,
	790, 0.000020673830, 0.000007465700, 0.000000000000,
	795, 0.000014559770, 0.000005257800, 0.000000000000,
	800, 0.000010253980, 0.000003702900, 0.000000000000,
	805, 0.000007221456, 0.000002607800, 0.000000000000,
	810, 0.000005085868, 0.000001836600, 0.000000000000,
	815, 0.000003581652, 0.000001293400, 0.000000000000,
	820, 0.000002522525, 0.000000910930, 0.000000000000,
	825, 0.000001776509, 0.000000641530, 0.000000000000,
	830, 0.000001251141, 0.000000451810, 0.000000000000,
};

// The conversion matrix from XYZ to linear sRGB color spaces.
// Values from https://en.wikipedia.org/wiki/SRGB.
constexpr double XYZ_TO_SRGB[9] = {
	+3.2406, -1.5372, -0.4986,
	-0.9689, +1.8758, +0.0415,
	+0.0557, -0.2040, +1.0570
};

// Values from "Reference Solar Spectral Irradiance: ASTM G-173", ETR column
// (see http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html),
// summed and averaged in each bin (e.g. the value for 360nm is the average
// of the ASTM G-173 values for all wavelengths between 360 and 370nm).
// Values in W.m^-2.
constexpr int kLambdaMin = 360;
constexpr int kLambdaMax = 830;
constexpr double kSolarIrradiance[48] = {
	1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.6887, 1.61253,
	1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.8298,
	1.8685, 1.8931, 1.85149, 1.8504, 1.8341, 1.8345, 1.8147, 1.78158, 1.7533,
	1.6965, 1.68194, 1.64654, 1.6048, 1.52143, 1.55622, 1.5113, 1.474, 1.4482,
	1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.2367, 1.2082,
	1.18737, 1.14683, 1.12362, 1.1058, 1.07124, 1.04992
};
// Values from http://www.iup.uni-bremen.de/gruppen/molspec/databases/
// referencespectra/o3spectra2011/index.html for 233K, summed and averaged in
// each bin (e.g. the value for 360nm is the average of the original values
// for all wavelengths between 360 and 370nm). Values in m^2.
constexpr double kOzoneCrossSection[48] = {
	1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
	8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
	1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
	4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
	2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
	6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
	2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
};
// From https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
constexpr double kDobsonUnit = 2.687e20;
// Maximum number density of ozone molecules, in m^-3 (computed so at to get
// 300 Dobson units of ozone - for this we divide 300 DU by the integral of
// the ozone density profile defined below, which is equal to 15km).
constexpr double kMaxOzoneNumberDensity = 300.0 * kDobsonUnit / 15000.0;
// Wavelength independent solar irradiance "spectrum" (not physically
// realistic, but was used in the original implementation).
constexpr double kConstantSolarIrradiance = 1.5;
constexpr double kBottomRadius = 6360000.0;
constexpr double kTopRadius = 6420000.0;
constexpr double kRayleigh = 1.24062e-6;
constexpr double kRayleighScaleHeight = 8000.0;
constexpr double kMieScaleHeight = 1200.0;
constexpr double kMieAngstromAlpha = 0.0;
constexpr double kMieAngstromBeta = 5.328e-3;
constexpr double kMieSingleScatteringAlbedo = 0.9;
constexpr double kMiePhaseFunctionG = 0.8;
constexpr double kGroundAlbedo = 0.1;

constexpr int num_precomputed_wavelengths = 15;
constexpr int num_lambda_slices = (kLambdaMax - kLambdaMin) / 10 + 1;

double CieColorMatchingFunctionTableValue(double wavelength, int column) {
	if(wavelength <= kLambdaMin || wavelength >= kLambdaMax) { return 0.0; };
	double u = (wavelength - kLambdaMin) / 5.0;
	int row = static_cast<int>(std::floor(u));
	assert(row >= 0 && row + 1 < 95);
	assert(CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength && CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
	u -= row;
	return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) + CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

double Interpolate(const double *a_wavelengths, const double *a_wavelength_function, double wavelength) {
	if(wavelength < a_wavelengths[0]) {
		return a_wavelength_function[0];
	}
	for(unsigned int i = 0; i < num_lambda_slices - 1; ++i) {
		if(wavelength < a_wavelengths[i + 1]) {
			double u = (wavelength - a_wavelengths[i]) / (a_wavelengths[i + 1] - a_wavelengths[i]);
			return a_wavelength_function[i] * (1.0 - u) + a_wavelength_function[i + 1] * u;
		}
	}
	return a_wavelength_function[num_lambda_slices - 1];
}

/*
<p>The utility method <code>ConvertSpectrumToLinearSrgb</code> is implemented
with a simple numerical integration of the given function, times the CIE color
matching funtions (with an integration step of 1nm), followed by a matrix
multiplication:
*/

void ConvertSpectrumToLinearSrgb(const double *a_wavelengths, const double *a_spectrum, double *p_r, double *p_g, double *p_b) {
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	const int dlambda = 1;
	for(int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
		double value = Interpolate(a_wavelengths, a_spectrum, lambda);
		x += CieColorMatchingFunctionTableValue(lambda, 1) * value;
		y += CieColorMatchingFunctionTableValue(lambda, 2) * value;
		z += CieColorMatchingFunctionTableValue(lambda, 3) * value;
	}
	*p_r = MAX_LUMINOUS_EFFICACY * (XYZ_TO_SRGB[0] * x + XYZ_TO_SRGB[1] * y + XYZ_TO_SRGB[2] * z) * dlambda;
	*p_g = MAX_LUMINOUS_EFFICACY * (XYZ_TO_SRGB[3] * x + XYZ_TO_SRGB[4] * y + XYZ_TO_SRGB[5] * z) * dlambda;
	*p_b = MAX_LUMINOUS_EFFICACY * (XYZ_TO_SRGB[6] * x + XYZ_TO_SRGB[7] * y + XYZ_TO_SRGB[8] * z) * dlambda;
}

/*
<p>We can then implement a utility function to compute the "spectral radiance to
luminance" conversion constants (see Section 14.3 in <a
href="https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
Evaluation of 8 Clear Sky Models</a> for their definitions):
*/

// The returned constants are in lumen.nm / watt.
void ComputeSpectralRadianceToLuminanceFactors(const double *a_wavelengths, const double *a_solar_irradiance, double lambda_power, double *k_r, double *k_g, double *k_b) {
	*k_r = *k_g = *k_b = 0.0;
	double solar_r = Interpolate(a_wavelengths, a_solar_irradiance, kLambdaR);
	double solar_g = Interpolate(a_wavelengths, a_solar_irradiance, kLambdaG);
	double solar_b = Interpolate(a_wavelengths, a_solar_irradiance, kLambdaB);
	int dlambda = 1;
	for(int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
		double x_bar = CieColorMatchingFunctionTableValue(lambda, 1);
		double y_bar = CieColorMatchingFunctionTableValue(lambda, 2);
		double z_bar = CieColorMatchingFunctionTableValue(lambda, 3);
		const double* xyz2srgb = XYZ_TO_SRGB;
		double r_bar = xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
		double g_bar = xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
		double b_bar = xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
		double irradiance = Interpolate(a_wavelengths, a_solar_irradiance, lambda);
		*k_r += r_bar * irradiance / solar_r * pow(lambda / kLambdaR, lambda_power);
		*k_g += g_bar * irradiance / solar_g * pow(lambda / kLambdaG, lambda_power);
		*k_b += b_bar * irradiance / solar_b * pow(lambda / kLambdaB, lambda_power);
	}
	*k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
	*k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
	*k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
}




struct NwAtmosphereModule: NwAtmosphereModuleI
{
	AtmosphereOptions atmosphere_options;
	AtmosphereParameters atmosphere_parameters;

	//
	double a_wavelengths[num_lambda_slices];
	double a_solar_irradiance_coeffs[num_lambda_slices];
	double a_rayleigh_scattering_coeffs[num_lambda_slices];
	double a_mie_scattering_coeffs[num_lambda_slices];
	double a_mie_extinction_coeffs[num_lambda_slices];
	double a_absorption_extinction_coeffs[num_lambda_slices];
	double a_ground_albedo_coeffs[num_lambda_slices];

	double sun_k_r, sun_k_g, sun_k_b;
	double sky_k_r, sky_k_g, sky_k_b;

	//
	HTexture2D transmittance_texture;
	HTexture2D irradiance_texture;
	HTexture2D delta_irradiance_texture;

	HTexture3D scattering_texture;
	HTexture3D single_mie_scattering_texture;
	HTexture3D delta_rayleigh_scattering_texture;
	HTexture3D delta_mie_scattering_texture;
	HTexture3D delta_scattering_density_texture;

	HTexture2D test_irradiance_texture;
	HTexture2D test_delta_irradiance_texture;

	NwClump &			_storage_clump;
	AllocatorI &	_allocator;

public:	// NwAtmosphereModuleI

	NwAtmosphereModule(
		AllocatorI & storage
		, NwClump & storage_clump
		)
		: _allocator( storage )
		, _storage_clump( storage_clump )
	{
	}

	virtual ERet Initialize() override
	{

		_CreateModel(
			CV3d( kLambdaR, kLambdaG, kLambdaB )
			);

		create_textures(atmosphere_options.use_half_precision);


		create_shaders();
		
		mxDO(precompute(false));
		
		//create_transmittance_shader();
		//precompute_transmittance();

		return ALL_OK;
	}

private:
	DensityProfile adjust_units(DensityProfile density)
	{
		density.layers[0].width /= kLengthUnitInMeters;
		density.layers[0].exp_scale *= kLengthUnitInMeters;
		density.layers[0].linear_term *= kLengthUnitInMeters;
		density.layers[1].width /= kLengthUnitInMeters;
		density.layers[1].exp_scale *= kLengthUnitInMeters;
		density.layers[1].linear_term *= kLengthUnitInMeters;
		return density;
	}

	void compute_white_point(double *p_white_point_r, double *p_white_point_g, double *p_white_point_b)
	{
		ConvertSpectrumToLinearSrgb(a_wavelengths, a_solar_irradiance_coeffs, p_white_point_r, p_white_point_g, p_white_point_b);
		double white_point = (*p_white_point_r + *p_white_point_g + *p_white_point_b) / 3.0;
		*p_white_point_r /= white_point;
		*p_white_point_g /= white_point;
		*p_white_point_b /= white_point;
	}

	float compute_luminance_from_radiance_coeff(double lambda, double delta_lambda, int component)
	{
		// Note that we don't include MAX_LUMINOUS_EFFICACY here, to avoid
		// artifacts due to too large values when using half precision on GPU.
		// We add this term back in kAtmosphereShader, via
		// SKY_SPECTRAL_RADIANCE_TO_LUMINANCE (see also the comments in the
		// Model constructor).
		double x = CieColorMatchingFunctionTableValue(lambda, 1);
		double y = CieColorMatchingFunctionTableValue(lambda, 2);
		double z = CieColorMatchingFunctionTableValue(lambda, 3);
		return static_cast<float>(( XYZ_TO_SRGB[component * 3] * x + XYZ_TO_SRGB[component * 3 + 1] * y + XYZ_TO_SRGB[component * 3 + 2] * z) * delta_lambda);
	}

	void _CreateModel( const V3d& lambdas )
	{
		for(int l = kLambdaMin, lambda_slice_index = 0; l <= kLambdaMax; l += 10, ++lambda_slice_index) {
			double lambda = static_cast<double>(l) * 1e-3;  // micro-meters
			double mie = kMieAngstromBeta / kMieScaleHeight * pow(lambda, -kMieAngstromAlpha);
			a_wavelengths[lambda_slice_index] = l;
			a_rayleigh_scattering_coeffs[lambda_slice_index] = kRayleigh * pow(lambda, -4);
			a_mie_scattering_coeffs[lambda_slice_index] = mie * kMieSingleScatteringAlbedo;
			a_mie_extinction_coeffs[lambda_slice_index] = mie;
			a_ground_albedo_coeffs[lambda_slice_index] = kGroundAlbedo;
			
			if(atmosphere_options.use_constant_solar_spectrum) { a_solar_irradiance_coeffs[lambda_slice_index] = kConstantSolarIrradiance;}
			else { a_solar_irradiance_coeffs[lambda_slice_index] = kSolarIrradiance[lambda_slice_index];}
			a_absorption_extinction_coeffs[lambda_slice_index] = atmosphere_options.use_ozone_layer ? kMaxOzoneNumberDensity * kOzoneCrossSection[lambda_slice_index] : 0.0;	
		}

		// Compute the values for the SKY_RADIANCE_TO_LUMINANCE constant. In theory
		// this should be 1 in precomputed illuminance mode (because the precomputed
		// textures already contain illuminance values). In practice, however, storing
		// true illuminance values in half precision textures yields artifacts
		// (because the values are too large), so we store illuminance values divided
		// by MAX_LUMINOUS_EFFICACY instead. This is why, in precomputed illuminance
		// mode, we set SKY_RADIANCE_TO_LUMINANCE to MAX_LUMINOUS_EFFICACY.
		if(atmosphere_options.use_luminance == PRECOMPUTED) {
			sky_k_r = sky_k_g = sky_k_b = MAX_LUMINOUS_EFFICACY;
		}
		else {
			ComputeSpectralRadianceToLuminanceFactors(a_wavelengths, a_solar_irradiance_coeffs, -3 /* lambda_power */, &sky_k_r, &sky_k_g, &sky_k_b);
		}
		// Compute the values for the SUN_RADIANCE_TO_LUMINANCE constant.
		ComputeSpectralRadianceToLuminanceFactors(a_wavelengths, a_solar_irradiance_coeffs, 0 /* lambda_power */, &sun_k_r, &sun_k_g, &sun_k_b);

		atmosphere_parameters.solar_irradiance.x = Interpolate(a_wavelengths, a_solar_irradiance_coeffs, lambdas.x);
		atmosphere_parameters.solar_irradiance.y = Interpolate(a_wavelengths, a_solar_irradiance_coeffs, lambdas.y);
		atmosphere_parameters.solar_irradiance.z = Interpolate(a_wavelengths, a_solar_irradiance_coeffs, lambdas.z);

		atmosphere_parameters.sun_angular_radius = kSunAngularRadius;
		atmosphere_parameters.bottom_radius = kBottomRadius / kLengthUnitInMeters;
		atmosphere_parameters.top_radius = kTopRadius / kLengthUnitInMeters;

		{
			DensityProfile rayleigh_density;
			{
				DensityProfileLayer	density_layer0;
				density_layer0.width = 0;
				density_layer0.exp_term = 0;
				density_layer0.exp_scale = 0;
				density_layer0.linear_term = 0;
				density_layer0.constant_term = 0;

				rayleigh_density.layers[0] = density_layer0;
			}
			{
				DensityProfileLayer	density_layer1;
				density_layer1.width = 0;
				density_layer1.exp_term = 1.0;
				density_layer1.exp_scale = -1.0 / kRayleighScaleHeight;
				density_layer1.linear_term = 0;
				density_layer1.constant_term = 0;

				rayleigh_density.layers[1] = density_layer1;
			}
			atmosphere_parameters.rayleigh_density = adjust_units(rayleigh_density);
			atmosphere_parameters.rayleigh_scattering.x = Interpolate(a_wavelengths, a_rayleigh_scattering_coeffs, lambdas.x) * kLengthUnitInMeters;
			atmosphere_parameters.rayleigh_scattering.y = Interpolate(a_wavelengths, a_rayleigh_scattering_coeffs, lambdas.y) * kLengthUnitInMeters;
			atmosphere_parameters.rayleigh_scattering.z = Interpolate(a_wavelengths, a_rayleigh_scattering_coeffs, lambdas.z) * kLengthUnitInMeters;
		}

		{
			DensityProfile mie_density;
			{
				DensityProfileLayer	density_layer0;
				density_layer0.width = 0;
				density_layer0.exp_term = 0;
				density_layer0.exp_scale = 0;
				density_layer0.linear_term = 0;
				density_layer0.constant_term = 0;

				mie_density.layers[0] = density_layer0;
			}
			{
				DensityProfileLayer	density_layer1;
				density_layer1.width = 0;
				density_layer1.exp_term = 1.0;
				density_layer1.exp_scale = -1.0 / kMieScaleHeight;
				density_layer1.linear_term = 0;
				density_layer1.constant_term = 0;

				mie_density.layers[1] = density_layer1;
			}
			atmosphere_parameters.mie_density = adjust_units(mie_density);
			atmosphere_parameters.mie_scattering.x = Interpolate(a_wavelengths, a_mie_scattering_coeffs, lambdas.x) * kLengthUnitInMeters;
			atmosphere_parameters.mie_scattering.y = Interpolate(a_wavelengths, a_mie_scattering_coeffs, lambdas.y) * kLengthUnitInMeters;
			atmosphere_parameters.mie_scattering.z = Interpolate(a_wavelengths, a_mie_scattering_coeffs, lambdas.z) * kLengthUnitInMeters;
			atmosphere_parameters.mie_extinction.x = Interpolate(a_wavelengths, a_mie_extinction_coeffs, lambdas.x) * kLengthUnitInMeters;
			atmosphere_parameters.mie_extinction.y = Interpolate(a_wavelengths, a_mie_extinction_coeffs, lambdas.y) * kLengthUnitInMeters;
			atmosphere_parameters.mie_extinction.z = Interpolate(a_wavelengths, a_mie_extinction_coeffs, lambdas.z) * kLengthUnitInMeters;
			atmosphere_parameters.mie_phase_function_g = kMiePhaseFunctionG;
		}

		{
			DensityProfile ozone_density;
			{
				DensityProfileLayer	density_layer0;
				density_layer0.width = 25000.0;
				density_layer0.exp_term = 0;
				density_layer0.exp_scale = 0;
				density_layer0.linear_term = 1.0 / 15000.0;
				density_layer0.constant_term = -2.0 / 3.0;

				ozone_density.layers[0] = density_layer0;
			}
			{
				DensityProfileLayer	density_layer1;
				density_layer1.width = 0;
				density_layer1.exp_term = 0;
				density_layer1.exp_scale = 0;
				density_layer1.linear_term = -1.0 / 15000.0;
				density_layer1.constant_term = 8.0 / 3.0;

				ozone_density.layers[1] = density_layer1;
			}
			atmosphere_parameters.absorption_density = adjust_units(ozone_density);
		}

		atmosphere_parameters.absorption_extinction.x = Interpolate(a_wavelengths, a_absorption_extinction_coeffs, lambdas.x) * kLengthUnitInMeters;
		atmosphere_parameters.absorption_extinction.y = Interpolate(a_wavelengths, a_absorption_extinction_coeffs, lambdas.y) * kLengthUnitInMeters;
		atmosphere_parameters.absorption_extinction.z = Interpolate(a_wavelengths, a_absorption_extinction_coeffs, lambdas.z) * kLengthUnitInMeters;

		atmosphere_parameters.ground_albedo.x = Interpolate(a_wavelengths, a_ground_albedo_coeffs, lambdas.x);
		atmosphere_parameters.ground_albedo.y = Interpolate(a_wavelengths, a_ground_albedo_coeffs, lambdas.y);
		atmosphere_parameters.ground_albedo.z = Interpolate(a_wavelengths, a_ground_albedo_coeffs, lambdas.z);

		const double max_sun_zenith_angle = (atmosphere_options.use_half_precision ? 102.0 : 120.0) / 180.0 * kPi;
		atmosphere_parameters.mu_s_min = cos(max_sun_zenith_angle);
	}

	void create_textures(bool use_half_precision)
	{
		const NGpu::Capabilities	device_caps = NGpu::getCapabilities();

		NwDataFormat::Enum	RGB_float_render_target_format = NwDataFormat::RGB32F;
		if( !device_caps.supports_RGB_float_render_target ) {
			RGB_float_render_target_format = NwDataFormat::RGBA32F;
		}

		const NwDataFormat::Enum format = use_half_precision
			? NwDataFormat::RGBA16F
			: RGB_float_render_target_format
			;

		transmittance_texture		= create_texture_2d(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, format);
		irradiance_texture			= create_texture_2d(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, format);
		delta_irradiance_texture	= create_texture_2d(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, format);

		scattering_texture					= create_texture_3d(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH, format);
		single_mie_scattering_texture		= create_texture_3d(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH, format);
		delta_rayleigh_scattering_texture	= create_texture_3d(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH, format);
		delta_mie_scattering_texture		= create_texture_3d(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH, format);
		delta_scattering_density_texture	= create_texture_3d(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH, format);
	}

	HTexture2D create_texture_2d(
		U32 width, U32 height
		, NwDataFormat::Enum format
		)
	{
		NwTexture2DDescription	desc(_NoInit);

		desc.width = width;
		desc.height = height;
		desc.format = format;
		desc.numMips = 1;
		desc.arraySize = 1;
		desc.flags = NwTextureCreationFlags::none;
UNDONE;
		return NGpu::createTexture2D(desc);
	}

	HTexture3D create_texture_3d(
		U32 width, U32 height, U32 depth
		, NwDataFormat::Enum format
		)
	{
		NwTexture3DDescription	desc(_NoInit);

		desc.width = width;
		desc.height = height;
		desc.depth = depth;
		desc.format = format;
		desc.numMips = 1;
		desc.flags = NwTextureCreationFlags::none;
UNDONE;
		return NGpu::createTexture3D(desc);
	}


	void create_shaders()
	{
		UNDONE;
		//renderer::compile_and_create_shader(com_demo_vs, L"../src/shaders/atmosphere_demo_vs.hlsl");		
		//renderer::compile_and_create_shader(com_precompute_vs, L"../src/shaders/atmosphere_precompute_vs.hlsl");
		//renderer::compile_and_create_shader(com_precompute_gs, L"../src/shaders/atmosphere_precompute_gs.hlsl");
		//create_demo_pixel_shader();
		//create_precomputation_shaders();
	}

	//void create_precomputation_shaders()
	//{UNDONE;
	//	for(UINT shader_index = 0; shader_index < num_precompute_shader_type; ++shader_index) {
	//		a_com_precompute_shaders[shader_index].Reset();
	//		a_precompute_shader_macros[num_precompute_shader_type].Definition = a_precompute_shader_macros[shader_index].Name;
	//		renderer::compile_and_create_shader(a_com_precompute_shaders[shader_index], L"../src/shaders/atmosphere_precompute_ps.hlsl", a_precompute_shader_macros);
	//	}
	//}



	ERet precompute(bool need_blend)
	{
		UNDONE;

		NGpu::NwRenderContext& render_context = NGpu::getMainRenderContext();


		NwShaderEffect* shader_fx;
		mxDO(Resources::Load(shader_fx, MakeAssetID("atmosphere_precompute"), &_storage_clump));

		//NwShaderEffect::Pass* shader_fx_passes = shader_fx->getPasses();


		//
		NGpu::NwPushCommandsOnExitingScope	autosubmit_commands(
			render_context,
			NGpu::FIRST_SORT_KEY
			nwDBG_CMD_SRCFILE_STRING
			);

		NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );


		//


		//
		{	// Precompute Transmittance

			//const NwShaderEffect::Pass* shader_fx_passes = shader_fx->findPass(
			//	mxHASH_STR("Precompute_Transmittance")
			//	);

			//cmd_writer.SetViewport(0, 0, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);


			// set pass state

			//



			//D3D11_VIEWPORT viewport = { 0.0, 0.0, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, 0.0, 1.0 };
			//com_device_context->RSSetViewports(1, &viewport);
			//com_device_context->PSSetShader(a_com_precompute_shaders[TRANSMITTANCE_SHADER_INDEX].Get(), 0, 0);
			//ID3D11RenderTargetView *a_rtvs[] = { transmittance_texture.com_rtv.Get()};
			//com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_rtvs, nullptr);
			//com_device_context->Draw(4, 0);

			//ID3D11RenderTargetView *const a_null_rtvs[count_of(a_rtvs)] = {};
			//com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_null_rtvs, nullptr);
		}


#if 0
		//
		NwPushCommandsOnExitingScope	submitBatch(
			render_context,
			GL::buildSortKey(viewId_AdaptLuminance, 0)
			);



		mxDO(technique->SetInput(nwNAMEHASH("t_initialLuminance"), GL::AsInput(initialLuminanceRT.handle)));
		mxDO(technique->SetInput(nwNAMEHASH("t_adaptedLuminance"), GL::AsInput(pSrcLumTarget->handle)));

		const NwShaderEffect::Pass& pass0 = technique->getPasses()[0];

		mxDO(push_FullScreenTriangle( render_context, *pass0.render_state, technique, pass0.defaultProgram ));

		// Downscale luminance

		// Generate the mip chain
		{
			GL::Cmd_GenerateMips	cmd_GenerateMips;
			cmd_GenerateMips.encode( GL::CMD_GENERATE_MIPS, 0, GL::AsInput(pDstLumTarget->handle).id );

			mxDO(render_context._command_buffer.put( cmd_GenerateMips ));
		}
#endif

#if 0
		const GL::PlatformData	device = GL::GetPlatformData();
		ComPtr<ID3D11DeviceContext> com_device_context = device.d3d11.context;

		com_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		com_device_context->VSSetShader(com_precompute_vs.Get(), 0, 0);
		ID3D11SamplerState *a_samplers[] = { renderer::get_sampler().Get() };
		com_device_context->PSSetSamplers(0, count_of(a_samplers), a_samplers);
		com_device_context->RSSetState(renderer::get_rasterizer_state().Get());
		com_device_context->OMSetDepthStencilState(renderer::get_depth_stencil_state().Get(), 0);
		ID3D11Buffer *a_cbs[] = { precompute_cb.com_cb.Get() };
		com_device_context->PSSetConstantBuffers(0, count_of(a_cbs), a_cbs);

		{	// Precompute Transmittance
			D3D11_VIEWPORT viewport = { 0.0, 0.0, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, 0.0, 1.0 };
			com_device_context->RSSetViewports(1, &viewport);
			com_device_context->PSSetShader(a_com_precompute_shaders[TRANSMITTANCE_SHADER_INDEX].Get(), 0, 0);
			ID3D11RenderTargetView *a_rtvs[] = { transmittance_texture.com_rtv.Get()};
			com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_rtvs, nullptr);
			com_device_context->Draw(4, 0);
			ID3D11RenderTargetView *const a_null_rtvs[count_of(a_rtvs)] = {};
			com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_null_rtvs, nullptr);
		}

		{	// Precompute Direct Irradiance
			D3D11_VIEWPORT viewport = { 0.0, 0.0, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 0.0, 1.0 };
			com_device_context->RSSetViewports(1, &viewport);
			ID3D11ShaderResourceView *a_srvs[] = { transmittance_texture.com_srv.Get() };
			com_device_context->PSSetShaderResources(0, count_of(a_srvs), a_srvs);
			com_device_context->PSSetShader(a_com_precompute_shaders[DELTA_IRRADIANCE_SHADER_INDEX].Get(), 0, 0);
			ID3D11RenderTargetView *a_rtvs[] = { delta_irradiance_texture.com_rtv.Get(), irradiance_texture.com_rtv.Get() };
			com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_rtvs, nullptr);
			if(need_blend) com_device_context->OMSetBlendState(renderer::get_blend_state_01().Get(), NULL, 0xffffffff);
			com_device_context->Draw(4, 0);
			ID3D11RenderTargetView *const a_null_rtvs[count_of(a_rtvs)] = {};
			com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_null_rtvs, nullptr);
			if(need_blend) com_device_context->OMSetBlendState(nullptr, NULL, 0xffffffff);
		}

		{	// Precompute Direct Rayleigh and Mie single scattering
			D3D11_VIEWPORT viewport = { 0.0, 0.0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, 0.0, 1.0 };
			com_device_context->RSSetViewports(1, &viewport);
			com_device_context->GSSetShader(com_precompute_gs.Get(), 0, 0);
			ID3D11ShaderResourceView *a_srvs[] = { transmittance_texture.com_srv.Get() };
			com_device_context->PSSetShaderResources(0, count_of(a_srvs), a_srvs);
			com_device_context->PSSetShader(a_com_precompute_shaders[SINGLE_SCATTERING_SHADER_INDEX].Get(), 0, 0);
			ID3D11RenderTargetView *a_rtvs[] = {
				delta_rayleigh_scattering_texture.com_rtv.Get(), delta_mie_scattering_texture.com_rtv.Get(),
				scattering_texture.com_rtv.Get(), single_mie_scattering_texture.com_rtv.Get()
			};
			com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_rtvs, nullptr);
			if(need_blend) com_device_context->OMSetBlendState(renderer::get_blend_state_0011().Get(), NULL, 0xffffffff);
			com_device_context->DrawInstanced(4, SCATTERING_TEXTURE_DEPTH, 0, 0);
			ID3D11ShaderResourceView *const a_null_srvs[count_of(a_srvs)] = {};
			com_device_context->PSSetShaderResources(0, count_of(a_srvs), a_null_srvs);
			ID3D11RenderTargetView *const a_null_rtvs[count_of(a_rtvs)] = {};
			com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_null_rtvs, nullptr);
			if(need_blend) com_device_context->OMSetBlendState(nullptr, NULL, 0xffffffff);
		}

		{	// Accumulate Multiple Scattering
			for(uint32_t scattering_order = 2; scattering_order <= 4; ++scattering_order)
			{
				{	// Scattering Density
					D3D11_VIEWPORT viewport = { 0.0, 0.0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, 0.0, 1.0 };
					com_device_context->RSSetViewports(1, &viewport);
					ID3D11ShaderResourceView *a_srvs[] = {
						transmittance_texture.com_srv.Get(), delta_rayleigh_scattering_texture.com_srv.Get(), delta_mie_scattering_texture.com_srv.Get(),
						delta_rayleigh_scattering_texture.com_srv.Get(), delta_irradiance_texture.com_srv.Get()
					};
					com_device_context->PSSetShaderResources(0, count_of(a_srvs), a_srvs);
					com_device_context->PSSetShader(a_com_precompute_shaders[SCATTERING_DENSITY_SHADER_INDEX].Get(), 0, 0);
					ID3D11RenderTargetView *a_rtvs[] = { delta_scattering_density_texture.com_rtv.Get() };
					com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_rtvs, nullptr);

					precompute_cb.data.scattering_order = scattering_order;
					renderer::update_cb(precompute_cb.com_cb, &precompute_cb.data, sizeof(precompute_cb.data));
					com_device_context->DrawInstanced(4, SCATTERING_TEXTURE_DEPTH, 0, 0);

					ID3D11ShaderResourceView *const a_null_srvs[count_of(a_srvs)] = {};
					com_device_context->PSSetShaderResources(0, count_of(a_srvs), a_null_srvs);
					ID3D11RenderTargetView *const a_null_rtvs[count_of(a_rtvs)] = {};
					com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_null_rtvs, nullptr);
				}

				{	// Indirect Irradiance
					precompute_cb.data.scattering_order = scattering_order - 1;
					renderer::update_cb(precompute_cb.com_cb, &precompute_cb.data, sizeof(precompute_cb.data));

					D3D11_VIEWPORT viewport = { 0.0, 0.0, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 0.0, 1.0 };
					com_device_context->RSSetViewports(1, &viewport);
					ID3D11ShaderResourceView *a_srvs[] = {
						delta_rayleigh_scattering_texture.com_srv.Get(), delta_mie_scattering_texture.com_srv.Get(),
						delta_rayleigh_scattering_texture.com_srv.Get() };
					com_device_context->PSSetShaderResources(0, count_of(a_srvs), a_srvs);
					com_device_context->PSSetShader(a_com_precompute_shaders[INDIRECT_IRRADIANCE_SHADER_INDEX].Get(), 0, 0);
					ID3D11RenderTargetView *a_rtvs[] = { delta_irradiance_texture.com_rtv.Get(), irradiance_texture.com_rtv.Get() };
					com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_rtvs, nullptr);
					com_device_context->OMSetBlendState(renderer::get_blend_state_01().Get(), NULL, 0xffffffff);
					com_device_context->Draw(4, 0);

					ID3D11ShaderResourceView *const a_null_srvs[count_of(a_srvs)] = {};
					com_device_context->PSSetShaderResources(0, count_of(a_srvs), a_null_srvs);
					ID3D11RenderTargetView *const a_null_rtvs[count_of(a_rtvs)] = {};
					com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_null_rtvs, nullptr);
					com_device_context->OMSetBlendState(nullptr, NULL, 0xffffffff);
				}

				{	// Multiple Scattering
					D3D11_VIEWPORT viewport = { 0.0, 0.0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, 0.0, 1.0 };
					com_device_context->RSSetViewports(1, &viewport);
					ID3D11ShaderResourceView *a_srvs[] = { transmittance_texture.com_srv.Get(), delta_scattering_density_texture.com_srv.Get() };
					com_device_context->PSSetShaderResources(0, count_of(a_srvs), a_srvs);
					com_device_context->PSSetShader(a_com_precompute_shaders[MULTIPLE_SCATTERING_SHADER_INDEX].Get(), 0, 0);
					ID3D11RenderTargetView *a_rtvs[] = { delta_rayleigh_scattering_texture.com_rtv.Get()/*delta_multiple_scattering_texture.com_rtv.Get()*/, scattering_texture.com_rtv.Get() };
					com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_rtvs, nullptr);
					com_device_context->OMSetBlendState(renderer::get_blend_state_01().Get(), NULL, 0xffffffff);
					com_device_context->DrawInstanced(4, SCATTERING_TEXTURE_DEPTH, 0, 0);

					ID3D11ShaderResourceView *const a_null_srvs[count_of(a_srvs)] = {};
					com_device_context->PSSetShaderResources(0, count_of(a_srvs), a_null_srvs);
					ID3D11RenderTargetView *const a_null_rtvs[count_of(a_rtvs)] = {};
					com_device_context->OMSetRenderTargets(count_of(a_rtvs), a_null_rtvs, nullptr);
					com_device_context->OMSetBlendState(nullptr, NULL, 0xffffffff);
				}
			}//for
		}

		com_device_context->ClearState();
		com_device_context->Flush();
#endif
		return ALL_OK;
	}

};


NwAtmosphereModuleI* NwAtmosphereModuleI::create(
	AllocatorI & storage
	, NwClump & storage_clump
	)
{
	return mxNEW(
		storage
		, NwAtmosphereModule
		, storage, storage_clump
		);
}

void NwAtmosphereModuleI::destroy( NwAtmosphereModuleI * o )
{
	NwAtmosphereModule* po = static_cast< NwAtmosphereModule* >( o );
	mxDELETE( po, po->_allocator );
}

}//namespace Atmosphere_Rendering
