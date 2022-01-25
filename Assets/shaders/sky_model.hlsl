// Analytic sky models.
#include "nw_shared_globals.h"
#include "_PS.h"

cbuffer Uniforms {
	SkyParameters	u_data;
};

struct Sky_VS_Out
{
    float4 position : SV_Position;
	float3 direction : Position0;
};

Sky_VS_Out VS_SkySphere( in float3 localPosition : Position )
{
    Sky_VS_Out output;
    output.position = mul( u_data.worldViewProjection, float4(localPosition, 1.0f) );
	output.direction = normalize( localPosition );
    return output;
}

float3 perez(float cos_theta, float gamma, float cos_gamma, float3 A, float3 B, float3 C, float3 D, float3 E)
{
    return (1 + A * exp(B / (cos_theta + 0.01))) * (1 + C * exp(D * gamma) + E * cos_gamma * cos_gamma);
}

float3 PS_Sky_Preetham( in Sky_VS_Out input ) : SV_Target
{
	float3 V = normalize(input.direction);

	float cos_theta = clamp(V.z, 0, 1);
	float cos_gamma = dot(V, u_data.sunDirection.xyz);
	float gamma_ = acos(cos_gamma);

	float3 R_xyY = u_data.Z.xyz * perez(
		cos_theta, gamma_, cos_gamma
		, u_data.A.xyz
		, u_data.B.xyz
		, u_data.C.xyz
		, u_data.D.xyz
		, u_data.E.xyz
	);

	float3 R_XYZ = float3(R_xyY.x, R_xyY.y, 1 - R_xyY.x - R_xyY.y) * R_xyY.z / R_xyY.y;

	float R_r = dot(float3( 3.240479, -1.537150, -0.498535), R_XYZ);
	float R_g = dot(float3(-0.969256,  1.875992,  0.041556), R_XYZ);
	float R_b = dot(float3( 0.055648, -0.204043,  1.057311), R_XYZ);

	float3 R = float3(R_r, R_g, R_b);
R = LinearToGamma(R);
	const float3 color = R;// + u_data.sunDirection.xyz * 1e-7f;
	return color * 10;
	return clamp(R, 0, 1);
}
