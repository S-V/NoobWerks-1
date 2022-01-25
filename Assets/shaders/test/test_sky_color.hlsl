// Renders sky colors with Rayleigh and Mie scattering.
// Based on "glsl-atmosphere": http://wwwtyro.github.io/glsl-atmosphere/
/* References:
* [detailed tutorial by Alan Zucconi] https://www.alanzucconi.com/2017/10/10/atmospheric-scattering-1/
* [scratchapixel](http://www.scratchapixel.com/lessons/goodies/simulating-sky)
* [nvidia](http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html)
* [patapom](http://patapom.com/topics/Revision2013/Revision%202013%20-%20Real-time%20Volumetric%20Rendering%20Course%20Notes.pdf)
*/

#include "_screen_shader.h"
#include "_PS.h"

#ifdef __cplusplus
#	define CPP_PADDING(TYPE)	TYPE pad##__LINE__;
#else
#	define CPP_PADDING(TYPE)
#endif

struct AtmosphereParameters {
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
};

cbuffer Uniforms {
	float4	u_cameraPosition;	// XYZ - camera position in world space, W - far plane
	// these are unit vectors in world space:
	float4	u_cameraForward;	// XYZ - camera look-to direction, W - aspect ratio
	float4	u_cameraRight;	// XYZ - camera 'Right' vector, W - tan of half vertical FoV
	float4	u_cameraUp;	// camera 'Up' vector
	AtmosphereParameters	u_atmosphere;
};

struct VS_Output
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
	float3 rayFarWS : TexCoord1;	// the ray's end point on the far image plane
};
// The sky is rendered as a fullscreen triangle. The pixel shader traces rays through each pixel.
VS_Output TestSkyColor_VS( in uint vertexID : SV_VertexID )	// vertexID:=[0,1,2]
{
    VS_Output output;
	GetFullScreenTrianglePosTexCoord( vertexID, output.position, output.texCoord );

	const float farPlaneDist = u_cameraPosition.w;	// distance from eye position to the image plane
	const float aspectRatio = u_cameraForward.w;	// screen width / screen height
	const float tanHalfFoVy = u_cameraRight.w;
	
	const float imagePlaneH = farPlaneDist * tanHalfFoVy;	// half-height of the image plane
	const float imagePlaneW = imagePlaneH * aspectRatio;	// half-width of the image plane

	const float3 f3FarCenter = u_cameraForward.xyz * farPlaneDist;
	const float3 f3FarRight = u_cameraRight.xyz * imagePlaneW;	// scaled 'right' vector on the far plane, in world space
	const float3 f3FarUp = u_cameraUp.xyz * imagePlaneH;	// scaled 'up' vector on the far plane, in world space

	// Compute the positions of the triangle's corners on the far image plane, in world space.
	float3 cornersWS[3];
	// lower-left ( UV = ( 0, 2 ) )
	cornersWS[0] = u_cameraPosition.xyz + f3FarCenter - (f3FarRight * 1) - (f3FarUp * 3);
	// upper left ( UV = ( 0, 0 ) )
	cornersWS[1] = u_cameraPosition.xyz + f3FarCenter - (f3FarRight * 1) + (f3FarUp * 1);
	// upper-right ( UV = ( 2, 0 ) )
	cornersWS[2] = u_cameraPosition.xyz + f3FarCenter + (f3FarRight * 3) + (f3FarUp * 1);

	// the positions on the far image plane will be linearly interpolated and passed to the pixel shader
	output.rayFarWS = cornersWS[ vertexID ];

    return output;
}

bool GetRaySphereIntersection(
	in float3 f3RayOrigin,
	in float3 f3RayDirection,	// normalized
	in float3 f3SphereCenter,
	in float fSphereRadius,	// sphere radius
	out float2 f2Intersections	// intersection times
	)
{
    // http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    f3RayOrigin -= f3SphereCenter;	// pretend the sphere is centered at the origin
    float A = dot(f3RayDirection, f3RayDirection);
    float B = 2 * dot(f3RayOrigin, f3RayDirection);
    float C = dot(f3RayOrigin,f3RayOrigin) - fSphereRadius*fSphereRadius;
    float D = B*B - 4*A*C;
    // If discriminant is negative, there are no real roots hence the ray misses the sphere
    if( D < 0 ) {
        f2Intersections = -1;
		return false;
    } else {
        D = sqrt(D);
        f2Intersections = float2(-B - D, -B + D) / (2*A); // A must be positive here!!
		return true;
    }
}

