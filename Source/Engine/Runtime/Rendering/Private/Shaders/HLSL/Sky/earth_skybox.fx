/*

%%
pass Sky
{
	vertex = SkyFullscreenTriangleVS
	pixel = SkyFullscreenTrianglePS

	state = skybox
}

// feature SKY_RENDERING_MODE
// {
	// bits = 1	// 0 = analytic model, 1 - skybox cube map
	// default = 0	// analytic model
// }

%%
*/

#include "base.hlsli"
#include <Common/transform.hlsli>
// 
#include <Shared/nw_shared_globals.h>
#include "base.hlsli"
#include <Sky/clearsky.hlsl>
#include <screen_shader.hlsl>
#include <Sky/atmosphere_analytic.hlsl>



struct VS_Output
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
	float3 rayFarWS : TexCoord1;	// the ray's end point on the far image plane
};

//=====================================================================

// The sky is rendered as a fullscreen triangle. The pixel shader traces rays through each pixel.
VS_Output SkyFullscreenTriangleVS( in uint vertexID : SV_VertexID )	// vertexID:=[0,1,2]
{
    VS_Output output;
	GetFullScreenTrianglePosTexCoord(
		vertexID
		, output.position
		, output.texCoord
		);
	//
	const float farPlaneDist = 1;// g_DepthClipPlanes.y;	// distance from eye position to the image plane
	const float aspect_ratio = g_viewport_aspectRatio();	// screen width / screen height
	const float tanHalfFoVy = g_viewport_tanHalfFoVy();
	
	const float imagePlaneH = farPlaneDist * tanHalfFoVy;	// half-height of the image plane
	const float imagePlaneW = imagePlaneH * aspect_ratio;	// half-width of the image plane

	const float3 f3FarCenter = g_camera_fwd_WS.xyz * farPlaneDist;
	const float3 f3FarRight = g_camera_right_WS.xyz * imagePlaneW;	// scaled 'right' vector on the far plane, in world space
	const float3 f3FarUp = g_camera_up_WS.xyz * imagePlaneH;	// scaled 'up' vector on the far plane, in world space

	// Compute the positions of the triangle's corners on the far image plane, in world space.
	float3 fullscreen_triangle_corners_WS[3];
	// lower-left ( UV = ( 0, 2 ) )
	fullscreen_triangle_corners_WS[0] = g_camera_origin_WS.xyz + f3FarCenter - (f3FarRight * 1) - (f3FarUp * 3);
	// upper left ( UV = ( 0, 0 ) )
	fullscreen_triangle_corners_WS[1] = g_camera_origin_WS.xyz + f3FarCenter - (f3FarRight * 1) + (f3FarUp * 1);
	// upper-right ( UV = ( 2, 0 ) )
	fullscreen_triangle_corners_WS[2] = g_camera_origin_WS.xyz + f3FarCenter + (f3FarRight * 3) + (f3FarUp * 1);

	// the positions on the far image plane will be linearly interpolated and passed to the pixel shader
	output.rayFarWS = fullscreen_triangle_corners_WS[ vertexID ];

    return output;
}

//=====================================================================







struct AtmosphereParameters2
{
	float3	f3SunDirection;     //!< the direction to the sun (normalized)
	float	fSunIntensity;		//!< intensity of the sun, default = 22

	float	fInnerRadius;		//!< radius of the planet in meters, default = 6371e3
	float	fOuterRadius;		//!< radius of the atmosphere in meters, default = 6471e3
	float3	f3BetaRayleigh;		//!< Rayleigh scattering coefficient at sea level ("Beta R") in m^-1, default = (3.8e-6f, 13.5e-6f, 33.1e-6f)//( 5.5e-6, 13.0e-6, 22.4e-6 )
	float	fRayleighHeight;	//!< Rayleigh scale height (atmosphere thickness if density were uniform) in meters, default = 7994
	float3	f3BetaMie;			//!< Mie scattering coefficient at sea level ("Beta M") in m^-1, default = 21e-6
	float	fMieHeight;			//!< Mie scale height (atmosphere thickness if density were uniform) in meters, default = 1.2e3
	/// The Mie phase function includes a term g (the Rayleigh phase function doesn't)
	/// which controls the anisotropy of the medium. Aerosols exhibit a strong forward directivity.
	float	fMie;	//!< Mie preferred scattering direction (aka "mean cosine"), default = 0.758
		
