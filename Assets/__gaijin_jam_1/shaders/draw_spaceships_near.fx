/*

Draws many instanced spaceships.

%%

pass FillGBuffer
{
	vertex = main_VS
	pixel = main_PS
	state = default
}

samplers
{
	s_sdf_volume = BilinearSampler
}

%%
*/

#include <Shared/nw_shared_globals.h>
#include <Common/Colors.hlsl>
#include <Common/transform.hlsli>

#include <GBuffer/nw_gbuffer_write.h>

#include "base.hlsli"	//UnpackVertexNormal


/*
==========================================================
	DEFINES
==========================================================
*/

#define SG_USE_SDF_VOLUMES	(0)

///
#define MAX_CB_SIZE		(65536)

/*
==========================================================
	SHADER PARAMETERS
==========================================================
*/

struct SgRBInstanceData
{
	float4	pos;
	float4	quat;
};

#define SIZE_OF_PER_INSTANCE_DATA	(16*2)
#define MAX_INSTANCES	(MAX_CB_SIZE/SIZE_OF_PER_INSTANCE_DATA)


DECLARE_CBUFFER( G_InstanceData, INSTANCE_CBUFFER_SLOT )
{
    SgRBInstanceData	g_instance_transforms[MAX_INSTANCES];
};

Texture3D< float >	t_sdf_volume;
SamplerState		s_sdf_volume;

/*
==========================================================
	FUNCTIONS
==========================================================
*/
column_major float3x3 GetRotationMatrixFromQuat(in float4 q)
{
	const float len_sq = dot(q, q);
	const float s = 2.f / len_sq;

	const float xs = q.x *  s, ys = q.y *  s, zs = q.z *  s;
	const float wx = q.w * xs, wy = q.w * ys, wz = q.w * zs;
	const float xx = q.x * xs, xy = q.x * ys, xz = q.x * zs;
	const float yy = q.y * ys, yz = q.y * zs, zz = q.z * zs;

	return float3x3(
		1.f - (yy + zz),  xy - wz,          xz + wy,
		xy + wz,          1.f - (xx + zz),  yz - wx,
		xz - wy,          yz + wx,          1.f - (xx + yy)
		);
}

column_major float4x4 GetMatrixFromPosAndQuat(in float3 pos, in float4 quat)
{
	const float3x3	rot = GetRotationMatrixFromQuat(quat);

	float4x4	m;

	// we use column-major matrices in shaders (vs row-major in app code), hence the transpose:
	m._11 = rot._11;    m._12 = rot._12;    m._13 = rot._13;    m._14 = pos.x;
	m._21 = rot._21;    m._22 = rot._22;    m._23 = rot._23;    m._24 = pos.y;
	m._31 = rot._31;    m._32 = rot._32;    m._33 = rot._33;    m._34 = pos.z;
	m._41 = 0;          m._42 = 0;          m._43 = 0;          m._44 = 1;

	return m;
}


/*
==========================================================
	VERTEX SHADER
==========================================================
*/

struct SpaceshipVertex
{
	/// Half4, [-1..+1]
	float4	position : Position;	// 8
};

struct VS_to_PS
{
    float4	position_cs : SV_Position;
	float3	position_vs: Position;
	
	nointerpolation float3	color: Color;
};

VS_to_PS main_VS(
	in SpaceshipVertex vertex_in
	, in uint instance_id: SV_InstanceID
)
{
	const float4 local_position = float4( vertex_in.position.xyz, 1 );

	//
	const SgRBInstanceData	per_instance_data = g_instance_transforms[ instance_id ];

	//
    VS_to_PS output;
    
	//
	const float4x4 world_matrix = GetMatrixFromPosAndQuat(per_instance_data.pos.xyz, per_instance_data.quat);

	// we use column-major matrices, so transforms are chained right-to-left
	const float4x4 world_view_projection_matrix = mul(g_view_projection_matrix, world_matrix);

	//
	output.position_cs = mul(
		world_view_projection_matrix
		, local_position
	);
	
	//
	const float4x4 world_view_matrix = mul(g_view_matrix, world_matrix);

	output.position_vs = mul(
		world_view_matrix
		, local_position
	).xyz;
	
	//
	const uint ship_type = asuint(per_instance_data.pos.w);


#define first_fighter_ship_type 4	// obj_alien_fighter_ship

	if(ship_type < first_fighter_ship_type) {
		output.color = float3(0.7, 0.7, 0.9);	// friendly ships
	} else {
		output.color = float3(0.8, 0.1, 0.05);
	}
	
    return output;
}

/*
==========================================================
	PIXEL SHADER
==========================================================
*/

void main_PS(
	in VS_to_PS pixel_in
	
	,
#if __PASS == FillGBuffer
	out PS_to_GBuffer pixel_out_
#else
	out float3 pixel_out_: SV_Target
#endif
)
{
	const float3 diffuse_color = pixel_in.color;

	//
	const float3 pixel_normal_in_view_space = normalize(
		-cross(ddx(pixel_in.position_vs), ddy(pixel_in.position_vs))
	);

	//
	const float3 specular_color = float3(1,1,1);

	//

#if __PASS == FillGBuffer

	Surface_to_GBuffer surface;
	surface.setDefaults();
	{
		surface.normal = pixel_normal_in_view_space;
		surface.albedo = diffuse_color.rgb;

		surface.specular_color = specular_color;
		surface.specular_power = 10.0f;

		surface.metalness = PBR_metalness();
		surface.roughness = PBR_roughness();
		
#if SG_USE_SDF_VOLUMES
		float3 sample_pos = float3(0,0,0);
		int mip_level = 0;
		surface.sun_visibility = t_sdf_volume.SampleLevel( s_sdf_volume, sample_pos, mip_level ).r;
#endif
	}
	GBuffer_Write( surface, pixel_out_ );

#else

	pixel_out_ = diffuse_color;

#endif

}