///NOTE: assumes that the sphere is centered at the origin
bool RayIntersectsSphere(
	in float3 ro, in float3 rd,
	in float radius,	// sphere radius; the sphere is centered at the origin
	out float2 intersections	// intersection times
	)
{
    // ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    float A = dot(rd, rd);
    float B = 2 * dot(rd, ro);
    float C = dot(ro, ro) - (radius * radius);
    float D = (B*B) - 4*A*C;
    if( D < 0 ) {
//		intersections = float2(1e5,-1e5);
		return false;
	}
	const float sqrtD = sqrt(D);
	//NOTE: No intersection when result.x > result.y
    intersections = float2(
        (-B - sqrtD)/(2*A),
        (-B + sqrtD)/(2*A)
    );
	return true;
}

/// The number of ray marching steps from the camera to the end of the atmosphere or the first intersection with the ground.
static const int PRIMARY_RAY_STEPS = 16;
/// The number of ray marching steps from the current position (along the primary ray) to the main light source (e.g. the Sun, the Moon).
static const int SECONDARY_RAY_STEPS = 8;

/// Computes the sky color for one pixel using single scattering.
/// NOTE: assumes that the planet is centered at the origin.
float3 ComputeIncidentLight(
	in AtmosphereParameters _a,
	in float3 _eyePosition,	//!< the camera's position on the planet, in meters
	in float3 _lookDirection//!< camera look-to direction, must be normalized
	)
{
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	//
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

	// Calculate the closest intersection of the ray with the outer atmosphere (point A in Figure 16-3 in Gpu Gems 2 article).
	
	// scratch vars to store the time when the viewing ray enters and exits the atmosphere/Earth volume
	float2 rayAtmosphereHits, rayGroundHits;

	const bool bRayHitsAtmosphere = RayIntersectsSphere(
		_eyePosition, _lookDirection,
		_a.fOuterRadius,
		rayAtmosphereHits
	);
	if( !bRayHitsAtmosphere ) {
		return (float3)BLACK;	// the ray doesn't intersect the atmosphere - the eye is in space
	}

	const float rayStartTime = rayAtmosphereHits.x;	//!< negative if the eye is inside the atmosphere
//	const float rayStartTime = max( rayAtmosphereHits.x, 0.0f );

	const bool bRayHitsPlanet = RayIntersectsSphere(
		_eyePosition, _lookDirection,
		_a.fInnerRadius,
		rayGroundHits
	);
	const float rayEndTime = min( rayAtmosphereHits.y, rayGroundHits.x );	//!< always positive

	// Calculate the step size of the primary ray (for computing the optical depth).
	const float fSegmentLength = (rayEndTime - rayStartTime) / float(PRIMARY_RAY_STEPS);
	
    // Initialize the primary ray time.
    float fPrimaryRayTime = 0;

	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	//
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	
    // Initialize accumulators for Rayleigh and Mie scattering.
    float3 totalRlh = float3(0,0,0);
    float3 totalMie = float3(0,0,0);

    // Initialize optical depth ("density") accumulators for the primary ray.
    float fAccumOpticalDepthRayleigh = 0;
    float fAccumOpticalDepthMie = 0;

    // Sample the primary ray (which goes from the camera to the end of the atmosphere or the Earth ground.
    for( int i = 0; i < PRIMARY_RAY_STEPS; i++ )
	{
        // Calculate the primary ray sample position. Note that the sample position is the segment's middle point.
        const float3 f3SamplePosition = _eyePosition + _lookDirection * (fPrimaryRayTime + fSegmentLength * 0.5);

        // Calculate the height of the sample above the planet.
        const float fHeight = length(f3SamplePosition) - _a.fInnerRadius;

        // Calculate the optical depth of the Rayleigh and Mie scattering for this step.
        const float fSegmentOpticalDepthRayleigh = exp(-fHeight / _a.fRayleighHeight) * fSegmentLength;
        const float fSegmentOpticalDepthMie      = exp(-fHeight / _a.fMieHeight) * fSegmentLength;

        // Accumulate optical depth.
        fAccumOpticalDepthRayleigh += fSegmentOpticalDepthRayleigh;
        fAccumOpticalDepthMie += fSegmentOpticalDepthMie;

		
        // Calculate the step size of the secondary ray.
		RayIntersectsSphere(
			f3SamplePosition, _a.f3SunDirection,
			_a.fOuterRadius,
			rayAtmosphereHits
		);
        const float fSegmentLengthLight = rayAtmosphereHits.y / float(SECONDARY_RAY_STEPS);
        // Initialize the secondary ray time.
        float fSecondaryRayTime = 0;
        // Initialize optical depth ("density") accumulators for the secondary ray.
		// see "Chapter 16. Accurate Atmospheric Scattering" by Sean O'Neil in Gpu Gems 2, "16.2.3 The Out-Scattering Equation"
        float fSegmentOpticalDepthRayleigh2 = 0;
        float fSegmentOpticalDepthMie2 = 0;
        // Sample the secondary ray.
		[unroll]
        for( int j = 0; j < SECONDARY_RAY_STEPS; j++ )
		{
            // Calculate the secondary ray sample position. Note that the sample position is the segment's middle point.
            const float3 f3SamplePosition2 = f3SamplePosition + _a.f3SunDirection * (fSecondaryRayTime + fSegmentLengthLight * 0.5);
            // Calculate the height of the sample.
            const float fHeight2 = length(f3SamplePosition2) - _a.fInnerRadius;
            // Accumulate the optical depth.
            fSegmentOpticalDepthRayleigh2 += exp( -fHeight2 / _a.fRayleighHeight ) * fSegmentLengthLight;
            fSegmentOpticalDepthMie2      += exp( -fHeight2 / _a.fMieHeight ) * fSegmentLengthLight;
            // Increment the secondary ray time.
            fSecondaryRayTime += fSegmentLengthLight;
        }

        // Calculate attenuation ("transmittance") (this value equals "received_light / emitted_light" and should be in range (0..1)).
        const float3 f3Attenuation = exp(
			-(
			   _a.f3BetaRayleigh * (fAccumOpticalDepthRayleigh + fSegmentOpticalDepthRayleigh2) +
			   _a.f3BetaMie      * (fAccumOpticalDepthMie      + fSegmentOpticalDepthMie2)
			)
		);

        // Accumulate scattering.
        totalRlh += fSegmentOpticalDepthRayleigh * f3Attenuation;
        totalMie += fSegmentOpticalDepthMie      * f3Attenuation;

        // Increment the primary ray time.
        fPrimaryRayTime += fSegmentLength;
    }

    // Calculate the Rayleigh and Mie phases.
    const float mu = dot( _lookDirection, _a.f3SunDirection );	//!< mu is the cosine of the angle between the sun and the view directions
    const float mumu = mu * mu;
	const float g = _a.fMie;
    const float gg = g * g;
	// the phase function controls the "angular dependency":
	// it describes how much and in which directions light is scattered when it collides with particles
    const float fPhaseRayleigh = 3 / (16 * M_PI) * (1 + mumu);	//!< the phase function for Rayleigh scattering
    const float fPhaseMie = (3 / (8 * M_PI)) * ((1 - gg) * (mumu + 1))
							/ (pow(abs(1 + gg - 2 * mu * g), 1.5f) * (2 + gg));	// added abs(f) to prevent warning: pow(f,e) won't work for negative f

    // Calculate and return the final color.
	// see "Chapter 16. Accurate Atmospheric Scattering" by Sean O'Neil in Gpu Gems 2, "16.2.4 The In-Scattering Equation"
	const float3 skyColorRayleigh = _a.fSunIntensity * fPhaseRayleigh * _a.f3BetaRayleigh * totalRlh;
	const float3 skyColorMie      = _a.fSunIntensity * fPhaseMie      * _a.f3BetaMie      * totalMie;
    return skyColorRayleigh + skyColorMie;
}

float3 TestSkyColor_PS( in VS_Output _inputs ) : SV_Target
{
	float3 f3RayOrigin = u_cameraPosition.xyz;
	float3 f3RayEnd = _inputs.rayFarWS;
	float3 f3RayDirection = normalize( f3RayEnd - f3RayOrigin );

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

    float3 color = ComputeIncidentLight(
		u_atmosphere,
		//float3(0, 0, 6372e3),	//+ view from ground
		//float3(0, 0, 6472e3),	//+ view from space
		//float3(0, 0, 6500e3),	//+ view from space
		//float3(0, 0, 6382e3),	//+ view from ground
		u_cameraPosition.xyz,
        f3RayDirection
	);

	// Draw the Sun.
	const float3 SunColor = float3(1.0f, 0.8f, 0.3f) * 1500;
	const float SunWidth = 0.015f;
    const float angle = acos( dot( f3RayDirection, u_atmosphere.f3SunDirection ) );
    color = lerp( SunColor, color, saturate(abs(angle) / SunWidth) );

	// Apply exposure.
    color = 1.0 - exp(-1.0 * color);
	return color;
}
