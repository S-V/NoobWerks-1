/*
Particle system.

%%

pass VolumetricParticles
{
	vertex = VSParticlemain
	geometry = GSParticlemain
	pixel = PSVolumeParticlemain

	state = volumetric_particles
}

%%
*/

#include <Shared/nw_shared_globals.h>
#include <Common/Colors.hlsl>
#include <Common/transform.hlsli>
#include "_basis.h"	// buildOrthonormalBasisFromNormal()
#include "_PS.h"
#include "_fog.h"
#include "_noise.h"
#include <GBuffer/nw_gbuffer_read.h>	// DepthTexture
#include <Noise/SimplexNoise3D.hlsli>
#include <Noise/fBM.hlsli>

/*
==========================================================
	DEFINES
==========================================================
*/

/// Soft particles (aka Z-feather) blend the edges of particles so they reduce clipping against geo.

/// hard edges
#define	SOFT_PARTICLES_QUALITY_NONE		(0)

/// use the method from "Soft Particles" sample [Direct3D 10] in DirectX SDK [June 2010]
#define	SOFT_PARTICLES_QUALITY_CHEAP	(1)

/// use the method from "Soft Particles" sample [Direct3D 10] in NVIDIA Direct3D SDK 10 [2007]
#define	SOFT_PARTICLES_QUALITY_MEDIUM	(1)

///
#define	SOFT_PARTICLES_QUALITY			(SOFT_PARTICLES_QUALITY_CHEAP)

/*
==========================================================
	SHADER PARAMETERS
==========================================================
*/


/*
==========================================================
	VERTEX SHADER
	Vertex shader for drawing particles
==========================================================
*/

struct ParticleVertex
{
	float4	position_and_size: POSITION;

	// life: [0..1], if life > 1 => particle is dead
	float4	velocity_and_life: TexCoord;
};

struct VS_to_GS
{
    float3 Pos            : POSITION;
    float Life            : LIFE;	//stage of animation we're in [0..1] 0 is first frame, 1 is last
    float Size            : SIZE;
};

VS_to_GS VSParticlemain(ParticleVertex input)
{
    VS_to_GS output;
    
    output.Pos = input.position_and_size.xyz;
    output.Life = input.velocity_and_life.w;
    output.Size = input.position_and_size.w;
    
    return output;
}


/*
==========================================================
	GEOMETRY SHADER
	Geometry shader for creating point sprites
==========================================================
*/

struct GS_to_PS
{
	float4 posH : SV_Position;

	//float3 Tex			  : TexCoord0;

	float  Size			  : TexCoord3;
	float3 worldPos		  : Position1;	// world position

	float3 particleOrig	  : Position2;	// particle_center_WS
	float3 particleColor  : TexCoord6;

	float3 eye_to_particle_dir : TexCoord7;

	// view-space depth of this particle; used by soft particles
	float1 view_space_depth : TexCoord8;

	float1 life : LIFE;
};

float3 getTranslationFrom4x4Matrix( in float4x4 m )
{
	return float3( m._41, m._42, m._43 );
}

[maxvertexcount(4)]
void GSParticlemain(
	point VS_to_GS input[1]
	, inout TriangleStream< GS_to_PS > SpriteStream
	)
{
    GS_to_PS output;

	//
    const float3 particle_origin_WS = input[0].Pos;

	//const float3 eye_to_particle_dir = normalize( particle_origin_WS - g_inverse_view_matrix[3].xyz );
	const float3 eye_to_particle_dir = normalize( particle_origin_WS - g_camera_origin_WS.xyz );//getTranslationFrom4x4Matrix( g_inverse_view_matrix ) );

	output.particleOrig = particle_origin_WS.xyz;
	output.eye_to_particle_dir = eye_to_particle_dir;

	//
	output.Size = input[0].Size;

	output.life = input[0].Life;

    // calculate color from a 1d gradient texture
    float3 particleColor = RED.rgb;// g_txColorGradient.SampleLevel( g_samLinearClamp, float2(input[0].Life,0), 0 );
    output.particleColor = particleColor;

	//

	//
	const float radius = input[0].Size;

#if 0
	const float3 local_up		= g_camera_up_WS.xyz * radius;
	const float3 local_right	= g_camera_right_WS.xyz * radius;
#else
	const float3x3 basis = buildOrthonormalBasisFromNormal( g_camera_fwd_WS.xyz );
	const float3 local_up		= basis[0] * radius;
	const float3 local_right	= basis[1] * radius;
#endif

	//
	const float3 top_left_WS = particle_origin_WS - local_right + local_up;
	output.posH = mul( g_view_projection_matrix, float4(top_left_WS,1) );
	output.worldPos = top_left_WS;
	output.view_space_depth = mul( g_view_matrix, float4(top_left_WS,1) ).y;
	output.eye_to_particle_dir = top_left_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	const float3 top_right_WS = particle_origin_WS + local_right + local_up;
	output.posH = mul( g_view_projection_matrix, float4(top_right_WS,1) );
	output.worldPos = top_right_WS;
	output.view_space_depth = mul( g_view_matrix, float4(top_right_WS,1) ).y;
	output.eye_to_particle_dir = top_right_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	const float3 bottom_left_WS = particle_origin_WS - local_right - local_up;
	output.posH = mul( g_view_projection_matrix, float4(bottom_left_WS,1) );
	output.worldPos = bottom_left_WS;
	output.view_space_depth = mul( g_view_matrix, float4(bottom_left_WS,1) ).y;
	output.eye_to_particle_dir = bottom_left_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	const float3 bottom_right_WS = particle_origin_WS + local_right - local_up;
	output.posH = mul( g_view_projection_matrix, float4(bottom_right_WS,1) );
	output.worldPos = bottom_right_WS;
	output.view_space_depth = mul( g_view_matrix, float4(bottom_right_WS,1) ).y;
	output.eye_to_particle_dir = bottom_right_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	SpriteStream.RestartStrip();
}