	void SetupForEarth()
	{
		fInnerRadius = 6371e3f;
		fOuterRadius= 6471e3f;

		f3BetaRayleigh = float3
			//( 3.8e-6f, 13.5e-6f, 33.1e-6f )
			( 5.5e-6, 13.0e-6, 22.4e-6 )
			;

		fRayleighHeight = 7994;
		f3BetaMie = 21e-6;
		fMieHeight = 1.2e3;
		fMie = 0.758;
	}
};

float3 SkyFullscreenTrianglePS( in VS_Output pixelIn ) : SV_Target
{
	float3 f3RayOrigin = g_camera_origin_WS.xyz;
	float3 f3RayEnd = pixelIn.rayFarWS;
	float3 f3RayDirection = normalize( f3RayEnd - f3RayOrigin );
	
#if 0 // atmospheric scattering
	AtmosphereParameters2	u_atmosphere;
	u_atmosphere.SetupForEarth();

	u_atmosphere.f3SunDirection = g_directional_light.light_vector_WS.xyz;
	u_atmosphere.fSunIntensity = 22;
	//
/*
	float3 f3SphereCenter = 0;
	float fSphereRadius = 1;
	float2 f2Intersections;
	if( GetRaySphereIntersection(
		f3RayOrigin,
		f3RayDirection,
		f3SphereCenter,
		fSphereRadius,
		f2Intersections ) )
	{
		float minDist = min( f2Intersections.x, f2Intersections.y );	// get the time of first hit
		if( minDist > 0 ) {
			// the ray intersects the sphere behind the camera
			float3 f3HitPos = f3RayOrigin + f3RayDirection * minDist;
			float3 f3HitNormal = normalize( f3HitPos - f3SphereCenter );
			float3 f3LightVector = u_atmosphere.f3SunDirection;
			return dot(f3HitNormal, f3LightVector) + 1e-3f;
		}
	}
*/

#if 0
    float3 color = ComputeIncidentLight(
		u_atmosphere,

		//float3(0, 0, 6372e3),	//+ view from ground
		//float3(0, 0, 6472e3),	//+ view from space
		//float3(0, 0, 6500e3),	//+ view from space
		float3(0, 0, 6382e3)	//+ view from ground
	
	/*
		// lift the camera above the ground
		g_camera_origin_WS.xyz + float3(0,0,
			//u_atmosphere.fOuterRadius*1.1	// from space
			u_atmosphere.fInnerRadius + 1000
		)// float3(0, 0, 6373e3),
	*/
        , f3RayDirection
	);
#endif

	AtmosphereParameters	atmo;
	atmo.preset_Earth();

	AtmosphereRenderContext atmo_ctx;
	atmo_ctx.setDefaults();
	
	atmo_ctx.camera_position = float3(
		0, 0,
		6382e3//+100
		);
	atmo_ctx.eye_to_pixel_ray_direction = f3RayDirection;
	atmo_ctx.light_vector = g_directional_light.light_vector_WS.xyz;

	
	float opacity;

	float3 color = computeScatteredColor(
		atmo
		, atmo_ctx
		, opacity
	);
		

	// Draw the Sun.
	#if 1
		const float3 SunColor = float3(1.0f, 0.8f, 0.3f) * 1500;
		const float SunWidth = 0.012f;
		const float angle = acos( dot( f3RayDirection, u_atmosphere.f3SunDirection ) );
		color = lerp( SunColor, color, saturate(abs(angle) / SunWidth) );
	#endif
	
	return color;
	
#else

	return CIEClearSky(
		f3RayDirection,
		g_directional_light.light_vector_WS.xyz
	) * 1e-1;

#endif
	// return AtmosphericScattering(
		// g_camera_fwd_WS.xyz//float3(0,-1,0)
		// ,
		// g_directional_light.light_vector_WS.xyz
		// );
}
