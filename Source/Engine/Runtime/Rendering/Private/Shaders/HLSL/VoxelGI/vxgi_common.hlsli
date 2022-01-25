// 
#ifndef __VOXEL_GI_H__
#define __VOXEL_GI_H__

struct VXGI_Voxel
{
	// RGB - albedo, A - opacity
	uint	albedo_opacity;
};

/// the scaling coefficient for packing HDR values into R8G8B8 textures
static const float VXGI_HDR_MULTIPLIER = 1;

/**
 Unpacks the given @c uint (R8G8B8A8) to a @c float4.

 @param		u
			The @c uint (R8G8B8A8) to unpack.
 @return	The corresponding unpacked @c float4.
 */
float4 unpackR8G8B8A8( uint u )
{
	const float4 f = 0xFFu & uint4(u >> 24u, u >> 16u, u >> 8u, u);
	return f * (1.0f / 255.0f);
}

/**
 Unpacks the given @c float4 to a @c uint (R8G8B8A8).

 @pre		@a f lies in [0.0,1.0]^4
 @param		u
			The @c float4 to pack.
 @return	The corresponding packed @c uint (R8G8B8A8).
 */
uint packR8G8B8A8( float4 f )
{
	const uint4 u = 255.0f * f;
	return (u.x << 24u) | (u.y << 16u) | (u.z << 8u) | u.w;
}

uint encodeAlbedoOpacity( float4 x )
{
	float3 scaledRGB = x.rgb * (1.0f / VXGI_HDR_MULTIPLIER);
	return packR8G8B8A8( float4( scaledRGB, x.a ) );
}

float4 decodeAlbedoOpacity( uint encoded )
{
	float4 scaledColor = unpackR8G8B8A8( encoded );
	return float4( scaledColor.rgb * VXGI_HDR_MULTIPLIER, scaledColor.a );
}

/*
uint EncodeRadiance(float3 L) {
	return PackR8G8B8A8(RGBtoLogLuv(L));
}

float3 DecodeRadiance(uint encoded_L) {
	return LogLuvToRGB(UnpackR8G8B8A8(encoded_L));
}

uint EncodeNormal(float3 n) {
	return PackR16G16(NORMAL_ENCODE_FUNCTION(n));
}

float3 DecodeNormal(uint encoded_n) {
	return NORMAL_DECODE_FUNCTION(UnpackR16G16(encoded_n));
}
*/

#endif // __VOXEL_GI_H__
