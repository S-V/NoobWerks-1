// For rendering voxel terrains with Discrete Level-Of-Detail ("stitching").
/*

%%
pass FillGBuffer {
	vertex = main_VS
	pixel = main_PS
	state = default
}

pass DebugWireframe {
	vertex = VS_Wireframe
	geometry = GSSolidWire
	pixel = PSSolidWireFade

	//state = solid_wireframe
	render_state = {
		rasterizer = {
			fill_mode	= 'Solid'
			cull_mode 	= 'Back'
			flags = (
				'Enable_DepthClip'
			)
		}
		depth_stencil = {
			flags = (
				'Enable_DepthTest'
				; no depth writes
			)
			; Reverse Z:
			comparison_function = 'Greater_Equal'
		}
	}
}

samplers {
	s_diffuseMap = DiffuseMapSampler
	s_normalMap = NormalMapSampler
}

%%

//TODO:
resources = [
	{
		name = 't_diffuseMaps'
		asset = 'voxel_terrain_dmaps'
	}
	{
		name = 't_normalMaps'
		asset = 'voxel_terrain_nmaps'
	}
]

*/

#define DEBUG_NO_LIGHTING	(0)


#define FLAT_SHADING		(0)//

#define USE_DIFFUSE_MAP		(1)//

#define USE_NORMAL_MAP		(1)//
//#define USE_VERTEX_COLOR	(0)


#include <Shared/nw_shared_globals.h>
#include <Common/transform.hlsli>
#include "_VS.h"
#include "_PS.h"
#include "_fog.h"
#include <GBuffer/nw_gbuffer_write.h>
//#include "vxgi_interop.h"
#include "Debug/solid_wireframe.h"

// Material textures
Texture2DArray	t_diffuseMaps;
Texture2DArray	t_normalMaps;

SamplerState	s_diffuseMap<string x = "test annotation - annotations are not available via HLSL reflection?";>;
SamplerState	s_normalMap;

struct VS_Out
{
	float4 position : SV_Position;

	float3 normalVS : Normal0;	// view-space normal
	float3 normalWS : Normal1;	// world-space normal

#if FLAT_SHADING
	float3 positionVS : Position0;
#endif

	float3 positionWS : Position1;	// world-space position

#if USE_NORMAL_MAP
	float3 tangentVS : Tangent;	// view-space tangent
#endif

	float4	color : Color;

	nointerpolation uint	material_id : Color1;
};

void main_VS( in Vertex_DLOD _inputs, out VS_Out _outputs )
{
	// get the normalized vertex position in the chunk's seam space
	uint	material_id;
	const float3 normalizedPosition = unpack_NormalizedPosition_and_MaterialIDs( _inputs, material_id );	// [0..1]

	const float4 f4LocalPosition = float4( normalizedPosition, 1.0f );
	const float3 localNormal = UnpackVertexNormal( _inputs.N );

	_outputs.position = Pos_Local_To_Clip( f4LocalPosition );
#if FLAT_SHADING
	_outputs.positionVS = Pos_Local_To_View( f4LocalPosition ).xyz;
#endif
	_outputs.positionWS = Pos_Local_To_World( f4LocalPosition ).xyz;

	_outputs.normalVS = Dir_Local_To_View( localNormal );
	_outputs.normalWS = Dir_Local_To_World( localNormal );

#if USE_NORMAL_MAP
	const float3 localTangent = float3(1,1,1);
	_outputs.tangentVS = Dir_Local_To_View(localTangent);
#endif
	
	_outputs.material_id = material_id;

	_outputs.color = _inputs.color;
}




//#ifdef DEFERRED