/*
==========================================================
	PIXEL SHADER
==========================================================
*/
// ray-sphere intersection
#define DIST_BIAS 0.01
bool RaySphereIntersect(
						float3 rO, float3 rD,
						float3 sO, float sR,
						inout float tnear, inout float tfar
						)
{
    float3 delta = rO - sO;
    
    float A = dot( rD, rD );
    float B = 2*dot( delta, rD );
    float C = dot( delta, delta ) - sR*sR;
    
    float disc = B*B - 4.0*A*C;
    if( disc < DIST_BIAS )
    {
		// error X4000: Use of potentially uninitialized variable
		tnear = 0;
		tfar = 0;
        return false;
    }
    else
    {
        float sqrtDisc = sqrt( disc );
        tnear = (-B - sqrtDisc ) / (2*A);
        tfar = (-B + sqrtDisc ) / (2*A);
        return true;
    }
}

//
// PS for the volume particles
//

float dfSphere(
			   vec3 sample_position,
			   vec3 sphere_center, float sphere_radius
			   )
{
    return length( sample_position - sphere_center ) - sphere_radius;
}


float calcSoftParticleDepthContrast(float Input, float ContrastPower)
{
#if 1
     //piecewise contrast function
     bool IsAboveHalf = Input > 0.5 ;
     float ToRaise = saturate(2*(IsAboveHalf ? 1-Input : Input));
     float Output = 0.5*pow(ToRaise, ContrastPower); 
     Output = IsAboveHalf ? 1-Output : Output;
     return Output;
#else
    // another solution to create a kind of contrast function
    return 1.0 - exp2(-2*pow(2.0*saturate(Input), ContrastPower));
#endif
}




/// Returns: The Perlin noise value within a range between -1 and 1.
/// X - A floating-point vector (any dimension) from which to generate Perlin noise.
#define HW_PERLIN_NOISE(X)	noise((X))



/// Fractional Brownian Motion
/// https://thebookofshaders.com/13/
/// http://iquilezles.org/www/articles/fbm/fbm.htm
///

struct fBM_snoise_3D
{
	int		_octaves;
	float	_frequency;
	float	_amplitude;
	float	_lacunarity;
	float	_gain;

	float compute(
		in vec3 P

		// essentially an fBm, but constructed from the absolute value of a signed noise to create sharp valleys in the function
		, in bool turbulence
		)
	{
		float value = 0.0f;

		float frequency = _frequency;
		float amplitude = _amplitude;

		for( int i = 0; i < _octaves; i++ )
		{
			float noise_value = snoise( P * frequency );	// [-1..+1]
			if( turbulence ) {
				noise_value = abs(noise_value);
			}
			value += noise_value * amplitude;
			amplitude *= _gain;
			frequency *= _lacunarity;
		}

		return value;
	}

	void setDefaults()
	{
		_octaves	= 1;
		_frequency	= 1.0f;
		_amplitude	= 0.5f;
		_lacunarity	= 2.0f;
		_gain		= 0.5f;
	}
};




void sampleParticleDistanceField(
								in float3 sample_position,
								//in float3 ray_direction,
								in float3 sphere_center,
								in float1 sphere_radius,
								in float1 particle_life01,

								out float3 color_,
								out float1 opacity_,	// clamped to [0..1]
								out float1 distance_
								)
{
	//
	float3	final_color = DARK_GRAY.rgb;

	//
	const float rad0 = sphere_radius * 0.1;
	const float rad1 = sphere_radius * 0.7;
	const float rad2 = sphere_radius * ( 1 - rad1 );


	// Calculate the distance from the center of the volume.
	float signed_distance = dfSphere( sample_position, sphere_center, rad0 ) + particle_life01;

	float distance01 = max( signed_distance, 0 ) / ( rad1 - rad0 );


	// Perturb the distance using noise.

	fBM_SimplexNoise3D	fBM;
	fBM.SetDefaults();
	fBM._num_octaves = 1;
	fBM._frequency	= 1.0f;
	fBM._lacunarity	= 2.0f;
	fBM._gain		= 0.5f;

	float noise_value = fBM.Evaluate(
		sample_position - sphere_center
		, false	// turbulence
		);

	float delta_height = noise_value;

	float perturbed_distance = distance01 + delta_height;

	float opacity = 1 - perturbed_distance;	// linear falloff
	
	//
	color_ = final_color;
	opacity_ = saturate( opacity );
	distance_ = signed_distance;
}

