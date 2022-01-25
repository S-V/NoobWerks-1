// Pixel shader utilities.

#ifndef __PIXEL_SHADERS_COMMON_H__
#define __PIXEL_SHADERS_COMMON_H__

#include "base.hlsli"

//-------------------------------------------------------------------
//	Normal vectors encoding/decoding
//-------------------------------------------------------------------

// Compresses a normalized vector.
// [-1..1] -> [0..1]
inline float3 CompressNormal( in float3 N )
{
	return N * 0.5f + 0.5f;
}

// Expands a range-compressed vector (used for sampling normal maps).
// [0..1] -> [-1..1]
inline float3 ExpandNormal( in float3 v )
{
	return v * 2.0f - 1.0f;
}


/**
 * Encodes normal into spherical coordinates.
 * Can be used for normal vectors in any space (not only view space).
 * Quite ALU-heavy.
 */
inline float2 ToSpherical( float3 N )
{
	return float2( atan2(N.y, N.x) * M_INV_PI, N.z ) * 0.5f + 0.5f;
}

/**
 * Decodes normal from spherical coordinates.
 * Quite ALU-heavy.
 */
inline float3 FromSpherical( float2 spherical )
{
	spherical = spherical * 2.0f - 1.0f;

	float2 theta;
	sincos( spherical.x * M_PI, theta.y, theta.x );

	float len = sqrt( 1.0f - spherical.y * spherical.y );

	return float3( theta.xy * len, spherical.y );
}

/*
// Converts a normalized cartesian direction vector
// to spherical coordinates.
float2 CartesianToSpherical(float3 cartesian)
{
	float2 spherical;

	spherical.x = atan2(cartesian.y, cartesian.x) * M_INV_PI;
	spherical.y = cartesian.z;

	return spherical * 0.5f + 0.5f;
}

// Converts a spherical coordinate to a normalized
// cartesian direction vector.
float3 SphericalToCartesian(float2 spherical)
{
	float2 sinCosTheta, sinCosPhi;

	spherical = spherical * 2.0f - 1.0f;
	sincos(spherical.x * M_PI, sinCosTheta.x, sinCosTheta.y);
	sinCosPhi = float2(sqrt(1.0 - spherical.y * spherical.y), spherical.y);

	return float3(sinCosTheta.y * sinCosPhi.x, sinCosTheta.x * sinCosPhi.x, sinCosPhi.y);    
}
*/


/**
 * Constants for Stereographic Projection method.
 */
#define ST_PROJ_SCALE		1.7777f
#define ST_PROJ_INV_SCALE	0.562525f
#define ST_PROJ_TWO_SCALE	3.5554f

inline float2 EncodeNormal_SP( float3 N )
{
	float2 Nenc;	// encoded normal
	Nenc = N.xy / (N.z + 1.0f);
	Nenc *= ST_PROJ_INV_SCALE;
	Nenc = Nenc * 0.5f + 0.5f;
	return Nenc;
}
inline float3 DecodeNormal_SP( float2 Nenc )
{
	float3 N;
	float3 nn;
	nn.xy = Nenc * ST_PROJ_TWO_SCALE - ST_PROJ_SCALE;
	nn.z = 1.0f;
	float g = 2.0f / dot(nn.xyz,nn.xyz);
	N.xy = g * nn.xy;
	N.z = g - 1.0f;
	return N;
}



inline float2 PackNormal( float3 N )
{
	float2 Nenc;	// encoded normal
/*
	These have been successfully used to pack *view-space* normal (into R16_G16_FLOAT):
*/
/* * /
	// From S.T.A.L.K.E.R : clear Sky:
	Nenc = N.xy * 0.5f + 0.5f;
	Nenc.x *= (N.z < 0.0f) ? -1.0f : 1.0f;
/* */
/* * /
	// From CryEngine 3:	(problems with lookDirWS(0,-1,0))
	Nenc = normalize(N.xy) * sqrt( (-N.z * 0.5f) + 0.5f );
	Nenc = Nenc * 0.5f + 0.5f;
/* */
/* * /
	// Spheremap Transform:
	// (works great, but had issues with env. mapping)
	float f = N.z*2+1;
	float g = dot(N,N);
	float p = sqrt(g+f);
	Nenc = N.xy/p * 0.5 + 0.5;
/* */
/* * /
	// Lambert Azimuthal Equal-Area Projection:
	// (works great, but had issues with env. mapping and world-space vectors (1;0;0))
	float f = sqrt(8*N.z+8);
	Nenc = N.xy / f + 0.5;
/* */
/*
*/	// Stereographic Projection method:
	// (works great for me)
/* * /
	Nenc = EncodeNormal_SP(N);
/* */
/* */
	// Spherical Coordinates:
	Nenc = ToSpherical( N );//works fine with R11G11B10_FLOAT
/* */
	return Nenc;
}

