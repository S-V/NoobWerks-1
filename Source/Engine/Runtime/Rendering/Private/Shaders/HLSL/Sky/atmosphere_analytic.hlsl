/*
Renders sky colors with Rayleigh and Mie scattering.

Based on "Atmosphere by Dimas Leenman, Shared under the MIT license":
https://github.com/Dimev/Realistic-Atmosphere-Godot-and-UE4
https://www.shadertoy.com/view/wlBXWK

References:

[detailed tutorial by Alan Zucconi] https://www.alanzucconi.com/2017/10/10/atmosphSeric-scattering-1/
[scratchapixel](http://www.scratchapixel.com/lessons/goodies/simulating-sky)
[nvidia](http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html)
[patapom](http://patapom.com/topics/Revision2013/Revision%202013%20-%20Real-time%20Volumetric%20Rendering%20Course%20Notes.pdf)
[glsl-atmosphere]: http://wwwtyro.github.io/glsl-atmosphere/

[Tectonics.js]
https://davidson16807.github.io/tectonics.js//2019/03/24/fast-atmospheric-scattering.html
https://github.com/davidson16807/tectonics.js/blob/master/precompiled/shaders/fragment/atmosphere.glsl
*/

#include "base.hlsli"
#include <Common/Colors.hlsl>
#include <Shared/nw_shared_definitions.h>
#include <Utility/_GLSL_to_HLSL.h>


#if 0

// Math helper constants
const float e = 2.71828182845904523536028747135266249775724709369995957;
const float PI = 3.141592653589793238462643383279502884197169;

// # Primary Wavelengths
// According to Preetham
const vec3 lambda = vec3( 680E-9, 550-9, 450E-9 );

// # Luminance
// Luminance is a photometric measure of the luminous intensity per unit area of
// light travelling in a given direction. It describes the amount of light that passes through,
// is emitted or reflected from a particular area, and falls within a given solid angle.
const float luminance = 1.0;

// # Turbidity
// Turbidity is the cloudiness or haziness of a fluid caused by
// large numbers of individual particles that are generally
// invisible to the naked eye, similar to smoke in air.
// Measurement unit for turbidity is the Formazin Turbidity Unit (FTU).
// ISO refers to its units as FNU (Formazin Nephelometric Units).
const float turbidity = 10.0;

// # Rayleigh Scattering Coefficient
const float rayleigh = 2.0;
// Optical length at zenith for molecules
const float rayleighZenithLength = 8.4E3;

// # Mie Scattering (Lorenz–Mie solution)
// Describes the scattering of an electromagnetic plane wave by a homogeneous sphere.
const float mieCoefficient = 0.005;
const float mieDirectionalG = 0.8;
// Optical length at zenith for molecules
const float mieZenithLength = 1.25E3;
// K coefficient for primary wavelengths
const vec3 mieK = vec3( 0.686, 0.678, 0.666 );

// # Refractive Index
// In optics the "refractive index" or "index of refraction" `n` of an optical medium is
// a dimensionless number that describes how light, or any other radiation,
// propagates through that medium. It is defined as: `n = c/v`,
// where `c` is the speed of light in vacuum and `v` is the phase velocity of light in the medium.
const float refractiveIndex = 1.0003;

// # Loschmidt constant
// Loschmidt's number (symbol: n0) is the number of particles
// (atoms or molecules) of an ideal gas in a given volume (the number density).
const float loschmidtNumber = 2.545E25;

// # Depolarization factor
// Describes polarization randomization (Symbol: pn)
// Value for standard air, at 288.15K and 1013mb (sea level -45 celsius)
const float depolarization = 0.035;

#endif

struct AtmosphereRenderContext
{
	float3	camera_position;

	/// direction from the eye towards the scene
	float3	eye_to_pixel_ray_direction;

	/// direction TOWARDS the Sun
	float3	light_vector;

	/// if the eye ray doesn't intersect the atmosphere
	//float3	background_scene_color;

	/// max ray distance
	float	max_scene_depth;

	void setDefaults()
	{
		//background_scene_color = (float3)0;
		max_scene_depth = 1e12;
	}
};



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

	if( D < 0 )
	{
		//intersections = float2(1e5,-1e5);
		return false;
	}
	else
	{
		const float sqrtD = sqrt(D);
		//NOTE: No intersection when result.x > result.y
		intersections = float2(
			(-B - sqrtD)/(2*A),	// min time
			(-B + sqrtD)/(2*A)	// max time
		);
		return true;
	}
}









