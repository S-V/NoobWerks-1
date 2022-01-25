#ifndef NW_GBUFFER_READ_H
#define NW_GBUFFER_READ_H


//-----------------------------------------------------------------------------
//	G-buffer textures
//-----------------------------------------------------------------------------

/// Hardware depth texture for restoring view-space positions.
Texture2D< float >		DepthTexture: register(t0);	// contains normalized, post-projection Z/W

Texture2D< float4 >		GBufferTexture0: register(t1);
Texture2D< float4 >		GBufferTexture1: register(t2);
//Texture2D< float4 >		GBufferTexture2: register(t3);
//Texture2D< float4 >		GBufferTexture3;

/// this structure is sampled from the geometry buffer
struct GBufferPixel
{
	float4	RT0;
	float4	RT1;
	//float4	RT2;
};

/*
Multiplication by our projection matrix (NOTE: z corresponds to height, y - to depth):

            | H   0   0   0 |
[x y z w] * | 0   0   A   1 | = | (x*H)  (z*V)  (y*A + w*B) (y) |
            | 0   V   0   0 |
            | 0   0   B   0 |
*/
/*
Multiplication by inverse projection matrix (P^-1):

            | 1/H   0   0    0  |
[x y z w] * |  0    0  1/V   0  | = | (x/H)   (w)   (y/V)  (z/B - w*A/B) |
            |  0    0   0   1/B |
            |  0    1   0  -A/B |
*/

/** Returns the view-space depth (the distance along the camera forward axis). */
inline float hardwareZToViewSpaceDepth(
								  in float hardware_z_depth
								  )
{
#if 0
	// remember, the matrices are transposed before passing to shaders
	float A = g_projection_matrix[2][1];
	float B = g_projection_matrix[2][3];
	return B / (hardware_z_depth - A);
#else
	// transform the above into a multiply-add and a reciprocal:
	float P = g_inverse_projection_matrix[3][2];	// P = 1/B
	float Q = g_inverse_projection_matrix[3][3];	// Q = -A/B
	return 1.0f / (P * hardware_z_depth + Q);
#endif
}

/// screen_space_position - clip-space/NDC position
/// hardware_z_depth - hardware depth buffer sample (post-projection Z/W)
inline float3 restoreViewSpacePosition(
									   in float2 screen_space_position
									   , in float hardware_z_depth
									   )
{
	float invH = g_inverse_projection_matrix[0][0];	// 1 / H
	float invV = g_inverse_projection_matrix[2][1];	// 1 / V
	float viewPosition_x = screen_space_position.x * invH;
	float viewPosition_y = screen_space_position.y * invV;
	// recover view-space depth
	float3 view_space_position = float3( viewPosition_x, 1, viewPosition_y ) * hardwareZToViewSpaceDepth( hardware_z_depth );
	return view_space_position;
}

/// Data that can be read from the G-buffer.
struct GSurfaceParameters
{
	float3	normal;		// view-space normal (normalized to [-1..+1])

	// aka diffuse color / albedo
	float3	albedo;	// [0..1]

	float	roughness;		// 0=smooth, 1=rough
	float	metalness;		// 0=non-metal, 1=metal




	// NOT USED:


	//
	float	accessibility;	// = ( 1 - ambient_occlusion ), 1 == unoccluded (default=1)

	// [0..1], 1 = unshadowed, 0 - shadowed
	float	sun_visibility;


	//
	float3 specular_color;	// [0..1]
	float  specular_power;	// aka Gloss - Roughness/Sharpness/Specular exponent, [0..1]


	//float3 emissiveColor;	// [0..1]
	//float 
	//float Alpha;	// Opacity	// [0..1]
};

GSurfaceParameters SurfaceData_from_GBufferPixel(
	in GBufferPixel data
	, in bool normalize_sampled_normal = true
	)
{
	GSurfaceParameters surface;

	surface.normal = data.RT0.xyz * 2.0f - 1.0f;
	if( normalize_sampled_normal ) {
		surface.normal = normalize( surface.normal );
	}
	surface.roughness = data.RT0.a;

	//
	surface.albedo = data.RT1.rgb;
	surface.metalness = data.RT1.a;

	//
	surface.specular_color = float3(1,1,1);//data.RT2.rgb;
	surface.specular_power = 1;//data.RT2.a;

	//
	//surface.accessibility = data.RT2.r;
	//surface.sun_visibility = data.RT2.g;
	surface.accessibility = 1;
	surface.sun_visibility = 1;

	return surface;
}

GBufferPixel GBuffer_LoadPixelData(in uint2 pixel_location)
{
	GBufferPixel gbuffer_pixel;
	gbuffer_pixel.RT0 = GBufferTexture0.Load( int3( pixel_location, 0/*sampleIndex*/ ) );
	gbuffer_pixel.RT1 = GBufferTexture1.Load( int3( pixel_location, 0/*sampleIndex*/ ) );
	//gbuffer_pixel.RT2 = GBufferTexture2.Load( int3( pixel_location, 0/*sampleIndex*/ ) );
	return gbuffer_pixel;
}

/// pixel_location - texel coordinates in the [0, textureWidth - 1] x [0, textureHeight - 1] range
GSurfaceParameters Load_Surface_Data_From_GBuffer(
	in uint2 pixel_location
	, in bool normalize_sampled_normal = true
	)
{
	const GBufferPixel gbuffer = GBuffer_LoadPixelData(pixel_location);
	return SurfaceData_from_GBufferPixel( gbuffer, normalize_sampled_normal );
}

GSurfaceParameters Sample_Surface_Data_From_GBuffer(
	in SamplerState _sampler
	, in float2 _texCoords
	, bool normalize_sampled_normal = true
	)
{
	GBufferPixel gbuffer;
	gbuffer.RT0 = GBufferTexture0.SampleLevel( _sampler, _texCoords, 0/*mipLevel*/ );
	gbuffer.RT1 = GBufferTexture1.SampleLevel( _sampler, _texCoords, 0/*mipLevel*/ );
	//gbuffer.RT2 = GBufferTexture2.SampleLevel( _sampler, _texCoords, 0/*mipLevel*/ );
	return SurfaceData_from_GBufferPixel( gbuffer, normalize_sampled_normal );
}

#endif // NW_GBUFFER_READ_H
