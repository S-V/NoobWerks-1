/*

%%
pass FillGBuffer {
	vertex = VertexShaderMain
	pixel = PixelShaderMain

	state = default

	//defines = [ 'DEFERRED' ]
}

samplers
{
	s_diffuseMap = DiffuseMapSampler
}

%%

*/

#include <Shared/nw_shared_globals.h>
#include "_VS.h"
#include <GBuffer/nw_gbuffer_write.h>
#include <Common/transform.hlsli>

struct VSOut
{
	float4 position : SV_Position;
	float2 texCoord : TexCoord0;
	float3 viewNormal : Normal0;
};

float4		u_diffuse_color = float4(1,1,1,1);

Texture2D		t_diffuseMap;
SamplerState	s_diffuseMap;

void VertexShaderMain( in DrawVertex inputs, out VSOut outputs )
{
	float4 localPosition = float4(inputs.position, 1.0f);
	float3 localNormal = UnpackVertexNormal(inputs.normal);

	outputs.position = Pos_Local_To_Clip( localPosition );
	outputs.texCoord = inputs.texCoord;
	outputs.viewNormal = Dir_Local_To_View(localNormal);
}


//#ifdef DEFERRED

void PixelShaderMain( in VSOut inputs, out PS_to_GBuffer _outputs )
{
	float4 diffuse_color = t_diffuseMap.Sample( s_diffuseMap, inputs.texCoord.xy ).rgba;

	diffuse_color = diffuse_color;// * u_diffuse_color;

	Surface_to_GBuffer surface;
	surface.setDefaults();
	{
		surface.normal = inputs.viewNormal;
		surface.albedo = diffuse_color.rgb;
	}
	GBuffer_Write( surface, _outputs );
}

// #else

// void PixelShaderMain( in VSOut inputs, out float4 pixelColor : SV_Target )
// {
	// float4 diffuse_color = t_diffuseMap.Sample( s_diffuseMap, inputs.texCoord.xy ).rgba;
	// pixelColor = u_diffuse_color * diffuse_color;
// }

// #endif