inline float3 UnpackNormal( in float2 Nenc )
{
	float3 N;
/*
	These have been successfully used to unpack *view-space* normal (from R16_G16_FLOAT):
*/
/* * /
	// From S.T.A.L.K.E.R : clear Sky:
	N.xy = 2.0f * abs(Nenc.xy) - 1.0f;
	N.z = ((Nenc.x < 0.0f) ? -1.0f : 1.0f)
		* sqrt(abs( 1.0f - dot(N.xy, N.xy) ));
/* */
/* * /
	// From CryEngine 3:	(problems with lookDirWS(0,-1,0))
	float2 fenc = Nenc.xy * 2.0f - 1.0f;
	N.z = -( dot(fenc,fenc) * 2.0f - 1.0f );
	N.xy = normalize(fenc) * sqrt(1.0f - N.z * N.z );
/* */
/* * /
	// Spheremap Transform:
	// (works great, but had issues with env. mapping)
	float2 tmp = -Nenc*Nenc+Nenc;
	float f = tmp.x+tmp.y;
	float m = sqrt(4*f-1);
	N.xy = (Nenc*4-2) * m;
	N.z = 8*f-3;
/* */
/* * /
	// Lambert Azimuthal Equal-Area Projection:
	// (works great, but had issues with env. mapping)
	float2 fenc = Nenc*4-2;
	float f = dot(fenc,fenc);
	float g = sqrt(1-f/4);
	N.xy = fenc*g;
	N.z = 1-f/2;
/* */
/*
*/	// Stereographic Projection method:
	// (works great for me)(but had small issues with lookDirWS(0,-1,0))
/* * /
	N = DecodeNormal_SP( Nenc );
/* */
/* */
	// Spherical Coordinates:
	N = FromSpherical( Nenc );//works fine with R11G11B10_FLOAT
/* */
	return N;
}

inline float3 GetBumpedNormal(
	in float3 sampledNormal,
	in float3 T,
	in float3 B,
	in float3 N
	)
{
	// sample tangent-space normal
	sampledNormal = ExpandNormal( sampledNormal );

	// transform normal from tangent space into TBN space
	float3 bumpedNormal = T * sampledNormal.x
						+ B * sampledNormal.y
						+ N * sampledNormal.z;
	return normalize(bumpedNormal);
}

//-------------------------------------------------------------------
//	GAMMA CORRECTION
//-------------------------------------------------------------------

float3 GammaToLinear( float3 rgb )
{
	return pow( abs(rgb), 2.2f );
}

// Apply a gamma correction (that is, raise to power 1/gamma) to the final pixel values
// as the very last step before displaying them (if sRGB frame-buffers are not available).
float3 LinearToGamma( float3 _rgb )
{
	const float gamma = 2.2f;
	const float inverse_gamma = 1.0f / gamma;
	return pow( abs(_rgb), inverse_gamma );
}

// A cheaper version, assuming gamma of 2.0 rather than 2.2
float3 LinearToGamma_Cheap( float3 _rgb )
{
	return sqrt( _rgb );
}

/*
float3 ToLinearSpace( float3 color )
{
	return pow(color, 2.2f);
}
float3 ToGammaSpace( float3 color )
{
	return pow(color, 0.45f);
}
*/


// 1.0=full saturation, 0.0=grayscale
float3 SaturateColor( in float3 _rgb, in float fSaturation )
{
	static const float3 LUMINANCE_VECTOR = float3(0.299f, 0.587f, 0.114f);
	float fFinalLum = dot(_rgb, LUMINANCE_VECTOR);
	return lerp((float3)fFinalLum, _rgb, fSaturation);
}

// 2.0 = contrast enhanced, 1.0=normal contrast, 0.01= max contrast reduced
float3 EnhanceContrast( in float3 _rgb, in float fInvContrast )
{
	return (_rgb - 0.5f) * fInvContrast + 0.5f;
}


#endif //__PIXEL_SHADERS_COMMON_H__