struct Ray
{
    float3 origin;
    float3 direction;	// normalized!
};

struct Segment: Ray
{
    //float3	origin;
    //float3	direction;	// normalized!
	float	max_distance;
};


///NOTE: assumes that the sphere is centered at the origin
bool RayIntersectsSphere2(
	in Ray ray,
	in float radius,	// sphere radius; the sphere is centered at the origin
	out float2 intersections	// intersection times (distances along the ray)
	)
{
    // ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    float A = dot(ray.direction, ray.direction);
    float B = 2 * dot(ray.direction, ray.origin);
    float C = dot(ray.origin, ray.origin) - (radius * radius);
    float D = (B*B) - 4*A*C;
    if( D < 0 ) {
		// early exit if there is no intersection
//		intersections = float2(1e5,-1e5);
		return false;
	}
	const float sqrtD = sqrt(D);
	//NOTE: No intersection when result.x > result.y (the sphere is behind)
    intersections = float2(
        (-B - sqrtD)/(2*A),
        (-B + sqrtD)/(2*A)
    );
	return true;// intersections.x <= intersections.y;
}


///NOTE: assumes that the sphere is centered at the origin
bool intersect_Segment_vs_Sphere0(
	in Segment  segment,
	in float    radius,	// sphere radius; the sphere is centered at the origin
	out float2  intersections_	// intersection times (distances along the ray)
	)
{
    // NOTE: the sphere is centered at the origin.

	// NOTE: the direction is normalized,
	// and the dot product of a unit vector with itself is the square of its magnitude
	// => A = 1 and computations can be simplified.
	//const float A = dot( segment.direction, segment.direction );

    const float B =  dot( segment.direction, segment.origin );
    const float C = dot( segment.origin, segment.origin ) - ( radius * radius );	//@TODO: precompute radius sq
    const float D = B * B - C;

	// A negative discriminant corresponds to ray missing sphere
	if( D < 0 ) {
		// early exit if there is no intersection
//		intersections = float2(1e5,-1e5);
		return false;
	}

	// Ray now found to intersect sphere
	const float sqrtD = sqrt(D);

	// Compute smallest and largest t values of intersection.
	const float tmin = ( -B - sqrtD );
	const float tmax = ( -B + sqrtD );

	// NOTE: If tmin is negative, ray started inside sphere so clamp t to zero.
	const float tmin_clamped = max( tmin, 0 );

	if( tmin_clamped <= tmax
		&& tmax <= segment.max_distance )
	{
		intersections_.x = tmin_clamped;
		intersections_.y = tmax;
		return true;
	}

	return false;
}


/// The number of ray marching steps from the camera to the end of the atmosphere or the first intersection with the ground.
static const int PRIMARY_RAY_STEPS = 64;
/// The number of ray marching steps from the current position (along the primary ray) to the main light source (e.g. the Sun, the Moon).
static const int SECONDARY_RAY_STEPS = 16;

