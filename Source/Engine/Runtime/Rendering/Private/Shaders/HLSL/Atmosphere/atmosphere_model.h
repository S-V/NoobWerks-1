static const int TRANSMITTANCE_TEXTURE_WIDTH = 256;
static const int TRANSMITTANCE_TEXTURE_HEIGHT = 64;
static const int SCATTERING_TEXTURE_R_SIZE = 32;
static const int SCATTERING_TEXTURE_MU_SIZE = 128;
static const int SCATTERING_TEXTURE_MU_S_SIZE = 32;
static const int SCATTERING_TEXTURE_NU_SIZE = 8;
static const int IRRADIANCE_TEXTURE_WIDTH = 64;
static const int IRRADIANCE_TEXTURE_HEIGHT = 16;
static const float3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = float3(114974.916437, 71305.954816, 65310.548555);
static const float3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = float3(98242.786222, 69954.398112, 66475.012354);
static const float kLengthUnitInMeters = 1000.000000;

static const AtmosphereParameters atmosphere = {
	float3(1.474000, 1.850400, 1.911980),
	0.004675,
	6360.000000,
	6420.000000,
	{ { 0.000000, 0.000000, 0.000000, 0.000000, 0.000000 }, { 0.000000, 1.000000, -0.125000, 0.000000, 0.000000 } },
	float3(0.005802, 0.013558, 0.033100),
	{ { 0.000000, 0.000000, 0.000000, 0.000000, 0.000000 }, { 0.000000, 1.000000, -0.833333, 0.000000, 0.000000 } },
	float3(0.003996, 0.003996, 0.003996),
	float3(0.004440, 0.004440, 0.004440),
	0.800000,
	{ { 25.000000, 0.000000, 0.000000, 0.066667, -0.666667 }, { 0.000000, 0.000000, 0.000000, -0.066667, 2.666667 } },
	float3(0.000650, 0.001881, 0.000085),
	float3(0.100000, 0.100000, 0.100000),
	-0.500000
};
