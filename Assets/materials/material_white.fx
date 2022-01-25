/*

A material without any texture/color - useful for testing lighting

%%

pass Opaque_GBuffer {
	vertex = VertexShaderMain
	pixel = PixelShaderMain
	state = default
	defines = [ DEFERRED ]
}

%%

*/

#include <Shared/nw_shared_globals.h>
#include "_VS.h"
#include "GBuffer/nw_gbuffer_write.h"
#include <Common/transform.hlsli>
#include <Common/Colors.hlsl>


struct VSOut
{
	float4 position : SV_Position;
	float3 normal_VS : Normal0;
};

void VertexShaderMain( in DrawVertex vertex, out VSOut vs_out )
{
	float4 localPosition = float4(vertex.position, 1.0f);
	float3 localNormal = UnpackVertexNormal(vertex.normal);

	vs_out.position = Pos_Local_To_Clip( localPosition );
	vs_out.normal_VS = Dir_Local_To_View(localNormal);
}


#ifdef DEFERRED

void PixelShaderMain( in VSOut pixel_in, out PS_to_GBuffer outputs_ )
{
	//
	Surface_to_GBuffer surface;
	surface.setDefaults();
	{
		surface.normal = pixel_in.normal_VS;
		surface.albedo = WHITE.rgb;
	}
	GBuffer_Write( surface, outputs_ );
}

#else

float3 PixelShaderMain( in VSOut pixel_in ) : SV_Target
{
	return WHITE.rgb;
}

#endif