///
float4 PSVolumeParticlemain( GS_to_PS input ) : SV_Target
{
//return float4(input.particleColor, 0.5f);


	//
	const float3 sphere_center = input.particleOrig;	// in world space
    const float sphere_radius = input.Size;

	//

	// We need to compare the distance stored in the depth buffer
	// and the value that came in from the GS.
	// Because the depth values aren't linear, we need to transform the depthsample back into view space
	// in order for the comparison to give us an accurate distance.

	const float hardware_depth = DepthTexture.Load( int3( input.posH.x, input.posH.y, 0 ) );

	const float view_space_depth_from_zbuffer = hardwareZToViewSpaceDepth( hardware_depth );
	const float view_space_depth_from_particle = input.view_space_depth;

	//

#if SOFT_PARTICLES_QUALITY == SOFT_PARTICLES_QUALITY_NONE


	float depthFade = 1;


/// use the method from "Soft Particles" sample [Direct3D 10] in DirectX SDK [June 2010]
#elif SOFT_PARTICLES_QUALITY == SOFT_PARTICLES_QUALITY_CHEAP


	float depthDiff = view_space_depth_from_zbuffer - view_space_depth_from_particle;
	if( depthDiff < 0 ) {
		discard;
	}

	float g_fFadeDistance = sphere_radius * 0.5f;	// 0 = hard edges
	float depthFade = saturate( depthDiff / g_fFadeDistance );


/// use the method from "Soft Particles" sample [Direct3D 10] in NVIDIA Direct3D SDK 10 [2007]
#elif SOFT_PARTICLES_QUALITY == SOFT_PARTICLES_QUALITY_MEDIUM


	float SoftParticleContrast = 2.0;
	float SoftParticleScale = 1.0;
	float zEpsilon = 0;

	const float zdiff = ( view_space_depth_from_zbuffer - input.Z );
	const float depthFade = calcSoftParticleDepthContrast(
		zdiff * SoftParticleScale,
		SoftParticleContrast
		);
	if( depthFade * zdiff <= zEpsilon )
	{
		discard;
	}


#endif

	//




    // ray sphere intersection
	const float3 worldPos = input.worldPos;
	//const float3 rayDir = g_camera_fwd_WS.xyz;
	const float3 rayDir = normalize(input.eye_to_particle_dir);


	// the interval along the ray overlapped by the volume
	float tnear, tfar;

	// find intersection with sphere
	if( !RaySphereIntersect( worldPos, rayDir, sphere_center, sphere_radius, tnear, tfar ) ) {
		// not intersecting
        discard;
	}

	// We don't want to sample voxels behind the eye
	// if it's inside the volume,
	// so keep the starting point at or in front of the eye.
	tnear = max( tnear, 0 );


	// Starting from the entry point,
	// march the ray through the volume and sample it

	const float3 rayOrigin = worldPos;
	//float3 worldNear = rayOrigin + rayDir * tnear;
	//float3 worldFar  = rayOrigin + rayDir * tfar;

	//
	static const int NUM_STEPS = 8;

	// Compute the step size to march through the volume grid.
	const float min_step_size = ( tfar - tnear ) / ( NUM_STEPS - 1 );

	// Total ray distance travelled.
    float t = tnear;

	//
	float	accumulated_opacity = 0;
	float3	accumulated_color = (float3) 0;

	// Raymarching/Sphere Tracing loop.
	// This loop continues until the ray either leaves the volume, or the accumulated color has become opaque (alpha=1).
	for( int i = 0; i < NUM_STEPS; i++)
	{
		// Loop break conditions.
		// Break out of the loop when the color is near opaque.
		if( accumulated_opacity >= 0.95 ) {
			break;
		}

		// Current ray position.
		const float3 curr_pos_on_ray = rayOrigin + rayDir * t;

		// Sample the volume.
		float3 sampled_color;		// 
		float1 sampled_opacity;		// [0..1]
		float1 sampled_distance;	// signed distance to the closest surface

		sampleParticleDistanceField(
			curr_pos_on_ray,
			sphere_center,
			sphere_radius,
			input.life,

			sampled_color,
			sampled_opacity,
			sampled_distance
			);

		// Accumulate the color and opacity
		// using the front-to-back compositing equation.

		const float transmittance = 1 - accumulated_opacity;
		const float sample_alpha = transmittance * sampled_opacity;

		accumulated_color   += sample_alpha * sampled_color;
		accumulated_opacity += sample_alpha;

		// Enforce minimum stepsize.
		//t += max( min_step_size, sampled_distance );
		t += min_step_size;
	}

//return float4(input.particleColor, 0.5f);

	const float life = input.life;	// dead if life > 1

	return float4(
		accumulated_color,
		accumulated_opacity * ( 1 - life ) * depthFade
		);
}
