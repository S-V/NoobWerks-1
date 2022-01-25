/*
Particle system.

%%

// NOTE: draw before smoke trails
//pass VolumetricParticles
pass Deferred_Translucent
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
	VERTEX SHADER
	Vertex shader for drawing particles
==========================================================
*/

struct ParticleVertex
{
	float4	position_and_type: Position;

	// w: life in [0..1], if life > 1 => particle is dead
	float4	direction_and_life: TexCoord;
};

struct VS_to_GS
{
    float3 pos : Position;
	float3 direction: Normal;
    float life : Life;	//stage of animation we're in [0..1] 0 is first frame, 1 is last
    float size : Size;
	float3 color: Color;
};

VS_to_GS VSParticlemain(ParticleVertex input)
{
    VS_to_GS output;
    
    output.pos = input.position_and_type.xyz;
	output.direction = input.direction_and_life.xyz;
    output.life = input.direction_and_life.w;

    output.size = 1.0f;


	//
	const float ship_type = input.position_and_type.w;

#define obj_small_fighter_ship	2
#define obj_combat_fighter_ship	3
#define obj_alien_fighter_ship	4

	if(ship_type <= obj_small_fighter_ship) {
		output.color = float3(0.7, 0.7, 0.9);
	}
	else if(ship_type == obj_combat_fighter_ship) {
		output.color = float3(0.01f, 0.8f, 0.02f);
	}
	else {
		output.color = float3(1.0f, 0.001f, 0.001f);
	}

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

	float  size			  : Size;
	float3 world_pos	  : WorldSpacePosition0;

	float3 particle_center: WorldSpacePosition1;
	
	float3 top_cap_center: WorldSpacePosition2;
	float3 bottom_cap_center: WorldSpacePosition3;

	float3 eye_to_particle_dir : Normal;

	// view-space depth of this particle; used by soft particles
	float1 view_space_depth : TexCoord0;

	float1 life : Life;
	float3 color: Color;
};

[maxvertexcount(4)]
void GSParticlemain(
	point VS_to_GS input[1]
	, inout TriangleStream< GS_to_PS > SpriteStream
	)
{
    GS_to_PS output;

	//
    const float3 particle_origin_WS = input[0].pos;

	const float3 eye_to_particle_dir = normalize( particle_origin_WS - g_camera_origin_WS.xyz );

	output.particle_center = particle_origin_WS;

	const float half_size = input[0].size * 0.5f;
	output.top_cap_center = particle_origin_WS + input[0].direction * half_size;
	output.bottom_cap_center = particle_origin_WS - input[0].direction * half_size;

	output.eye_to_particle_dir = eye_to_particle_dir;

	//
	output.size = input[0].size;

	output.life = input[0].life;
	
	output.color = input[0].color;

	//
	const float radius = input[0].size;

	const float3x3 basis = buildOrthonormalBasisFromNormal( g_camera_fwd_WS.xyz );
	const float3 local_up		= basis[0] * radius;
	const float3 local_right	= basis[1] * radius;

	//
	const float3 top_left_WS = particle_origin_WS - local_right + local_up;
	output.posH = mul( g_view_projection_matrix, float4(top_left_WS,1) );
	output.world_pos = top_left_WS;
	output.view_space_depth = mul( g_view_matrix, float4(top_left_WS,1) ).y;
	output.eye_to_particle_dir = top_left_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	const float3 top_right_WS = particle_origin_WS + local_right + local_up;
	output.posH = mul( g_view_projection_matrix, float4(top_right_WS,1) );
	output.world_pos = top_right_WS;
	output.view_space_depth = mul( g_view_matrix, float4(top_right_WS,1) ).y;
	output.eye_to_particle_dir = top_right_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	const float3 bottom_left_WS = particle_origin_WS - local_right - local_up;
	output.posH = mul( g_view_projection_matrix, float4(bottom_left_WS,1) );
	output.world_pos = bottom_left_WS;
	output.view_space_depth = mul( g_view_matrix, float4(bottom_left_WS,1) ).y;
	output.eye_to_particle_dir = bottom_left_WS - g_camera_origin_WS.xyz;
	SpriteStream.Append(output);

	//
	const float3 bottom_right_WS = particle_origin_WS + local_right - local_up;
	output.posH = mul( g_view_projection_matrix, float4(bottom_right_WS,1) );
	output.world_pos = bottom_right_WS;
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

// intersect capsule : http://www.iquilezles.org/www/articles/intersectors/intersectors.htm
float capIntersect( in float3 ro, in float3 rd, in float3 pa, in float3 pb, in float r )
{
    float3  ba = pb - pa;
    float3  oa = ro - pa;

    float baba = dot(ba,ba);
    float bard = dot(ba,rd);
    float baoa = dot(ba,oa);
    float rdoa = dot(rd,oa);
    float oaoa = dot(oa,oa);

    float a = baba      - bard*bard;
    float b = baba*rdoa - baoa*bard;
    float c = baba*oaoa - baoa*baoa - r*r*baba;
    float h = b*b - a*c;
    if( h>=0.0 )
    {
        float t = (-b-sqrt(h))/a;
        float y = baoa + t*bard;
        // body
        if( y>0.0 && y<baba ) return t;
        // caps
        float3 oc = (y<=0.0) ? oa : ro - pb;
        b = dot(rd,oc);
        c = dot(oc,oc) - r*r;
        h = b*b - c;
        if( h>0.0 ) return -b - sqrt(h);
    }
    //return -1.0;
	return 0;
}


///
float4 PSVolumeParticlemain( GS_to_PS input ) : SV_Target
{
	//
	const float3 sphere_center = input.particle_center;	// in world space
    const float sphere_radius = input.size;

	//

	// We need to compare the distance stored in the depth buffer
	// and the value that came in from the GS.
	// Because the depth values aren't linear, we need to transform the depthsample back into view space
	// in order for the comparison to give us an accurate distance.

	const float hardware_depth = DepthTexture.Load( int3( input.posH.x, input.posH.y, 0 ) );

	const float view_space_depth_from_zbuffer = hardwareZToViewSpaceDepth( hardware_depth );
	const float view_space_depth_from_particle = input.view_space_depth;

	//

	// use the method from "Soft Particles" sample [Direct3D 10] in DirectX SDK [June 2010]

	float depth_diff = view_space_depth_from_zbuffer - view_space_depth_from_particle;
	if( depth_diff < 0 ) {
		discard;
	}

	const float fade_distance = sphere_radius * 0.5f;	// 0 = hard edges
	const float depth_fade = saturate( depth_diff / fade_distance );

	//

    // ray sphere intersection
	const float3 world_pos = input.world_pos;
	const float3 ray_dir = normalize(input.eye_to_particle_dir);


	// the interval along the ray overlapped by the volume
	float tnear, tfar;

	// find intersection with sphere
	if( !RaySphereIntersect( world_pos, ray_dir, sphere_center, sphere_radius, tnear, tfar ) ) {
		// not intersecting
        discard;
	}


	// Starting from the entry point,
	// march the ray through the volume and sample it

	const float3 ray_origin = world_pos;

	//
	const float accumulated_opacity = capIntersect(
		ray_origin - ray_dir, ray_dir
		, input.top_cap_center
		, input.bottom_cap_center
		, 0.07f // capR
	);

	//
	const float life = input.life;	// dead if life > 1

	return float4(
		input.color,
		accumulated_opacity * ( 1 - life ) * depth_fade
		);
}