/// Computes the sky color for one pixel using single scattering.
/// NOTE: assumes that the planet is centered at the origin.
float3 computeScatteredColor(
	in AtmosphereParameters atmo,

	in AtmosphereRenderContext p,

	/// The camera's position and the direction of the ray (the camera vector).
	//in Ray eye_segment

	out float opacity_
	)
{
	// points from the pixel into the scene
	Segment eye_segment;
	eye_segment.origin = p.camera_position;
	eye_segment.direction = p.eye_to_pixel_ray_direction;
	eye_segment.max_distance = p.max_scene_depth;

	//
	const float3 light_direction = -p.light_vector;

	// Calculate the closest intersection of the ray with the outer atmosphere (point A in Figure 16-3 in Gpu Gems 2 article).

	// scratch vars to store the time when the viewing ray enters and exits the atmosphere
	float2 ray_atmosphere_hits;

	//
	const bool ray_hits_atmosphere = intersect_Segment_vs_Sphere0(
		eye_segment,
		atmo.outer_radius,
		ray_atmosphere_hits
	);
	if( !ray_hits_atmosphere ) {
		return BLACK.rgb;	// the ray doesn't intersect the atmosphere - the eye is in space
	}

//++
//return RED.rgb * 0.2f;


#if 0
	// scratch var to store the times when the viewing ray enters and exits the Earth volume
	float2 ray_ground_hits;

	const bool ray_hits_planet = intersect_Segment_vs_Sphere0(
		eye_segment,
		atmo.inner_radius,
		ray_ground_hits
	);
//++
if( ray_hits_planet ) {
	return BLUE.rgb * 0.2f;
}
#endif


	//
#if 1
	const float primary_ray_start_time = ray_atmosphere_hits.x;
	const float primary_ray_end_time = min( ray_atmosphere_hits.y, p.max_scene_depth );
#else
	const float primary_ray_start_time = 0;//ray_atmosphere_hits.x - 1000;
	//ray_atmosphere_hits.y += 900;
	//const float primary_ray_end_time = min( ray_atmosphere_hits.y, ray_ground_hits.x );	// always positive
	//const float primary_ray_end_time = ray_atmosphere_hits.y;
	//const float primary_ray_end_time = ray_ground_hits.x;
	const float primary_ray_end_time = min( ray_atmosphere_hits.y, p.max_scene_depth );
#endif


	// Calculate the step size of the primary ray (for computing the optical depth).
	const float primary_segment_length = ( primary_ray_end_time - primary_ray_start_time ) / float(PRIMARY_RAY_STEPS);

//+
//if( primary_segment_length > 0 ) {
//	return GREEN.rgb * 0.2f;
//}

    // Initialize the primary ray time.

	// If the camera is outside the atmosphere,
	// the ray should start at the edge of the atmosphere (where the rays enters the atmosphere).
	// If the camera is inside the atmosphere, the ray should start at the camera's position ( i.e. start_time = 0 ).
	float primary_ray_time = primary_ray_start_time;

	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    // Initialize accumulators for Rayleigh and Mie scattering.
    float3 totalRlh = (float3) 0;
    float3 totalMie = (float3) 0;

    // Initialize optical depth ("density") accumulators for the primary ray.
    float accum_optical_depth_Rayleigh = 0;
    float accum_optical_depth_Mie      = 0;

    // Sample the primary ray (which goes from the camera to the end of the atmosphere or the Earth ground.
    for( int i = 0; i < PRIMARY_RAY_STEPS; i++ )
	{
        // Calculate the primary ray sample position. Note that the sample position is the segment's middle point.
        const float3 pos_on_eye_ray
			= eye_segment.origin
			+ eye_segment.direction * ( primary_ray_time + primary_segment_length * 0.5 )
			;

        // Calculate the height of the sample above the planet.
        const float height_above_ground = length(pos_on_eye_ray) - atmo.inner_radius;

        // Calculate the optical depth of the Rayleigh and Mie scattering for this step.
        const float eye_segment_optical_depth_Rayleigh = exp( -height_above_ground / atmo.height_Rayleigh ) * primary_segment_length;
        const float eye_segment_optical_depth_Mie      = exp( -height_above_ground / atmo.height_Mie )      * primary_segment_length;

        // Accumulate optical depth.
        accum_optical_depth_Rayleigh += eye_segment_optical_depth_Rayleigh;
        accum_optical_depth_Mie      += eye_segment_optical_depth_Mie;

        // Calculate the step size of the secondary ray.
		Ray light_ray;
		light_ray.origin = pos_on_eye_ray;
		light_ray.direction = p.light_vector;

		RayIntersectsSphere2(
			light_ray,
			atmo.outer_radius,
			ray_atmosphere_hits
		);

		const float light_segment_length = ray_atmosphere_hits.y / float(SECONDARY_RAY_STEPS);
        // Initialize the secondary ray time.
        float light_ray_time = 0;
        // Initialize optical depth ("density") accumulators for the secondary ray.
		// see "Chapter 16. Accurate Atmospheric Scattering" by Sean O'Neil in Gpu Gems 2, "16.2.3 The Out-Scattering Equation"
        float light_segment_optical_depth_Rayleigh = 0;
        float light_segment_optical_depth_Mie = 0;

		// Sample the secondary ray.
		[unroll]
        for( int j = 0; j < SECONDARY_RAY_STEPS; j++ )
		{
            // Calculate the secondary ray sample position. Note that the sample position is the segment's middle point.
            const float3 pos_on_light_ray
				= pos_on_eye_ray
				+ p.light_vector * ( light_ray_time + light_segment_length * 0.5 )
				;
            // Calculate the height of the sample.
            const float fHeight2 = length(pos_on_light_ray) - atmo.inner_radius;
            // Accumulate the optical depth.
            light_segment_optical_depth_Rayleigh += exp( -fHeight2 / atmo.height_Rayleigh ) * light_segment_length;
            light_segment_optical_depth_Mie      += exp( -fHeight2 / atmo.height_Mie )      * light_segment_length;
			// Increment the secondary ray time.
            light_ray_time += light_segment_length;
        }//

        // Calculate attenuation ("transmittance").
		// This value equals "received_light / emitted_light" and should be in range (0..1).
        const float3 attenuation = exp(
			-(
			   atmo.beta_Rayleigh * (accum_optical_depth_Rayleigh + light_segment_optical_depth_Rayleigh) +
			   atmo.beta_Mie      * (accum_optical_depth_Mie      + light_segment_optical_depth_Mie)
			)
		);

        // Accumulate scattering.
        totalRlh += eye_segment_optical_depth_Rayleigh * attenuation;
        totalMie += eye_segment_optical_depth_Mie      * attenuation;

        // Increment the primary ray time.
        primary_ray_time += primary_segment_length;
    }

    // Calculate the Rayleigh and Mie phases.

	// mu is the cosine of the angle between the sun and the view directions
	const float mu = dot( eye_segment.direction, p.light_vector );
    const float mumu = mu * mu;
	const float g = atmo.g;
    const float gg = g * g;

	// the phase function controls the "angular dependency":
	// it describes how much and in which directions light is scattered when it collides with particles
#if 0

    const float phase_Rayleigh = 3 / (16 * M_PI) * (1 + mumu);	//!< the phase function for Rayleigh scattering

	const float phase_Mie = (3 / (8 * M_PI)) * ((1 - gg) * (mumu + 1))
							/ (pow(abs(1 + gg - 2 * mu * g), 1.5f) * (2 + gg));	// added abs(f) to prevent warning: pow(f,e) won't work for negative f

#else

	float phase_Rayleigh = 3.0 / (50.2654824574 /* (16 * pi) */) * (1.0 + mumu);

	float phase_Mie
		= true
		? ( 3.0 / (25.1327412287 /* (8 * pi) */) * ((1.0 - gg) * (mumu + 1.0)) )
				/ (pow(abs(1.0 + gg - 2.0 * mu * g), 1.5) * (2.0 + gg))	// added abs(f) to prevent warning: pow(f,e) won't work for negative f
		: 0.0
		;

#endif

    // Calculate and return the final color.
	// see "Chapter 16. Accurate Atmospheric Scattering" by Sean O'Neil in Gpu Gems 2, "16.2.4 The In-Scattering Equation"
	const float3 scattered_color_Rayleigh = phase_Rayleigh * atmo.beta_Rayleigh * totalRlh;
	const float3 scattered_color_Mie      = phase_Mie      * atmo.beta_Mie      * totalMie;

	const float3 scattered_color = scattered_color_Rayleigh + scattered_color_Mie;


	// calculate how much light can pass through the atmosphere

	const float3 total_attenuation = exp(
		-(
		atmo.beta_Rayleigh * accum_optical_depth_Rayleigh
		+
		atmo.beta_Mie      * accum_optical_depth_Mie
		)
		* atmo.density_multiplier
	);

	const float opacity = length( total_attenuation );

	//
	float3 atmosphere_color = scattered_color * atmo.sun_intensity;


	//if( opacity <= 0 ) {
	//	atmosphere_color += GREEN.rgb * 0.2;
	//}

	opacity_ = opacity;

	return atmosphere_color;
}


float3 GetSkyColorOnEarthGround(
								in float3 light_vector_WS
								, in float3 eye_to_pixel_ray_direction_WS
								)
{

	AtmosphereParameters	atmo;
	atmo.preset_Earth();

	AtmosphereRenderContext atmo_ctx;
	atmo_ctx.setDefaults();

	atmo_ctx.camera_position = float3(
		0, 0,
		6382e3//+100
		);
	atmo_ctx.eye_to_pixel_ray_direction = eye_to_pixel_ray_direction_WS;
	atmo_ctx.light_vector = light_vector_WS;


	//
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
	const float angle = acos( dot( eye_to_pixel_ray_direction_WS, light_vector_WS ) );
	color = lerp( SunColor, color, saturate(abs(angle) / SunWidth) );
#endif

	return color;
}