void main_PS( in VS_Out ps_in, out PS_to_GBuffer _outputs )
{
	const float3 worldSpacePosition = ps_in.positionWS;

	// get the world-space normal of the fragment
	const float3 worldSpaceNormal = normalize( ps_in.normalWS );

	// Eric Lengyel, 2010, P.49.
	// Calculate a vector that holds ones for values of s that should be negated. 
	//float3 flip = float3(worldSpaceNormal.x < 0.0, worldSpaceNormal.y >= 0.0, worldSpaceNormal.z < 0.0); 

	// calculate the blend factor ('delta') for each direction
	const float blendSharpness = 0.5f;
	// Raise each component of normal vector to 4th power.
	const float power = 4.0f;

	float3 blendWeights = saturate(abs(worldSpaceNormal) - blendSharpness);
//	blendWeights = pow( blendWeights, power );
	// Normalize result by dividing by the sum of its components. 
	blendWeights /= dot(blendWeights, (float3)1.0f);	// force sum to 1.0

static const float texture_scale =
	0.04f//0.002//0.04
	;

	const VoxelMaterial	voxelMaterial = g_voxel_materials[ ps_in.material_id.x ];

	// the third component is an index into the texture array:
	const float3 texCoordA0 = float3( worldSpacePosition.yz * texture_scale, voxelMaterial.textureIDs.x );
	const float3 texCoordA1 = float3( worldSpacePosition.zx * texture_scale, voxelMaterial.textureIDs.y );
	const float3 texCoordA2 = float3( worldSpacePosition.xy * texture_scale, voxelMaterial.textureIDs.z );

	
	float3 albedo;
	
#if USE_DIFFUSE_MAP
	
	// sample diffuse color
	const float3 sampledColorAX = t_diffuseMaps.Sample(s_diffuseMap, texCoordA0).rgb;
	const float3 sampledColorAY = t_diffuseMaps.Sample(s_diffuseMap, texCoordA1).rgb;
	const float3 sampledColorAZ = t_diffuseMaps.Sample(s_diffuseMap, texCoordA2).rgb;
	// blend the results of the 3 planar projections
	const float3 blendedColor =
		sampledColorAX * blendWeights.x +
		sampledColorAY * blendWeights.y +
		sampledColorAZ * blendWeights.z;
		
	albedo = blendedColor;
#if DEBUG_NO_LIGHTING
	albedo = (float3)0.5f + (blendedColor * 1e-4f);
#endif

#else
		
	albedo = (float3)1;
	
#endif


#if FLAT_SHADING
	// face normal in eye-space:
	// '-' because we use right-handed coordinate system
	float3 viewSpaceNormal = normalize(-cross(ddx(ps_in.positionVS), ddy(ps_in.positionVS)));
#else
	float3 viewSpaceNormal = normalize(ps_in.normalVS);
#endif

#if USE_NORMAL_MAP
	const float3 viewSpaceTangent = normalize(ps_in.tangentVS);
	//const float3 viewSpaceTangent = normalize(ps_in.tangentVS);
	const float3 viewSpaceBitangent = normalize(cross( viewSpaceNormal, viewSpaceTangent ));

	const float3 sampledNormal0 = ExpandNormal(t_normalMaps.Sample(s_normalMap, texCoordA0).rgb);
	const float3 sampledNormal1 = ExpandNormal(t_normalMaps.Sample(s_normalMap, texCoordA1).rgb);
	const float3 sampledNormal2 = ExpandNormal(t_normalMaps.Sample(s_normalMap, texCoordA2).rgb);
	//const float3 blendedNormal = sampledNormal0 * blendWeights.x + sampledNormal1 * blendWeights.y + sampledNormal2 * blendWeights.z;
	const float3 bentNormal =
		(viewSpaceBitangent * sampledNormal0.x + viewSpaceBitangent * sampledNormal0.y) * blendWeights.x +
		(viewSpaceBitangent * sampledNormal1.x + viewSpaceBitangent * sampledNormal1.y) * blendWeights.y +
		(viewSpaceBitangent * sampledNormal2.x + viewSpaceBitangent * sampledNormal2.y) * blendWeights.z;
#if DEBUG_NO_LIGHTING
	viewSpaceNormal += bentNormal * 1e-4f;
#else
	viewSpaceNormal += bentNormal;
	//viewSpaceNormal += ExpandNormal( blendedNormal );
#endif
#endif

// #if USE_VERTEX_COLOR
	// albedo *= ps_in.color.rgb;
	// //albedo = lerp( albedo, ps_in.color, 0.5f );
// #endif

	//dbg:
	//albedo *= 1 - ps_in.color.a;


	Surface_to_GBuffer surface;
	surface.setDefaults();
	{
		surface.normal = normalize(viewSpaceNormal);
		
		surface.albedo = albedo.rgb;
	
	#if DEBUG
		surface.albedo *= float3(0,1,0);
	#endif

		surface.metalness = voxelMaterial.metalness();
		surface.roughness = voxelMaterial.roughness();

		surface.accessibility = ps_in.color.a;
	}
	GBuffer_Write( surface, _outputs );
}

//#endif // DEFERRED


/*
==============================================================================
	WIREFRAME RENDERING
==============================================================================
*/
void VS_Wireframe( in Vertex_DLOD _inputs, out GS_INPUT _outputs )
{
	// get the normalized vertex position in the chunk's seam space
	uint	material_id;
	const float3 normalizedPosition = unpack_NormalizedPosition_and_MaterialIDs( _inputs, material_id );	// [0..1]

	const float4 f4LocalPosition = float4( normalizedPosition, 1.0f );
	const float3 localNormal = UnpackVertexNormal( _inputs.N );

	//
	_outputs.Pos = Pos_Local_To_Clip( f4LocalPosition );
	_outputs.PosV = Pos_Local_To_View( f4LocalPosition );
}

