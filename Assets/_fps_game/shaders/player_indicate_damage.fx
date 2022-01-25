/*

damage indicator, explosion flash, etc.

%%
pass PostFX
{
	vertex = main_VS
	pixel = main_PS

	render_state = {
		rasterizer = 	{
			fill_mode	= 'Solid'
			cull_mode 	= 'None'
			flags = ()
		}

		depth_stencil = 	{
			flags = ()
			comparison_function = 'Always'
		}

		blend = {
			flags = (
				'Enable_Blending'
			)
			color_channel = {
				operation = 'ADD'
				source_factor = 'SRC_ALPHA'
				destination_factor = 'INV_SRC_ALPHA'
			}
			write_mask	= ('ENABLE_ALL')
		}
	}
}

%%
*/
#include <Shared/nw_shared_globals.h>
#include <GBuffer/nw_gbuffer_read.h>
#include "_screen_shader.h"
#include <Common/Colors.hlsl>

// color and alpha
float4	u_color;

/*
==========================================================
	VERTEX SHADER
==========================================================
*/
struct VS_OUT
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
};

VS_OUT main_VS( in uint vertexID : SV_VertexID )	// vertexID:=[0,1,2]
{
    VS_OUT output;
	GetFullScreenTrianglePosTexCoord( vertexID, output.position, output.texCoord );
    return output;
}

/*
==========================================================
	PIXEL SHADER
==========================================================
*/
float4 main_PS( in VS_OUT ps_in ) : SV_Target
{
	return u_color;
}
